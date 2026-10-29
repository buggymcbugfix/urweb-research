#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#include <pthread.h>

#include "memmem.h"
#include "urweb.h"
#include "request.h"

static int try_rollback(uw_context ctx, int will_retry) {
  int r = uw_rollback(ctx, will_retry);

  if (r) {
    uw_log_error(ctx, "Error running SQL ROLLBACK\n");
    uw_reset(ctx);
    uw_write(ctx, "HTTP/1.1 500 Internal Server Error\r\n");
    uw_write(ctx, "Content-type: text/plain\r\n\r\n");
    uw_write(ctx, "Error running SQL ROLLBACK\n");
    uw_set_error_message(ctx, "Database error; you are probably out of storage space.");
  }

  return r;
}

// A context with its database connection, for requests (serves_requests) or
// for the initializer alone.  An application with io handlers (io RPCs) may
// run io code on any context that serves requests, and io code needs the heap
// that never moves: a heap that relocated would ask for the request to be run
// again.  The initializer's context serves no request and is freed once the
// initializer has run, so it keeps the plain heap: nothing to reserve, and
// nothing of eight gigabytes to unmap.
static uw_context new_context(int id, uw_app *app, uw_loggers *ls, int serves_requests) {
  void *logger_data = ls->logger_data;
  uw_logger log_debug = ls->log_debug;
  uw_context ctx = uw_init(id, ls);
  int attempts_left = UW_RETRY_ATTEMPTS;

  if (uw_set_app(ctx, app)) {
    uw_log_at(ctx, UW_LOG_WARNING, "Unable to initialize request context.  Most likely the limit on number of form inputs has been exceeded.\n");
    uw_free(ctx);
    return NULL;
  }

  if (serves_requests && app->io_handlers)
    uw_io_prepare(ctx);

  while (1) {
    failure_kind fk = uw_begin_init(ctx);

    if (fk == SUCCESS) {
      uw_log(UW_LOG_DEBUG, "Database connection initialized.\n");
      break;
    } else if (fk == BOUNDED_RETRY) {
      if (uw_retry_again(fk, &attempts_left))
        log_debug(logger_data, "Initialization error triggers bounded retry: %s\n", uw_error_message(ctx));
      else {
        uw_log_error(ctx, "Fatal initialization error (out of retries): %s\n", uw_error_message(ctx));
        uw_free(ctx);
        return NULL;
      }
    } else if (fk == UNLIMITED_RETRY)
      log_debug(logger_data, "Initialization error triggers unlimited retry: %s\n", uw_error_message(ctx));
    else if (fk == FATAL) {
      uw_log_error(ctx, "Fatal initialization error: %s\n", uw_error_message(ctx));
      uw_free(ctx);
      return NULL;
    } else {
      uw_log_error(ctx, "Unknown uw_begin_init return code!\n");
      uw_free(ctx);
      return NULL;
    }
  }

  return ctx;
}

uw_context uw_request_new_context(int id, uw_app *app, uw_loggers *ls) {
  return new_context(id, app, ls, 1);
}

static void *ticker(void *data) {
  (void)data;

  while (1) {
    usleep(100000);
    ++uw_time;
  }

  return NULL;
}

typedef struct {
  int id;
  uw_loggers *ls;
  uw_periodic pdic;
  uw_app *app;
} periodic;

