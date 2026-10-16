#include "config.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <ctype.h>

#include <pthread.h>

#include "urweb.h"
#include "request.h"
#include "queue.h"

#include "fastcgi.h"

#define THREAD_LOCAL __thread

extern uw_app uw_application;

// The most content a record carries: its length has two bytes.
#define FCGI_CONTENT_MAX 65535

typedef struct {
  unsigned char version;
  unsigned char type;
  unsigned char requestIdB1;
  unsigned char requestIdB0;
  unsigned char contentLengthB1;
  unsigned char contentLengthB0;
  unsigned char paddingLength;
  unsigned char reserved;
  unsigned char contentData[FCGI_CONTENT_MAX];
} FCGI_Record;

// A record's header: what comes before its content.
#define FCGI_HEADER_SIZE (sizeof(FCGI_Record) - FCGI_CONTENT_MAX)

typedef struct {
  FCGI_Record r;
  int sock;
  // While a worker writes a response: the records not sent yet, to go out
  // in one write.  NULL when each record is sent as it is made.
  unsigned char *buf;
  size_t used;
} FCGI_Output;

// Room for two whole records: a response that fits goes out in one write,
// and one that does not in pieces of at least one record.
#define FCGI_OUTPUT_BUFFER (2 * sizeof(FCGI_Record))

typedef struct {
  FCGI_Record r;
  int available, used, sock;
} FCGI_Input;

// The FastCGI request ID corresponding to the request being handled by the
// current worker thread.  (Each worker thread can only handle one request at a
// time.)
static THREAD_LOCAL int current_request_id;

// Reads the FastCGI request ID from a FastCGI record.  The result is guaranteed
// to be in the range [0, 2^16); this function returns an int to avoid C type
// promotion insanity.
static int fastcgi_request_id(const FCGI_Record* const r) {
  const int requestid = r->requestIdB1 << 8 | r->requestIdB0;
  assert(requestid >= 0);
  assert(requestid <= UINT16_MAX);
  return requestid;
}

// An output on the connection sock (-1: none yet): the fields of its
// record that every record shares, and no buffer.
static void fastcgi_output_init(FCGI_Output *o, int sock) {
  o->sock = sock;
  o->buf = NULL;
  o->r.version = FCGI_VERSION_1;
  o->r.paddingLength = 0;
  o->r.reserved = 0;
}

static FCGI_Output *fastcgi_output() {
  FCGI_Output *o = malloc(sizeof(FCGI_Output));

  fastcgi_output_init(o, -1);

  return o;
}

static FCGI_Input *fastcgi_input() {
  FCGI_Input *i = malloc(sizeof(FCGI_Input));

  i->available = i->used = 0;

  return i;
}

static void fastcgi_input_reset(FCGI_Input *i) {
  i->available = i->used = 0;
}

// The records buffered for a response, sent.
static int fastcgi_flush(FCGI_Output *o) {
  int r = o->used ? uw_really_send(o->sock, o->buf, o->used) : 0;
  o->used = 0;
  return r;
}

// n bytes of records added to a response's buffer, sent first if they would
// not fit.
static int fastcgi_append(FCGI_Output *o, const unsigned char *bytes, size_t n) {
  if (o->used + n > FCGI_OUTPUT_BUFFER && fastcgi_flush(o))
    return -1;
  memcpy(o->buf + o->used, bytes, n);
  o->used += n;
  return 0;
}

static int fastcgi_send(FCGI_Output *o,
                        unsigned char type,
                        unsigned short contentLength) {
  o->r.type = type;
  assert(current_request_id <= UINT16_MAX);
  o->r.requestIdB1 = current_request_id >> 8;
  o->r.requestIdB0 = current_request_id & 0x000000ff;
  o->r.contentLengthB1 = contentLength >> 8;
  o->r.contentLengthB0 = contentLength & 255;
  if (o->buf)
    return fastcgi_append(o, (const unsigned char *)&o->r, FCGI_HEADER_SIZE + contentLength);
  return uw_really_send(o->sock, &o->r, FCGI_HEADER_SIZE + contentLength);
}

