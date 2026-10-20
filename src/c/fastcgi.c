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
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <pthread.h>

#include "urweb.h"
#include "request.h"
#include "queue.h"

#include "fastcgi.h"

extern uw_app uw_application;

// The most content a record carries: its length has two bytes.
#define FCGI_CONTENT_MAX 65535

// The most padding a record carries after its content: its length has one
// byte.  A web server may pad so that the next record starts on a multiple
// of eight; nginx pads every record.
#define FCGI_PADDING_MAX 255

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
  // The id the web server gave the request, which every record of its
  // response carries.
  int request_id;
  // While a worker writes a response: the records not sent yet, to go out
  // in one write.  NULL when each record is sent as it is made.
  unsigned char *buf;
  size_t used;
} FCGI_Output;

// Room for two whole records: a response that fits goes out in one write,
// and one that does not in pieces of at least one record.
#define FCGI_OUTPUT_BUFFER (2 * sizeof(FCGI_Record))

typedef struct {
  union {
    FCGI_Record r;
    // r and the padding that may follow it, as one buffer: a record of the
    // most content fits whole with its padding.
    unsigned char bytes[sizeof(FCGI_Record) + FCGI_PADDING_MAX];
  };
  int available, used, sock;
} FCGI_Input;

// Reads the FastCGI request ID from a FastCGI record.  The result is guaranteed
// to be in the range [0, 2^16); this function returns an int to avoid C type
// promotion insanity.
static int fastcgi_request_id(const FCGI_Record* const r) {
  const int requestid = r->requestIdB1 << 8 | r->requestIdB0;
  assert(requestid >= 0);
  assert(requestid <= UINT16_MAX);
  return requestid;
}

// An output on the connection sock (-1: none yet) for the request
// request_id: the fields of its record that every record shares, and no
// buffer.
static void fastcgi_output_init(FCGI_Output *o, int sock, int request_id) {
  o->sock = sock;
  o->request_id = request_id;
  o->buf = NULL;
  o->r.version = FCGI_VERSION_1;
  o->r.paddingLength = 0;
  o->r.reserved = 0;
}

static FCGI_Output *fastcgi_output() {
  FCGI_Output *o = malloc(sizeof(FCGI_Output));

  fastcgi_output_init(o, -1, 0);

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
  assert(o->request_id <= UINT16_MAX);
  o->r.requestIdB1 = o->request_id >> 8;
  o->r.requestIdB0 = o->request_id & 0x000000ff;
  o->r.contentLengthB1 = contentLength >> 8;
  o->r.contentLengthB0 = contentLength & 255;
  if (o->buf)
    return fastcgi_append(o, (const unsigned char *)&o->r, FCGI_HEADER_SIZE + contentLength);
  return uw_really_send(o->sock, &o->r, FCGI_HEADER_SIZE + contentLength);
}