// The thread of a periodic task whose body is an io computation: run it
// outside any transaction, log a failure, sleep, again.  The body runs its
// own transactions (Basis.runTransaction), each with the retry protocol; the
// body itself is never run again, whatever failed, since its side effects
// cannot be undone.  The expunger's lock is taken by those transactions
// only, not across the body, which may be slow (a network call, say).
static void *io_periodic_loop(periodic *p) {
  uw_context ctx = uw_request_new_context(p->id, p->app, p->ls);

  if (!ctx)
    exit(1);
  uw_set_activity(ctx, "periodic task at %s", p->pdic.where);
  uw_io_prepare(ctx);

  while (1) {
    uw_reset(ctx);

    if (uw_runIo(ctx, p->pdic.callback) == SUCCESS) {
      // What the body registered outside any transaction (an FFI
      // transactional, say) is committed now that the body is over, as a
      // request's would be by the request's own commit.
      uw_commit(ctx);
      if (uw_has_error(ctx))
        uw_log_error(ctx, "Fatal error in io task, at its end: %s\n", uw_error_message(ctx));
    } else {
      uw_log_error(ctx, "Fatal error in io task: %s\n", uw_error_message(ctx));
      // Rolls back what was registered outside any transaction, and a
      // database transaction if an FFI left one open.
      if (uw_rollback(ctx, 0))
        uw_log_error(ctx, "Error running SQL ROLLBACK after a failed io task\n");
    }

    uw_heap_shrink(ctx);
    sleep(p->pdic.period);
  }
}

// The thread of one periodic task: run it, commit, sleep, again, for the
// life of the process.  A failure of any kind is logged, and the thread
// goes on to the next period: there is nobody to answer to, so the log is
// all there is, and a task that stopped running would be worse than one
// that failed once.
static void *periodic_loop(void *data) {
  periodic *p = (periodic *)data;
  uw_loggers *ls = p->ls;
  void *ld = ls->logger_data;
  uw_context ctx;

  if (p->pdic.io)
    return io_periodic_loop(p);

  ctx = uw_request_new_context(p->id, p->app, p->ls);

  if (!ctx)
    exit(1);
  uw_set_activity(ctx, "periodic task at %s", p->pdic.where);

  while (1) {
    int attempts_left = UW_RETRY_ATTEMPTS;
    failure_kind r;
    int again;

    uw_transaction_arrives();

    do {
      again = 0;
      uw_reset(ctx);
      r = uw_runCallback(ctx, p->pdic.callback);

      if (r == SUCCESS) {
        if (uw_commit(ctx)) {
          // A serialization failure at COMMIT: the database rolled the
          // transaction back and uw_commit undid the transactionals.  Run
          // the callback again, as for such a failure in the body.
          ls->log_debug(ld, "Commit conflict in periodic task; retrying\n");
          again = 1;
          continue;
        }
        if (uw_has_error(ctx)) {
          // Either COMMIT itself failed (a lock held past the busy timeout,
          // say) and the transaction may still be open, or a transactional's
          // commit callback failed after the database committed.  uw_commit
          // has freed the transactionals either way; an open transaction is
          // rolled back, so that the next tick can begin its own.
          uw_log_error(ctx, "Fatal error in periodic task, at or after commit: %s\n", uw_error_message(ctx));
          uw_reset_keep_error_message(ctx);
          if (try_rollback(ctx, 0))
            uw_log_at(ctx, UW_LOG_WARNING, "Periodic task continues after the failed rollback\n");
        }
        break;
      }

      if (r == BOUNDED_RETRY)
        ls->log_debug(ld, "Error triggers bounded retry in periodic: %s\n", uw_error_message(ctx));
      else if (r == UNLIMITED_RETRY)
        ls->log_debug(ld, "Error triggers unlimited retry in periodic: %s\n", uw_error_message(ctx));
      else if (r == FATAL)
        uw_log_error(ctx, "Fatal error in periodic task: %s\n", uw_error_message(ctx));
      else
        uw_log_error(ctx, "Unknown uw_runCallback return code in periodic task!\n");

      again = uw_retry_again(r, &attempts_left);
      if (try_rollback(ctx, again))
        // The rollback itself failed (no transaction was active, or the
        // connection is gone).  Nothing is held: the next period starts
        // afresh, and uw_ensure_transaction reconnects if it has to.
        uw_log_at(ctx, UW_LOG_WARNING, "Periodic task continues after the failed rollback\n");
    } while (again);

    if (r == BOUNDED_RETRY)
      uw_log_error(ctx, "Fatal error in periodic task (out of retries): %s\n", uw_error_message(ctx));

    uw_transaction_departs();

    uw_heap_shrink(ctx);
    sleep(p->pdic.period);
  };
}