static FCGI_Record *fastcgi_recv(FCGI_Input *i) {
  if (i->used > 0) {
    memmove((void*)&i->r, (void*)&i->r + i->used, i->available - i->used);
    i->available -= i->used;
    i->used = 0;
  }

  while (1) {
    ssize_t n;

    if (i->available >= FCGI_HEADER_SIZE
        && i->available >= FCGI_HEADER_SIZE
        + ((i->r.contentLengthB1 << 8) | i->r.contentLengthB0)
        + i->r.paddingLength) {
      i->used = FCGI_HEADER_SIZE
        + ((i->r.contentLengthB1 << 8) | i->r.contentLengthB0)
        + i->r.paddingLength;

      return &i->r;
    }

    n = recv(i->sock, (void*)&i->r + i->available, sizeof(i->r) - i->available, 0);

    if (n <= 0)
      return NULL;

    i->available += n;
  }
}

static void on_success(uw_context ctx) {
  (void)ctx;
}

static void on_failure(uw_context ctx) {
  uw_write_header(ctx, "Status: 500 Internal Server Error\r\n");
}

static int write_stdout(void *data, const char *buf, size_t len) {
  FCGI_Output *o = (FCGI_Output *)data;
  while (len > 0) {
    size_t len2 = len;
    if (len2 > FCGI_CONTENT_MAX)
      len2 = FCGI_CONTENT_MAX;
    if (o->buf) {
      // The header in place, then the data, copied once.
      o->r.type = FCGI_STDOUT;
      o->r.requestIdB1 = current_request_id >> 8;
      o->r.requestIdB0 = current_request_id & 0x000000ff;
      o->r.contentLengthB1 = len2 >> 8;
      o->r.contentLengthB0 = len2 & 255;
      if (fastcgi_append(o, (const unsigned char *)&o->r, FCGI_HEADER_SIZE)
          || fastcgi_append(o, (const unsigned char *)buf, len2)) {
        fprintf(stderr, "fastcgi_send() failed in write_stdout().\n");
        return -1;
      }
      buf += len2;
      len -= len2;
      continue;
    }
    memcpy(o->r.contentData, buf, len2);
    if (fastcgi_send(o, FCGI_STDOUT, len2)) {
      fprintf(stderr, "fastcgi_send() failed in write_stdout().\n");
      return -1;
    }
    buf += len2;
    len -= len2;
  }

  return 0;
}

#include <errno.h>

static void close_stream(FCGI_Output *o, unsigned char type) {
  if (fastcgi_send(o, type, 0))
    fprintf(stderr, "fastcgi_send() failed in close_stream().\n");
}

// The program logs as the HTTP server does: errors on stderr, and debug
// output on stdout, which -q turns off.  Nothing goes on a request's
// FastCGI stderr stream, which a web server puts in its error log apart
// from the program's other output.  A complaint about a request as the
// web server sent it goes the same way.
static void log_error(void *data, const char *fmt, ...) {
  va_list ap;
  (void)data;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fflush(stderr);
  va_end(ap);
}

static int quiet = 0;

static void log_debug(void *data, const char *fmt, ...) {
  (void)data;

  if (!quiet) {
    va_list ap;
    va_start(ap, fmt);

    vprintf(fmt, ap);
    fflush(stdout);
    va_end(ap);
  }
}

typedef struct {
  char *name, *value;
  unsigned name_len, value_len;
} nvp;

static char *search_nvps(nvp *nvps, const char *h) {
  for (; nvps->name[0]; ++nvps)
    if (!strcmp(h, nvps->name))
      return nvps->value;

  return NULL;
}

typedef struct {
  nvp *nvps;
  char *uppercased;
  int n_nvps, uppercased_len;
} headers;

static char *get_header(void *data, const char *h) {
  headers *hs = (headers *)data;
  size_t len = strlen(h);
  char *s;
  const char *saved_h = h;

  if (len > hs->uppercased_len) {
    hs->uppercased_len = len;
    hs->uppercased = realloc(hs->uppercased, len + 6);
  }

  strcpy(hs->uppercased, "HTTP_");
  for (s = hs->uppercased+5; *h; ++h)
    *s++ = *h == '-' ? '_' : toupper((int)*h);
  *s = 0;

  if (!strcasecmp(saved_h, "Content-length")
      || !strcasecmp(saved_h, "Content-type")) {
    if ((s = search_nvps(hs->nvps, hs->uppercased + 5)))
      return s;
  }

  return search_nvps(hs->nvps, hs->uppercased);
}