static FCGI_Record *fastcgi_recv(FCGI_Input *i) {
  if (i->used > 0) {
    memmove(i->bytes, i->bytes + i->used, i->available - i->used);
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

    n = recv(i->sock, i->bytes + i->available, sizeof(i->bytes) - i->available, 0);

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
      o->r.requestIdB1 = o->request_id >> 8;
      o->r.requestIdB0 = o->request_id & 0x000000ff;
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

static int close_stream(FCGI_Output *o, unsigned char type) {
  if (fastcgi_send(o, type, 0)) {
    fprintf(stderr, "fastcgi_send() failed in close_stream().\n");
    return -1;
  }
  return 0;
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
// FCGI_END_REQUEST with the request's status.  0, or -1 when some of it
// could not be sent.
static int fastcgi_end_request(FCGI_Output *out, request_result rr) {
  FCGI_EndRequestBody *erb = (FCGI_EndRequestBody *)out->r.contentData;
  int failed = 0;

  failed |= close_stream(out, FCGI_STDOUT);
  failed |= close_stream(out, FCGI_STDERR);

  if (rr == SERVED)
    erb->appStatusB3 = erb->appStatusB2 = erb->appStatusB1 = erb->appStatusB0 = 0;
  else
    erb->appStatusB3 = erb->appStatusB2 = erb->appStatusB1 = erb->appStatusB0 = 0xFF;

  erb->protocolStatus = FCGI_REQUEST_COMPLETE;
  failed |= fastcgi_send(out, FCGI_END_REQUEST, sizeof(FCGI_EndRequestBody));
  if (out->buf && fastcgi_flush(out)) {
    fprintf(stderr, "fastcgi_flush() failed in fastcgi_end_request().\n");
    failed = -1;
  }
  return failed ? -1 : 0;
}

static int fastcgi_close_with(FCGI_Output *out, request_result rr) {
  fastcgi_end_request(out, rr);
  return close(out->sock);
}

// The request last begun on a connection, by socket.  A request for
// messages (/.msgs) hands its connection to the runtime, which answers on
// it from whichever thread sends the message, through fastcgi_send_normal
// and fastcgi_close, which get the socket alone.  The records they write
// must carry the id the web server gave the request, and the connection
// stays open afterwards if the web server asked for that.  A worker enters
// both before every request, since the handing over happens inside
// uw_request; the entry is read once the connection has been handed over,
// which the runtime orders after the handing over through the client's
// lock.
typedef struct {
  int request_id;
  // FCGI_KEEP_CONN: the web server asked for the connection to stay open
  // after the request.
  int keep;
  // Set while the kept connection, with its next request begun or the web
  // server done with it, waits in the queue for a worker, and taken off by
  // the worker that dequeues it.
  int kept;
} connection;

static connection *connections;
static size_t n_connections;
static pthread_mutex_t connections_mutex = PTHREAD_MUTEX_INITIALIZER;

static void set_handed(int sock, int request_id, int keep) {
  pthread_mutex_lock(&connections_mutex);
  if ((size_t)sock >= n_connections) {
    size_t n = n_connections ? n_connections : 256;

    while ((size_t)sock >= n)
      n *= 2;
    connections = realloc(connections, n * sizeof(connection));
    memset(connections + n_connections, 0, (n - n_connections) * sizeof(connection));
    n_connections = n;
  }
  connections[sock].request_id = request_id;
  connections[sock].keep = keep;
  pthread_mutex_unlock(&connections_mutex);
}

static int handed_id(int sock) {
  int request_id;

  pthread_mutex_lock(&connections_mutex);
  request_id = connections[sock].request_id;
  pthread_mutex_unlock(&connections_mutex);
  return request_id;
}

static int handed_keep(int sock) {
  int keep;

  pthread_mutex_lock(&connections_mutex);
  keep = connections[sock].keep;
  pthread_mutex_unlock(&connections_mutex);
  return keep;
}

// A kept connection, from the poller, to the workers: it waits in the
// queue for one of them, marked as kept.
static void enqueue_kept(int sock) {
  pthread_mutex_lock(&connections_mutex);
  connections[sock].kept = 1;
  pthread_mutex_unlock(&connections_mutex);
  uw_enqueue(sock);
}

// Whether sock, just dequeued, is a kept connection from the poller rather
// than a new one.  The mark is taken off.
static int dequeued_kept(int sock) {
  int r = 0;

  pthread_mutex_lock(&connections_mutex);
  if ((size_t)sock < n_connections) {
    r = connections[sock].kept;
    connections[sock].kept = 0;
  }
  pthread_mutex_unlock(&connections_mutex);
  return r;
}

// The poller.  A kept connection whose response has gone sits idle until
// the web server sends its next request on it or closes it, which may be
// a long time: nginx keeps a connection for 60 seconds by default.  Such
// a connection waits with the poller, one thread for all of them, and
// holds no worker meanwhile; so the web server may keep as many as the
// limit on open files allows, whatever the number of workers.  Whoever is
// done with a kept connection parks it (park): a worker after its
// response, and the thread that answered a request for messages
// (fastcgi_close).  The poller waits on all of them at once with poll(),
// and a connection that has something to read -- the beginning of a
// request, or the web server's end of it -- goes into the workers' queue,
// marked as kept.  A byte down a pipe wakes the poller for a connection
// parked while it waits.  A connection is in one place at a time: with a
// worker, in the queue, or with the poller.
//
// What the poller costs.  poll() looks at every descriptor it is given, and
// each park and each wake-up rebuilds the array and polls again, so every
// request on a kept connection costs about 0.3 to 0.5 microseconds of the
// poller's time for each connection parked.  Measured on two CPUs with two
// workers, one busy kept connection and N idle ones parked: the poller used
// 17 microseconds a request at N = 0, 102 at 300, 459 at 1000 and 1457 at
// 3000, and the busy connection got 9965 requests a second at 0, 9019 at
// 300, 1947 at 1000 and 663 at 3000.  The poller is one thread, so from a
// few hundred parked connections on it limits the program's throughput on
// kept connections, whatever the number of workers.  A set kept by the
// kernel, epoll on Linux and kqueue on the BSDs and macOS, would not depend
// on N: a park would arm the descriptor for one event (epoll_ctl, kevent)
// and the poller would wait for events instead of rebuilding the array.
// That is a backend for each, with this one as the fallback, and is left
// until a deployment keeps hundreds of connections.
static int *parked;
static size_t n_parked, parked_cap;
static pthread_mutex_t parked_mutex = PTHREAD_MUTEX_INITIALIZER;
static int poller_pipe[2];

// The room for parked connections to start with; it doubles from there.
#define PARKED_FIRST 64

static void park(int sock) {
  unsigned char byte = 0;

  pthread_mutex_lock(&parked_mutex);
  if (n_parked == parked_cap) {
    parked_cap = parked_cap ? 2 * parked_cap : PARKED_FIRST;
    parked = realloc(parked, parked_cap * sizeof(int));
  }
  parked[n_parked++] = sock;
  pthread_mutex_unlock(&parked_mutex);
  // A pipe that is full holds wake-ups enough.
  while (write(poller_pipe[1], &byte, 1) < 0 && errno == EINTR);
}

static void *poller(void *data) {
  struct pollfd *fds = NULL;
  size_t fds_cap = 0;
  (void)data;

  while (1) {
    size_t n, i, kept;

    // The pipe first, then the parked connections as they stand.  Only
    // the poller takes connections out, and park adds at the end, so the
    // first n entries are these until they are taken out below.
    pthread_mutex_lock(&parked_mutex);
    if (n_parked + 1 > fds_cap) {
      fds_cap = parked_cap + 1;
      fds = realloc(fds, fds_cap * sizeof(struct pollfd));
    }
    fds[0].fd = poller_pipe[0];
    fds[0].events = POLLIN;
    for (i = 0; i < n_parked; ++i) {
      fds[i + 1].fd = parked[i];
      fds[i + 1].events = POLLIN;
    }
    n = n_parked;
    pthread_mutex_unlock(&parked_mutex);

    if (poll(fds, n + 1, -1) < 0) {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "poll failed in the poller: %s\n", strerror(errno));
      exit(1);
    }

    if (fds[0].revents) {
      // The wake-ups so far; what does not fit leaves the pipe readable.
      unsigned char bytes[PARKED_FIRST];
      while (read(poller_pipe[0], bytes, sizeof bytes) > 0);
    }

    // The connections with something to read go to the workers; the rest
    // stay, moved up over them, and so does whatever was parked meanwhile.
    pthread_mutex_lock(&parked_mutex);
    for (i = 0, kept = 0; i < n; ++i)
      if (!fds[i + 1].revents)
        parked[kept++] = parked[i];
    memmove(parked + kept, parked + n, (n_parked - n) * sizeof(int));
    n_parked -= n - kept;
    pthread_mutex_unlock(&parked_mutex);
    for (i = 0; i < n; ++i)
      if (fds[i + 1].revents)
        enqueue_kept(fds[i + 1].fd);
  }

  return NULL;
}

// The end of the answer to a request for messages, from whichever thread
// sent it.  The connection is then closed, or parked with the poller when
// the web server asked to keep it, as the FastCGI spec requires: it is the
// web server's to close.  One whose end could not be sent is closed.
static int fastcgi_close(int sock) {
  FCGI_Output out;

  fastcgi_output_init(&out, sock, handed_id(sock));
  if (fastcgi_end_request(&out, SERVED) || !handed_keep(sock))
    return close(sock);
  park(sock);
  return 0;
}

int fastcgi_send_normal(int sock, const void *buf, ssize_t len) {
  FCGI_Output out;

  fastcgi_output_init(&out, sock, handed_id(sock));
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
  // request (FCGI_KEEP_CONN).  Once the response has gone, the connection
  // waits with the poller for its next request, and this thread takes the
  // next connection from the queue; unless bytes of that request have
  // come already, when this thread reads on.
  int keep = 0;

  while (1) {
    FCGI_Record *r;
    size_t used_nvps = 0;
    int body_len, body_read;
    char *s;
    char *method, *path, *path_info, *query_string;
    int kept = keep;

    keep = 0;
    if (!kept) {
      in->sock = out->sock = uw_dequeue();
      // A kept connection from the poller, with its next request begun or
      // the web server done with it.
      kept = dequeued_kept(in->sock);
    }

    if (!(r = fastcgi_recv(in))) {
      // A kept connection that the web server closes is simply done with.
      if (!kept)
        fprintf(stderr, "Error receiving initial message\n");
      goto done;
    }

    out->request_id = fastcgi_request_id(r);

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

      if (fastcgi_request_id(r) != out->request_id) {
        log_error(out, "Ignoring environment variables for request %d (current request has id %d)\n",
                  fastcgi_request_id(r), out->request_id);
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

      if (fastcgi_request_id(r) != out->request_id) {
        log_error(out, "Ignoring STDIN for request %d (current request has id %d)\n",
                  fastcgi_request_id(r), out->request_id);
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

      set_handed(in->sock, out->request_id, keep);
      rr = uw_request(rc, ctx, method, path, query_string, body, body_read,
                      on_success, on_failure,
                      out, log_error, log_debug,
                      in->sock, fastcgi_send_normal, fastcgi_close);

      // A request for messages has handed its connection over.  Whoever
      // sends the messages ends it with fastcgi_close, which gives a kept
      // one back to the workers.
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
        // Parked, unless the web server has sent on already: what has
        // come of the next request is in this thread's buffer, so this
        // thread reads on.
        if (in->available == in->used) {
          park(in->sock);
          keep = 0;
        }
      } else
        fastcgi_close_with(out, rr);
      out->buf = NULL;
      goto done2;
    }

  done:
    keep = 0;
    close(in->sock);
  done2:
    // What has come of the next request on a kept connection this thread
    // reads on stays.
    if (!keep)
      fastcgi_input_reset(in);
    uw_reset(ctx);
    uw_heap_shrink(ctx);
  }

  return NULL;
}

static void help(char *cmd) {
  printf("Usage: %s [-t <thread-count>] [-q]\n"
         "The '-q' option turns off the debug output on stdout.\n", cmd);
}

static uw_loggers ls = {NULL, log_error, log_debug};

// How long the accept loop waits between attempts, in milliseconds, once
// accept has failed with an error that may pass: at first
// ACCEPT_WAIT_FIRST_MS, then twice as long each time, up to
// ACCEPT_WAIT_LAST_MS.
#define ACCEPT_WAIT_FIRST_MS 5
#define ACCEPT_WAIT_LAST_MS 1000

// A failed accept: whether the accept loop is to go on.  It stops only when
// the listening socket itself is unusable (EBADF, ENOTSOCK, EINVAL, EFAULT):
// no accept can succeed then, and trying again would spin.  A connection that
// was dropped before accept got to it (ECONNABORTED, EPROTO) or a signal
// (EINTR) is tried again at once.  Anything else is waited out: the program
// being out of descriptors or memory (EMFILE, ENFILE, ENOBUFS, ENOMEM), which
// passes as connections end, and the errors that Linux reports through accept
// for a connection in the listen queue (ENETDOWN, ENETUNREACH, EHOSTDOWN,
// EHOSTUNREACH, ENONET, ENOPROTOOPT, EOPNOTSUPP, EPERM for a firewall rule,
// ETIMEDOUT, ...), which accept(2) says to treat as EAGAIN.  EOPNOTSUPP is
// also what accept says on a socket that is not a stream; such a listener
// gets the line and the waits, not an exit.  The loop waits for *wait_ms
// milliseconds before it tries again, 0 meaning that the last accept worked,
// and a line goes to stderr once for each spell of failures.  No kept
// connection is closed to make room: closing one is for the web server to
// decide.
static int accept_failed(int error, int *wait_ms) {
  struct timespec delay;

  if (error == EINTR || error == ECONNABORTED || error == EPROTO)
    return 1;

  if (error == EBADF || error == ENOTSOCK || error == EINVAL || error == EFAULT) {
    fprintf(stderr, "Socket accept failed: %s\n", strerror(error));
    return 0;
  }

  if (!*wait_ms) {
    *wait_ms = ACCEPT_WAIT_FIRST_MS;
    fprintf(stderr, "Socket accept failed: %s; %s\n", strerror(error),
            error == EMFILE || error == ENFILE ? "waiting for a descriptor"
            : error == ENOBUFS || error == ENOMEM ? "waiting for memory"
            : "trying again");
  }
  delay.tv_sec = *wait_ms / 1000;
  delay.tv_nsec = (*wait_ms % 1000) * 1000000L;
  nanosleep(&delay, NULL);
  *wait_ms = *wait_ms < ACCEPT_WAIT_LAST_MS / 2 ? 2 * *wait_ms : ACCEPT_WAIT_LAST_MS;
  return 1;
}

int main(int argc, char *argv[]) {
  // The skeleton for this function comes from Beej's sockets tutorial.
  // The connector's address: room for any family's, a Unix socket's path
  // and an IPv6 address included.
  struct sockaddr_storage their_addr;
  socklen_t sin_size;
  int nthreads = 1, i, *names, opt, accept_wait_ms = 0;
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

  while ((opt = getopt(argc, argv, "hqt:")) != -1) {
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

  {
    pthread_t thread;

    if (pipe(poller_pipe) || fcntl(poller_pipe[0], F_SETFL, O_NONBLOCK) || fcntl(poller_pipe[1], F_SETFL, O_NONBLOCK)) {
      fprintf(stderr, "Error creating the poller's pipe\n");
      return 1;
    }
    if (pthread_create(&thread, NULL, poller, NULL)) {
      fprintf(stderr, "Error creating poller thread\n");
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
    int new_fd;

    // accept leaves the length of the address in sin_size, which may be more
    // than the buffer holds when the address was cut short; each call is
    // given the buffer's size afresh.
    sin_size = sizeof their_addr;
    new_fd = accept(FCGI_LISTENSOCK_FILENO, (struct sockaddr *)&their_addr, &sin_size);

    if (new_fd < 0) {
      if (accept_failed(errno, &accept_wait_ms))
        continue;
      return 1;
    }
    accept_wait_ms = 0;

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