static unsigned long long stackSize;

int pthread_create_big(pthread_t *outThread, void *foo, void *threadFunc, void *arg)
{
  (void)foo;

  if (stackSize > 0) {
    int err;
    pthread_attr_t stackSizeAttribute;

    err = pthread_attr_init(&stackSizeAttribute);
    if (err) return err;

    err = pthread_attr_setstacksize(&stackSizeAttribute, stackSize);
    if (err) return err;

    return pthread_create(outThread, &stackSizeAttribute, threadFunc, arg);
  } else {
    return pthread_create(outThread, NULL, threadFunc, arg);
  }
}

void uw_request_init(uw_app *app, uw_loggers* ls) {
  uw_context ctx;
  failure_kind fk;
  char *stackSize_s;

  uw_logger log_debug = ls->log_debug;
  void* logger_data = ls->logger_data;

  if ((stackSize_s = getenv("URWEB_STACK_SIZE")) != NULL && stackSize_s[0] != 0) {
    stackSize = atoll(stackSize_s);

    if (stackSize <= 0) {
      uw_log(UW_LOG_ERR, "Invalid stack size \"%s\"\n", stackSize_s);
      exit(1);
    }
  }

  uw_global_init();
  uw_app_init(app);

  ctx = new_context(0, app, ls, 0);

  if (!ctx)
    exit(1);

  for (fk = uw_initialize(ctx); fk == UNLIMITED_RETRY; fk = uw_initialize(ctx)) {
    log_debug(logger_data, "Unlimited retry during init: %s\n", uw_error_message(ctx));
    uw_rollback(ctx, 1);
    uw_reset(ctx);
  }

  if (fk != SUCCESS) {
    uw_log_error(ctx, "Failed to initialize database!  %s\n", uw_error_message(ctx));
    uw_rollback(ctx, 0);
    exit(1);
  }

  // The threads make connections of their own, so this one is closed, and
  // before a server that daemonizes forks (http.c).  SQLite keeps what it
  // knows of its file locks per process.  A connection carried across fork()
  // would leave the child believing it holds locks that only the parent held
  // in the kernel, and the parent exits: another process could then take the
  // database for its own, rebuild the WAL index and delete the WAL.
  if (uw_get_db(ctx))
    uw_close(ctx);
  uw_free(ctx);
}

void uw_request_prepare_heaps(uw_app *app, int workers) {
  uw_periodic *ps;
  int n = workers + 1;   // and the client pruner

  for (ps = app->periodics; ps->callback; ++ps)
    ++n;
  uw_heap_prepare(n);
}

// The threads that run for the life of the process: the ticker, and one per
// periodic task.  Separate from uw_request_init so that a server which forks
// to daemonize can do so in between: fork() carries only the calling thread,
// so threads created before it are lost in the child.
void uw_request_start_threads(uw_app *app, uw_loggers *ls) {
  uw_periodic *ps;
  int id;

  {
    pthread_t thread;

    if (uw_time_max && pthread_create_big(&thread, NULL, ticker, NULL)) {
      uw_log(UW_LOG_ERR, "Error creating ticker thread\n");
      exit(1);
    }
  }

  id = 1;
  for (ps = app->periodics; ps->callback; ++ps) {
    pthread_t thread;
    periodic *arg = malloc(sizeof(periodic));
    arg->id = id++;
    arg->ls = ls;
    arg->pdic = *ps;
    arg->app = app;

    if (pthread_create_big(&thread, NULL, periodic_loop, arg)) {
      uw_log(UW_LOG_ERR, "Error creating periodic thread\n");
      exit(1);
    }
  }
}