static char *get_env(void *data, const char *h) {
  headers *hs = (headers *)data;
  char *v = search_nvps(hs->nvps, h);
  return v ? v : getenv(h);
}

static int read_funny_len(unsigned char **buf, int *len) {
  if (*len <= 0)
    return -1;

  if ((*buf)[0] >> 7 == 0) {
    int r = (*buf)[0];
    ++*buf;
    --*len;
    return r;
  }
  else if (*len < 4)
    return -1;
  else {
    int r = (((*buf)[0] & 0x7f) << 24) + ((*buf)[1] << 16) + ((*buf)[2] << 8) + (*buf)[3];
    *buf += 4;
    *len -= 4;
    return r;
  }
}

static int read_nvp(unsigned char **buf, int len, nvp *nv) {
  int nameLength, valueLength;

  if ((nameLength = read_funny_len(buf, &len)) < 0)
    return -1;
  if ((valueLength = read_funny_len(buf, &len)) < 0)
    return -2;
  if (len < nameLength + valueLength)
    return -3;

  if (nameLength+1 > nv->name_len) {
    nv->name_len = nameLength+1;
    nv->name = realloc(nv->name, nv->name_len);
  }
  if (valueLength+1 > nv->value_len) {
    nv->value_len = valueLength+1;
    nv->value = realloc(nv->value, nv->value_len);
  }

  memcpy(nv->name, *buf, nameLength);
  nv->name[nameLength] = 0;

  memcpy(nv->value, *buf + nameLength, valueLength);
  nv->value[valueLength] = 0;

  *buf += nameLength + valueLength;

  return 0;
}

// The end of a response: its stdout and stderr streams closed, and
// FCGI_END_REQUEST with the request's status.
static void fastcgi_end_request(FCGI_Output *out, request_result rr) {
  FCGI_EndRequestBody *erb = (FCGI_EndRequestBody *)out->r.contentData;

  close_stream(out, FCGI_STDOUT);
  close_stream(out, FCGI_STDERR);

  if (rr == SERVED)
    erb->appStatusB3 = erb->appStatusB2 = erb->appStatusB1 = erb->appStatusB0 = 0;
  else
    erb->appStatusB3 = erb->appStatusB2 = erb->appStatusB1 = erb->appStatusB0 = 0xFF;

  erb->protocolStatus = FCGI_REQUEST_COMPLETE;
  fastcgi_send(out, FCGI_END_REQUEST, sizeof(FCGI_EndRequestBody));
  if (out->buf && fastcgi_flush(out))
    fprintf(stderr, "fastcgi_flush() failed in fastcgi_end_request().\n");
}

static int fastcgi_close_with(FCGI_Output *out, request_result rr) {
  fastcgi_end_request(out, rr);
  return close(out->sock);
}

// -T: how long, in seconds, a connection kept open for further requests
// may sit idle, waiting for the next request to begin, before the program
// closes it.  0, the default, is no limit: a kept connection is the web
// server's to close.
static int recv_timeout = 0;

// The wait for the next bytes on sock limited to seconds, 0 for not at
// all (SO_RCVTIMEO).  Set while a kept connection waits for its next
// request, and taken off once that request has begun, so that a request's
// own bytes may come as slowly as the web server sends them.
static void idle_limit(int sock, int seconds) {
  struct timeval tv = {seconds, 0};
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv);
}

static int fastcgi_close(int sock) {
  FCGI_Output out;

  fastcgi_output_init(&out, sock);
  return fastcgi_close_with(&out, SERVED);
}

int fastcgi_send_normal(int sock, const void *buf, ssize_t len) {
  FCGI_Output out;

  fastcgi_output_init(&out, sock);
  return write_stdout(&out, buf, len);
}