struct uw_rc {
  size_t path_copy_size, queryString_size;
  char *path_copy, *queryString;
};

uw_request_context uw_new_request_context(void) {
  uw_request_context r = malloc(sizeof(struct uw_rc));
  r->path_copy_size = 0;
  r->queryString_size = 1;
  r->path_copy = malloc(0);
  r->queryString = malloc(1);
  return r;
}

void uw_free_request_context(uw_request_context r) {
  free(r->path_copy);
  free(r->queryString);
  free(r);
}

// A request refused for its shape before any page runs: logged at notice
// after what the program was doing, and answered 400 with the reason as
// the body, so that the visitor hears what was wrong instead of nothing.
static request_result bad_request(uw_context ctx, const char *fmt, ...) {
  char small[UW_LOG_SMALL], *msg;
  va_list ap;

  va_start(ap, fmt);
  msg = uw_vformat(small, sizeof small, fmt, ap);
  va_end(ap);
  uw_log_at(ctx, UW_LOG_NOTICE, "%s\n", msg ? msg : fmt);
  uw_write_status(ctx, "400 Bad Request");
  uw_write_header(ctx, "Content-type: text/plain\r\n");
  uw_write(ctx, "Bad request: ");
  uw_write(ctx, msg ? msg : fmt);
  uw_write(ctx, "\n");
  if (msg && msg != small)
    free(msg);
  return FAILED;
}

// A method other than GET and POST, which no page takes: logged at notice,
// as a bad request is, and answered 405 with the methods that are allowed.
// The answer to a HEAD has no body: a client reads none after its head,
// and on a connection kept open would take one for the next answer.
static request_result method_not_allowed(uw_context ctx, const char *method) {
  uw_log_at(ctx, UW_LOG_NOTICE, "Not ready for non-GET/POST command: %s\n", method);
  uw_write_status(ctx, "405 Method Not Allowed");
  uw_write_header(ctx, "Allow: GET, POST\r\n");
  uw_write_header(ctx, "Content-type: text/plain\r\n");
  if (strcmp(method, "HEAD")) {
    uw_write(ctx, "Method not allowed: ");
    uw_write(ctx, method);
    uw_write(ctx, "\n");
  }
  return FAILED;
}