static void *worker(void *data) {
  FCGI_Input *in = fastcgi_input();
  FCGI_Output *out = fastcgi_output();
  unsigned char *response = malloc(FCGI_OUTPUT_BUFFER);
  uw_loggers ls = {out, log_error, log_debug};
  uw_context ctx = uw_request_new_context(*(int *)data, &uw_application, &ls);
  uw_request_context rc = uw_new_request_context();
  headers hs;
  size_t body_size = 0;
  char *body = malloc(0);
  size_t path_size = 0;
  char *path_buf = malloc(0);

  hs.uppercased = malloc(6);
  hs.uppercased_len = 0;
  hs.nvps = malloc(sizeof(nvp));
  hs.n_nvps = 1;
  hs.nvps[0].name = malloc(1);
  hs.nvps[0].name_len = 1;
  hs.nvps[0].value = malloc(0);
  hs.nvps[0].value_len = 0;

  // keep: the web server asked for the connection to stay open after the
  // request (FCGI_KEEP_CONN), and the next request comes on it.  The
  // connection then holds this thread until it closes or waits past -T.
  int keep = 0;

  while (1) {
    FCGI_Record *r;
    size_t used_nvps = 0;
    int body_len, body_read;
    char *s;
    char *method, *path, *path_info, *query_string;
    int kept = keep;

    keep = 0;
    if (!kept)
      in->sock = out->sock = uw_dequeue();

    if (!(r = fastcgi_recv(in))) {
      // A kept connection that the web server closes, or that waits past
      // -T, is simply done with.
      if (!kept)
        fprintf(stderr, "Error receiving initial message\n");
      goto done;
    }
    if (kept && recv_timeout > 0)
      idle_limit(in->sock, 0);   // the request has begun: no limit on its bytes

    // Save the FastCGI request ID this worker is handling so that fastcgi_send
    // can include it in its response.
    current_request_id = fastcgi_request_id(r);

    if (r->type != FCGI_BEGIN_REQUEST) {
      log_error(out, "First message is not BEGIN_REQUEST\n");
      goto done;
    } else if (r->contentData[1] != FCGI_RESPONDER) {
      log_error(out, "Request is for a role besides RESPONDER\n");
      goto done;
    }

    keep = r->contentData[2] & FCGI_KEEP_CONN;

    while (1) {
      unsigned char *buf;
      int len;

      if (!(r = fastcgi_recv(in))) {
        log_error(out, "Error receiving environment variables\n");
        goto done;
      }

      if (fastcgi_request_id(r) != current_request_id) {
        log_error(out, "Ignoring environment variables for request %d (current request has id %d)\n",
                  fastcgi_request_id(r), current_request_id);
        continue;
      }

      if (r->type != FCGI_PARAMS) {
        log_error(out, "Expected FCGI_PARAMS but got %d\n", r->type);
        goto done;
      }

      if (r->contentLengthB1 == 0 && r->contentLengthB0 == 0)
        break;

      len = (r->contentLengthB1 << 8) | r->contentLengthB0;

      for (buf = r->contentData; buf < r->contentData + len; ) {
        if (used_nvps == hs.n_nvps-1) {
          ++hs.n_nvps;
          hs.nvps = realloc(hs.nvps, hs.n_nvps * sizeof(nvp));
          hs.nvps[hs.n_nvps-1].name = malloc(1);
          hs.nvps[hs.n_nvps-1].value = malloc(0);
          hs.nvps[hs.n_nvps-1].name_len = 1;
          hs.nvps[hs.n_nvps-1].value_len = 0;
        }

        if (read_nvp(&buf, len - (buf - r->contentData), &hs.nvps[used_nvps]) < 0) {
          log_error(out, "Error reading FCGI_PARAMS name-value pair\n");
          goto done;
        }

        //log_error(out, "PARAM: %s -> %s\n", hs.nvps[used_nvps].name, hs.nvps[used_nvps].value);

        ++used_nvps;
      }
    }

    hs.nvps[used_nvps].name[0] = 0;

    if ((s = get_header(&hs, "Content-Length"))) {
      body_len = atoi(s);
      if (body_len < 0) {
        log_error(out, "Invalid Content-Length\n");
        goto done;
      }
    } else
      body_len = 0;

    if (body_len+1 > body_size) {
      body_size = body_len+1;
      body = realloc(body, body_size);
    }

    // Read the stream to its end, the empty record, even past Content-Length
    // bytes and even when there is no body.  Answering before that record
    // has come closes the connection while the web server may still be
    // writing it, and the web server then fails with EPIPE after the
    // response was sent.
    for (body_read = 0; ; ) {
      int this_len;

      if (!(r = fastcgi_recv(in))) {
        log_error(out, "Error receiving STDIN\n");
        goto done;
      }

      if (fastcgi_request_id(r) != current_request_id) {
        log_error(out, "Ignoring STDIN for request %d (current request has id %d)\n",
                  fastcgi_request_id(r), current_request_id);
        continue;
      }

      if (r->type != FCGI_STDIN) {
        log_error(out, "Expected FCGI_STDIN but got %d\n", r->type);
        goto done;
      }

      if (r->contentLengthB1 == 0 && r->contentLengthB0 == 0) {
        if (body_read < body_len)
          log_error(out, "FastCGI stdin ended %d bytes short of CONTENT_LENGTH\n", body_len - body_read);
        break;
      }

      this_len = (r->contentLengthB1 << 8) | r->contentLengthB0;

      if (body_read + this_len > body_len) {
        log_error(out, "Too much STDIN\n");
        goto done;
      }

      memcpy(&body[body_read], r->contentData, this_len);
      body_read += this_len;
    }

    body[body_read] = 0;

    if (!(method = search_nvps(hs.nvps, "REQUEST_METHOD"))) {
      log_error(out, "REQUEST_METHOD not set\n");
      goto done;
    }

    if (!(path = search_nvps(hs.nvps, "SCRIPT_NAME"))) {
      log_error(out, "SCRIPT_NAME not set\n");
      goto done;
    }

    if ((path_info = search_nvps(hs.nvps, "PATH_INFO"))) {
      int len1 = strlen(path), len2 = strlen(path_info);
      int len = len1 + len2 + 1;

      if (len > path_size) {
        path_size = len;
        path_buf = realloc(path_buf, path_size);
      }

      sprintf(path_buf, "%s%s", path, path_info);
      path = path_buf;
    }

    if (!(query_string = search_nvps(hs.nvps, "QUERY_STRING")))
      query_string = "";

    uw_set_headers(ctx, get_header, &hs);
    uw_set_env(ctx, get_env, &hs);

    {
      request_result rr;

      rr = uw_request(rc, ctx, method, path, query_string, body, body_read,
                      on_success, on_failure,
                      out, log_error, log_debug,
                      in->sock, fastcgi_send_normal, fastcgi_close);

      // A request for messages has handed its connection over; it is
      // closed once the messages are sent, whatever the web server asked.
      if (rr == KEEP_OPEN) {
        keep = 0;
        goto done2;
      }

      // The response, its end included, in one write when it fits.
      out->buf = response;
      out->used = 0;
      uw_output(ctx, write_stdout, out);
      if (keep) {
        fastcgi_end_request(out, rr);
        if (recv_timeout > 0)
          idle_limit(in->sock, recv_timeout);
      } else
        fastcgi_close_with(out, rr);
      out->buf = NULL;
      goto done2;
    }

  done:
    keep = 0;
    close(in->sock);
  done2:
    // What has come of the next request on a kept connection stays.
    if (!keep)
      fastcgi_input_reset(in);
    uw_reset(ctx);
    uw_heap_shrink(ctx);
  }

  return NULL;
}