// uw_request, but for the line it logs at the end; *attempts counts how
// many times the handler started.
static request_result uw_request_attempts(uw_request_context rc, uw_context ctx,
                                          char *method, char *path, char *query_string,
                                          char *body, size_t body_len,
                                          void (*on_success)(uw_context), void (*on_failure)(uw_context),
                                          void *logger_data, uw_logger log_debug,
                                          int sock,
                                          int (*send)(int sockfd, const void *buf, ssize_t len),
                                          int (*close)(int fd),
                                          int *attempts) {
  int attempts_left = UW_RETRY_ATTEMPTS;
  failure_kind fk;
  int is_post = 0;
  char *boundary = NULL;
  size_t boundary_len = 0;
  char *inputs;
  const char *prefix = uw_get_url_prefix(ctx);
  char *s;
  int had_error = 0, bad = 0, is_fancy = 0;
  char errmsg[ERROR_BUF_LEN];

  uw_reset(ctx);
  uw_reproducible_request();

  rc->queryString[0] = 0;

  for (s = path; *s; ++s) {
    if (s[0] == '%' && s[1] == '2' && s[2] == '7') {
      s[0] = '\'';
      memmove(s+1, s+3, strlen(s+3)+1);
    }
  }

  uw_set_currentUrl(ctx, path);

  if (!strcmp(method, "POST")) {
    char *clen_s = uw_Basis_requestHeader(ctx, "Content-length");
    if (!clen_s) {
      clen_s = "0";
      /*uw_log_error(ctx, "No Content-length with POST\n");
      return FAILED;*/
    }
    int clen = atoi(clen_s);
    if (clen < 0)
      return bad_request(ctx, "Negative Content-length with POST");

    if (body_len < clen)
      return bad_request(ctx, "Request doesn't contain all POST data (according to Content-Length)");

    is_post = 1;
    uw_isPost(ctx);

    clen_s = uw_Basis_requestHeader(ctx, "Content-type");

    if (!clen_s || strcasecmp(clen_s, "application/x-www-form-urlencoded"))
      is_fancy = 1;

    if (clen_s && !strncasecmp(clen_s, "multipart/form-data", 19)) {
      if (strncasecmp(clen_s + 19, "; boundary=", 11))
        return bad_request(ctx, "Bad multipart boundary spec");

      boundary = clen_s + 28;
      boundary[0] = '-';
      boundary[1] = '-';
      boundary_len = strlen(boundary);
    } else if (clen_s) {
      uw_Basis_postBody pb = {clen_s, body, body_len};
      uw_postBody(ctx, pb);
    }
  } else if (strcmp(method, "GET"))
    return method_not_allowed(ctx, method);

  if (!strncmp(path, prefix, strlen(prefix))
      && !strcmp(path + strlen(prefix), ".msgs")) {
    char *id = uw_Basis_requestHeader(ctx, "UrWeb-Client");
    char *pass = uw_Basis_requestHeader(ctx, "UrWeb-Pass");

    if (sock < 0) {
      uw_log_error(ctx, ".msgs requested, but not socket supplied\n");
      return FAILED;
    }

    if (id && pass) {
      unsigned idn = atoi(id);
      uw_client_connect(ctx, idn, atoi(pass), sock, send, close);
      uw_log(UW_LOG_DEBUG, "Processed request for messages by client %u\n\n", idn);
      return KEEP_OPEN;
    }
    else
      return bad_request(ctx, "Missing fields in .msgs request: %s, %s", id ? id : "(none)", pass ? pass : "(none)");
  }

  if (boundary) {
    char *part = body, *after_sub_headers, *header, *after_header;
    size_t part_len;

    part = strstr(part, boundary);
    if (!part)
      return bad_request(ctx, "Missing first multipart boundary");
    part += boundary_len;

    while (1) {
      char *name = NULL, *filename = NULL, *type = NULL;

      if (part[0] == '-' && part[1] == '-')
        break;

      if (*part != '\r')
        return bad_request(ctx, "No \\r after multipart boundary");
      ++part;
      if (*part != '\n')
        return bad_request(ctx, "No \\n after multipart boundary");
      ++part;
            
      if (!(after_sub_headers = strstr(part, "\r\n\r\n")))
        return bad_request(ctx, "Missing end of headers after multipart boundary");
      after_sub_headers[2] = 0;
      after_sub_headers += 4;

      for (header = part; (after_header = strstr(header, "\r\n")); header = after_header + 2) {
        char *colon, *after_colon;

        *after_header = 0;
        if (!(colon = strchr(header, ':')))
          return bad_request(ctx, "Missing colon in multipart sub-header");
        *colon++ = 0;
        if (*colon++ != ' ')
          return bad_request(ctx, "No space after colon in multipart sub-header");
        
        if (!strcasecmp(header, "Content-Disposition")) {
          if (strncmp(colon, "form-data; ", 11))
            return bad_request(ctx, "Multipart data is not \"form-data\"");

          for (colon += 11; (after_colon = strchr(colon, '=')); colon = after_colon) {
            char *data;
            after_colon[0] = 0;
            if (after_colon[1] != '"')
              return bad_request(ctx, "Disposition setting is missing initial quote");
            data = after_colon+2;
            if (!(after_colon = strchr(data, '"')))
              return bad_request(ctx, "Disposition setting is missing final quote");
            after_colon[0] = 0;
            ++after_colon;
            if (after_colon[0] == ';' && after_colon[1] == ' ')
              after_colon += 2;

            if (!strcasecmp(colon, "name"))
              name = data;
            else if (!strcasecmp(colon, "filename"))
              filename = data;
          }
        } else if (!strcasecmp(header, "Content-Type")) {
          type = colon;
        }
      }

      part = memmem(after_sub_headers, body + body_len - after_sub_headers, boundary, boundary_len);
      if (!part)
        return bad_request(ctx, "Missing boundary after multipart payload");
      part[-2] = 0;
      part_len = part - after_sub_headers - 2;
      part[0] = 0;
      part += boundary_len;

      if (filename) {
        uw_Basis_file f = {filename, type, {part_len, after_sub_headers}};

        if (uw_set_file_input(ctx, name, f))
          return bad_request(ctx, "%s", uw_error_message(ctx));
      } else if (uw_set_input(ctx, name, after_sub_headers))
        return bad_request(ctx, "%s", uw_error_message(ctx));
    }
  }
  else if (!is_fancy) {
    inputs = is_post ? body : query_string;

    if (inputs) {
      char *name, *value;
      int len = strlen(inputs);

      if (len+1 > rc->queryString_size) {
        char *qs = realloc(rc->queryString, len+1); 
        if(qs == NULL) {
          uw_log_at(ctx, UW_LOG_WARNING, "queryString is too long (not enough memory)\n");
          return FAILED;
        }
        rc->queryString = qs;
        rc->queryString_size = len+1;
      }
      strcpy(rc->queryString, inputs);

      while (*inputs) {
        name = inputs;
        if ((inputs = strchr(inputs, '&')))
          *inputs++ = 0;
        else
          inputs = strchr(name, 0);

        if ((value = strchr(name, '='))) {
          *value++ = 0;
          if (uw_set_input(ctx, name, value))
            return bad_request(ctx, "%s", uw_error_message(ctx));
        }
        else if (uw_set_input(ctx, name, ""))
          return bad_request(ctx, "%s", uw_error_message(ctx));
      }
    }
  }

  while (1) {
    ++*attempts;
    uw_reproducible_attempt();
    uw_setQueryString(ctx, rc->queryString);

    if (!had_error) {
      size_t path_len = strlen(path);

      on_success(ctx);

      if (path_len + 1 > rc->path_copy_size) {
        char *pc = realloc(rc->path_copy, path_len + 1);
        if(pc == NULL) {
          uw_log_at(ctx, UW_LOG_WARNING, "Path is too long (not enough memory)\n");
          return FAILED;
        }
        rc->path_copy = pc;
        rc->path_copy_size = path_len + 1;
      }
      strcpy(rc->path_copy, path);

      uw_set_deadline(ctx, uw_time + uw_time_max);
      uw_transaction_arrives();
      fk = uw_begin(ctx, rc->path_copy);
    } else {
      uw_set_deadline(ctx, uw_time + uw_time_max);
      uw_transaction_arrives();
      fk = uw_begin_onError(ctx, errmsg, bad ? "400 Bad Request" : "500 Internal Server Error");
    }

    if (fk == SUCCESS || fk == RETURN_INDIRECTLY) {
      if (uw_commit(ctx)) {
        // A serialization failure at COMMIT: the database rolled the
        // transaction back and uw_commit undid the transactionals.  Run the
        // handler again, as for such a failure during the transaction.
        log_debug(logger_data, "Commit conflict; retrying\n");
        uw_transaction_departs();
        uw_reset_keep_request(ctx);
        continue;
      }
      if (uw_has_error(ctx) && !had_error) {
        uw_log_error(ctx, "Fatal error: %s\n", uw_error_message(ctx));
        uw_reset_keep_error_message(ctx);
        on_failure(ctx);

        if (uw_get_app(ctx)->on_error) {
          // On to the end of the loop, which rolls back what COMMIT may have
          // left open and leaves the expunger's lock, once, as for a failure
          // in the handler; then onError runs as a new attempt.
          had_error = 1;
          strcpy(errmsg, uw_error_message(ctx));
        } else {
          try_rollback(ctx, 0);
          uw_transaction_departs();

          uw_write_header(ctx, "Content-type: text/html\r\n");
          uw_write(ctx, "<html><head><title>Fatal Error</title></head><body>");
          uw_write(ctx, "Fatal error: ");
          uw_write(ctx, uw_error_message(ctx));
          uw_write(ctx, "\n</body></html>");
        
          return FAILED;
        }
      } else {
        uw_transaction_departs();
        return had_error ? FAILED : SERVED;
      }
    } else if (fk == BOUNDED_RETRY) {
      if (uw_retry_again(fk, &attempts_left))
        log_debug(logger_data, "Error triggers bounded retry: %s\n", uw_error_message(ctx));
      else {
        uw_log_error(ctx, "Fatal error (out of retries): %s\n", uw_error_message(ctx));

        if (!had_error && uw_get_app(ctx)->on_error) {
          had_error = 1;
          strcpy(errmsg, uw_error_message(ctx));
        } else {
          try_rollback(ctx, 0);
          uw_transaction_departs();

          uw_reset_keep_error_message(ctx);
          on_failure(ctx);
          uw_write_header(ctx, "Content-type: text/plain\r\n");
          uw_write(ctx, "Fatal error (out of retries): ");
          uw_write(ctx, uw_error_message(ctx));
          uw_write(ctx, "\n");

          return FAILED;
        }
      }
    } else if (fk == UNLIMITED_RETRY)
      log_debug(logger_data, "Error triggers unlimited retry: %s\n", uw_error_message(ctx));
    else if (fk == FATAL) {
      // A bad request, an argument or an input the request sent that is
      // none or malformed, is the request's own doing: a line at notice
      // and a 400, where an error of the program's is a line at err and a
      // 500. The mark is read here, before any reset clears it, and kept
      // for the onError page.
      bad = uw_bad_request_raised(ctx);
      if (bad)
        uw_log_at(ctx, UW_LOG_NOTICE, "Bad request: %s\n", uw_error_message(ctx));
      else
        uw_log_error(ctx, "Fatal error: %s\n", uw_error_message(ctx));

      if (uw_get_app(ctx)->on_error && !had_error) {
        had_error = 1;
        strcpy(errmsg, uw_error_message(ctx));
      } else {
        try_rollback(ctx, 0);
        uw_transaction_departs();

        uw_reset_keep_error_message(ctx);
        if (bad)
          uw_write_status(ctx, "400 Bad Request");
        else
          on_failure(ctx);
        uw_write_header(ctx, "Content-type: text/html\r\n");
        uw_write(ctx, bad ? "<html><head><title>Bad Request</title></head><body>Bad request: "
                 : "<html><head><title>Fatal Error</title></head><body>Fatal error: ");
        uw_write(ctx, uw_error_message(ctx));
        uw_write(ctx, "\n</body></html>");

        return FAILED;
      }
    } else {
      uw_log_error(ctx, "Unknown uw_handle return code!\n");

      if (uw_get_app(ctx)->on_error && !had_error) {
        had_error = 1;
        strcpy(errmsg, "Unknown uw_handle return code");
      } else {
        try_rollback(ctx, 0);
        uw_transaction_departs();
        
        uw_reset_keep_request(ctx);
        on_failure(ctx);
        uw_write_header(ctx, "Content-type: text/plain\r\n");
        uw_write(ctx, "Unknown uw_handle return code!\n");

        return FAILED;
      }
    }

    if (try_rollback(ctx, 1)) {
      uw_transaction_departs();
      return FAILED;
    }
    uw_transaction_departs();

    uw_reset_keep_request(ctx);
  }
}

// A figure for the line uw_request logs, to be printed with "%.*f %s".
typedef struct {
  int decimals;
  double value;
  const char *unit;
} amount;

// Where a figure moves to the next unit: from the value that would print
// as the unit's whole, "1024.0" with one decimal, "1000.0", or "1000"
// with none, since printf rounds.
#define ROUNDS_TO_1024 1023.95
#define ROUNDS_TO_1000 999.95
#define ROUNDS_TO_WHOLE_1000 999.5

// n bytes: as a whole number of B, or in the largest of KiB, MiB, GiB and
// TiB that leaves at least 1, to one decimal.
static amount in_bytes(size_t n) {
  static const char *const units[] = {"KiB", "MiB", "GiB", "TiB"};
  amount a = {0, (double)n, "B"};
  int u = 0;

  if (n < 1024)
    return a;
  for (a.value = n / 1024.0; u < 3 && a.value >= ROUNDS_TO_1024; ++u)
    a.value /= 1024;
  a.decimals = 1;
  a.unit = units[u];
  return a;
}

// A duration: as a whole number of us, or in ms or s to one decimal.
static amount in_time(double secs) {
  amount a = {0, secs * 1e6, "us"};

  if (a.value < ROUNDS_TO_WHOLE_1000)
    return a;
  a.decimals = 1;
  if (secs * 1e3 < ROUNDS_TO_1000) {
    a.value = secs * 1e3;
    a.unit = "ms";
  } else {
    a.value = secs;
    a.unit = "s";
  }
  return a;
}