static void help(char *cmd) {
  printf("Usage: %s [-t <thread-count>] [-q] [-T <seconds>]\n"
         "The '-q' option turns off the debug output on stdout.\n"
         "The '-T' option sets how many seconds a connection kept open for further\n"
         "requests may sit idle before it is closed (default: no limit).\n", cmd);
}

static uw_loggers ls = {NULL, log_error, log_debug};

int main(int argc, char *argv[]) {
  // The skeleton for this function comes from Beej's sockets tutorial.
  struct sockaddr_in their_addr; // connector's address information
  socklen_t sin_size;
  int nthreads = 1, i, *names, opt;
  char *fwsa = getenv("FCGI_WEB_SERVER_ADDRS"), *nthreads_s = getenv("URWEB_NUM_THREADS");

  if (nthreads_s) {
    nthreads = atoi(nthreads_s);
    if (nthreads <= 0) {
      fprintf(stderr, "Bad URWEB_NUM_THREADS value\n");
      return 1;
    }
  }

  signal(SIGPIPE, SIG_IGN);
  // A line at a time, whatever stdout is, as for the HTTP server.
  setvbuf(stdout, NULL, _IOLBF, 0);

  while ((opt = getopt(argc, argv, "hqt:T:")) != -1) {
    switch (opt) {
    case '?':
      fprintf(stderr, "Unknown command-line option");
      help(argv[0]);
      return 1;

    case 'h':
      help(argv[0]);
      return 0;

    case 'q':
      quiet = 1;
      break;

    case 'T':
      recv_timeout = atoi(optarg);
      if (recv_timeout < 0) {
        fprintf(stderr, "Invalid receive timeout\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 't':
      nthreads = atoi(optarg);
      if (nthreads <= 0) {
        fprintf(stderr, "Invalid thread count\n");
        help(argv[0]);
        return 1;
      }
      break;

    default:
      fprintf(stderr, "Unexpected getopt() behavior\n");
      return 1;
    }
  }

  uw_set_on_success("");
  uw_request_init(&uw_application, &ls);
  uw_request_prepare_heaps(&uw_application, nthreads);
  uw_request_start_threads(&uw_application, &ls);

  names = calloc(nthreads, sizeof(int));

  sin_size = sizeof their_addr;

  {
    pthread_t thread;

    pruner_data *pd = (pruner_data *)malloc(sizeof(pruner_data));
    pd->app = &uw_application;
    pd->loggers = &ls;

    if (pthread_create_big(&thread, NULL, client_pruner, pd)) {
      fprintf(stderr, "Error creating pruner thread\n");
      return 1;
    }
  }

  for (i = 0; i < nthreads; ++i) {
    pthread_t thread;
    names[i] = i;
    if (pthread_create_big(&thread, NULL, worker, &names[i])) {
      fprintf(stderr, "Error creating worker thread #%d\n", i);
      return 1;
    }
  }

  while (1) {
    int new_fd = accept(FCGI_LISTENSOCK_FILENO, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd < 0) {
      fprintf(stderr, "Socket accept failed\n");
      return 1;
    }

    if (fwsa) {
      char host[100], matched = 0;
      char *ips, *sep;

      if (getnameinfo((struct sockaddr *)&their_addr, sin_size, host, sizeof host, NULL, 0, NI_NUMERICHOST)) {
        fprintf(stderr, "Remote IP determination failed\n");
        return 1;
      }

      for (ips = fwsa; (sep = strchr(ips, ',')); ips = sep+1) {
        if (!strncmp(ips, host, sep - ips)) {
          matched = 1;
          break;
        }
      }

      if (!matched && strcmp(ips, host)) {
        fprintf(stderr, "Remote address is not in FCGI_WEB_SERVER_ADDRS");
        return 1;
      }
    }

    uw_enqueue(new_fd);
  }
}

void *uw_init_client_data() {
  return NULL;
}

void uw_free_client_data(void *data) {
  (void)data;
}

void uw_copy_client_data(void *dst, void *src) {
  (void)dst;
  (void)src;
}

void uw_do_expunge(uw_context ctx, uw_Basis_client cli, void *data) {
  (void)data;

  uw_ensure_transaction(ctx);
  uw_get_app(ctx)->expunger(ctx, cli);

  if (uw_commit(ctx))
    uw_error(ctx, UNLIMITED_RETRY, "Rerunning expunge transaction");
}

void uw_post_expunge(uw_context ctx, void *data) {
  (void)ctx;
  (void)data;
}

int uw_supports_direct_status = 0;