// Serve a request, and log a line for it: how long it took, how many
// attempts when there was more than one, and the most heap it had in use
// at once and all it allocated (uw_heap_usage).  The time is that of the
// request's work, not of reading it in or of sending the response.  The
// line is formatted by the logger, so that one that prints nothing (-q)
// costs nothing more.  There is no line for a request for messages
// (.msgs), which hands its connection over to the client's channel and
// returns: nothing of the program runs.
request_result uw_request(uw_request_context rc, uw_context ctx,
                          char *method, char *path, char *query_string,
                          char *body, size_t body_len,
                          void (*on_success)(uw_context), void (*on_failure)(uw_context),
                          void *logger_data, uw_logger log_error, uw_logger log_debug,
                          int sock,
                          int (*send)(int sockfd, const void *buf, ssize_t len),
                          int (*close)(int fd)) {
  struct timespec start, end;
  int attempts = 0;
  request_result rr;

  (void)log_error;  // errors go through uw_log_error, with the request's method and path
  uw_set_activity(ctx, "%s %s", method, path);
  clock_gettime(CLOCK_MONOTONIC, &start);
  rr = uw_request_attempts(rc, ctx, method, path, query_string, body, body_len, on_success, on_failure,
                           logger_data, log_debug, sock, send, close, &attempts);

  if (rr != KEEP_OPEN) {
    const char *verb = rr == FAILED ? "Failed" : "Served";
    size_t peak, total;
    amount t, p, a;

    clock_gettime(CLOCK_MONOTONIC, &end);
    uw_heap_usage(ctx, &peak, &total);
    t = in_time((double)(end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9);
    p = in_bytes(peak);
    a = in_bytes(total);
    if (attempts > 1)
      log_debug(logger_data, "%s URI %s in %.*f %s; %d attempts; %.*f %s peak; %.*f %s total\n",
                verb, path, t.decimals, t.value, t.unit, attempts,
                p.decimals, p.value, p.unit, a.decimals, a.value, a.unit);
    else
      log_debug(logger_data, "%s URI %s in %.*f %s; %.*f %s peak; %.*f %s total\n",
                verb, path, t.decimals, t.value, t.unit,
                p.decimals, p.value, p.unit, a.decimals, a.value, a.unit);
  }

  return rr;
}

void *client_pruner(void *data) {
  pruner_data *pd = (pruner_data *)data;
  uw_context ctx = uw_request_new_context(0, pd->app, pd->loggers);

  if (!ctx)
    exit(1);

  while (1) {
    uw_prune_clients(ctx);
    uw_heap_shrink(ctx);
    sleep(5);
  }
}
