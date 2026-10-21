#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <time.h>
#include <math.h>

#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>

#include <unicode/utf8.h>
#include <unicode/uchar.h>

#include "types.h"

#include "uthash.h"

// Defined with the context below, used before it.
void uw_log_error(uw_context ctx, const char *fmt, ...);

uw_unit uw_unit_v = 0;


// Socket extras

int uw_really_send(int sock, const void *buf, ssize_t len) {
  while (len > 0) {
    ssize_t n = send(sock, buf, len, 0);

    if (n < 0)
      return n;

    buf += n;
    len -= n;
  }

  return 0;
}

int uw_really_write(int fd, const void *buf, size_t len) {
  while (len > 0) {
    ssize_t n = write(fd, buf, len);

    if (n < 0)
      return n;

    buf += n;
    len -= n;
  }

  return 0;
}


// Buffers

void uw_buffer_init(size_t max, uw_buffer *b, size_t s) {
  b->max = max;
  b->front = b->start = malloc(s);
  b->back = b->front + s;
}

void uw_buffer_free(uw_buffer *b) {
  free(b->start);
}

void uw_buffer_reset(uw_buffer *b) {
  b->front = b->start;
  if (b->front != b->back) {
    *b->front = 0;
  }
}

int uw_buffer_check(uw_buffer *b, size_t extra) {
  if (b->back - b->front < extra) {
    size_t desired = b->front - b->start + extra, next;
    char *new_heap;

    next = b->back - b->start;
    if (next == 0)
      next = 1;
    for (; next < desired; next *= 2);

    if (next > b->max) {
      if (desired <= b->max)
        next = desired;
      else
        return 1;
    }

    new_heap = realloc(b->start, next);
    b->front = new_heap + (b->front - b->start);
    b->back = new_heap + next;
    b->start = new_heap;
  }

  return 0;
}

__attribute__((noreturn)) void uw_error(uw_context, failure_kind, const char *, ...);

static void ctx_uw_buffer_check(uw_context ctx, const char *kind, uw_buffer *b, size_t extra) {
  if (uw_buffer_check(b, extra))
    uw_error(ctx, FATAL, "Memory limit exceeded (%s)", kind);
}

size_t uw_buffer_used(uw_buffer *b) {
  return b->front - b->start;
}

size_t uw_buffer_avail(uw_buffer *b) {
  return b->back - b->start;
}

int uw_buffer_append(uw_buffer *b, const char *s, size_t len) {
  if (uw_buffer_check(b, len+1))
    return 1;

  memcpy(b->front, s, len);
  b->front += len;
  *b->front = 0;

  return 0;
}

static void ctx_uw_buffer_append(uw_context ctx, const char *kind, uw_buffer *b, const char *s, size_t len) {
  ctx_uw_buffer_check(ctx, kind, b, len+1);

  memcpy(b->front, s, len);
  b->front += len;
  *b->front = 0;
}


// Persistent state types

typedef enum { UNUSED, USED } usage;

typedef struct client {
  unsigned id;
  usage mode;
  int pass;
  struct client *next;
  pthread_mutex_t lock, pull_lock;
  uw_buffer msgs;
  int sock;
  int (*send)(int sockfd, const void *buf, ssize_t len);
  int (*close)(int fd);
  time_t last_contact;
  unsigned n_channels;
  unsigned refcount;
  void *data;
} client;


// Persistent client state

static client **clients, *clients_free, *clients_used;
static unsigned n_clients;

static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
size_t uw_messages_max = SIZE_MAX;
size_t uw_clients_max = SIZE_MAX;

void *uw_init_client_data();
void uw_free_client_data(void *);
void uw_copy_client_data(void *dst, void *src);

// reproducible.c
extern void uw_reproducible_init(void);
extern int uw_reproducible_epoch(int64_t *);
extern int uw_reproducible_rand(uint64_t *);

// dmllog.c
extern void uw_dml_log_init(void);

static int my_rand(uw_context ctx) {
  unsigned int ret;
  uint64_t pinned;
  if (uw_reproducible_rand(&pinned))
    return pinned >> 33; // as below: 31 bits
  if (RAND_bytes((unsigned char *)&ret, sizeof ret)) {
    ret >>= 1; // clear top bit
    return ret;
  } else
    uw_error(ctx, FATAL, "Random number generation failed");
}

static client *new_client(uw_context ctx) {
  client *c;
  int pass = my_rand(ctx);

  pthread_mutex_lock(&clients_mutex);

  if (clients_free) {
    c = clients_free;
    clients_free = clients_free->next;
  }
  else if (n_clients >= uw_clients_max) {
    pthread_mutex_unlock(&clients_mutex);
    return NULL;
  } else {
    ++n_clients;
    clients = realloc(clients, sizeof(client) * n_clients);
    c = malloc(sizeof(client));
    c->id = n_clients-1;
    pthread_mutex_init(&c->lock, NULL);
    pthread_mutex_init(&c->pull_lock, NULL);
    uw_buffer_init(uw_messages_max, &c->msgs, 0);
    clients[n_clients-1] = c;
  }

  pthread_mutex_lock(&c->lock);
  c->mode = USED;
  c->pass = pass;
  c->sock = -1;
  c->last_contact = time(NULL);
  uw_buffer_reset(&c->msgs);
  c->n_channels = 0;
  c->refcount = 0;
  c->data = uw_init_client_data();
  pthread_mutex_unlock(&c->lock);

  c->next = clients_used;
  clients_used = c;

  pthread_mutex_unlock(&clients_mutex);

  return c;
}

static void use_client(client *c) {
  pthread_mutex_lock(&c->lock);
  ++c->refcount;
  pthread_mutex_unlock(&c->lock);
  pthread_mutex_lock(&c->pull_lock);
}

static void release_client(client *c) {
  pthread_mutex_unlock(&c->pull_lock);
  pthread_mutex_lock(&c->lock);
  --c->refcount;
  pthread_mutex_unlock(&c->lock);
}

static const char begin_msgs[] = "Content-type: text/plain\r\n\r\n";
// Set while this thread is the pruner and holds clients_mutex: an expunge
// looks clients up, and the mutex is not recursive.  Per thread, so that no
// other thread reads it while the pruner writes it.
static _Thread_local int pruning = 0;

static client *find_client(unsigned id) {
  client *c;
  int i_am_pruner = pruning;

  if (!i_am_pruner) pthread_mutex_lock(&clients_mutex);

  if (id >= n_clients) {
    if (!i_am_pruner) pthread_mutex_unlock(&clients_mutex);
    return NULL;
  }

  c = clients[id];

  if (!i_am_pruner) pthread_mutex_unlock(&clients_mutex);
  return c;
}

static char *on_success = "HTTP/1.1 200 OK\r\n";
static char *on_redirect = "HTTP/1.1 303 See Other\r\n";

void uw_set_on_success(char *s) {
  on_success = s;
}

static void chastise(int (*send)(int sockfd, const void *buf, ssize_t len), int (*close)(int fd), int sock) {
  send(sock, on_success, strlen(on_success));
  send(sock, begin_msgs, sizeof(begin_msgs) - 1);
  send(sock, "R", 1);
  close(sock);
}

void uw_client_connect(uw_context ctx, unsigned id, int pass, int sock,
                       int (*send)(int sockfd, const void *buf, ssize_t len),
                       int (*close)(int fd)) {
  client *c = find_client(id);

  if (c == NULL) {
    uw_log_error(ctx, "Out-of-bounds client request (%u)\n", id);
    chastise(send, close, sock);
    return;
  }

  pthread_mutex_lock(&c->lock);

  if (c->mode != USED) {
    pthread_mutex_unlock(&c->lock);
    uw_log_error(ctx, "Client request for unused slot (%u)\n", id);
    chastise(send, close, sock);
    return;
  }

  if (pass != c->pass) {
    pthread_mutex_unlock(&c->lock);
    uw_log_error(ctx, "Wrong client password (%u, %d)\n", id, pass);
    chastise(send, close, sock);
    return;
  }

  if (c->sock != -1) {
    c->close(c->sock);
    c->sock = -1;
  }

  c->last_contact = time(NULL);

  if (uw_buffer_used(&c->msgs) > 0) {
    send(sock, on_success, strlen(on_success));
    send(sock, begin_msgs, sizeof(begin_msgs) - 1);
    send(sock, c->msgs.start, uw_buffer_used(&c->msgs));
    uw_buffer_reset(&c->msgs);
    close(sock);
  }
  else {
    c->sock = sock;
    c->send = send;
    c->close = close;
  }

  pthread_mutex_unlock(&c->lock);
}

static void free_client(client *c) {
  // A poll still held for the client is answered as any poll for a client
  // that is gone (chastise), and its connection goes back to the protocol,
  // which closes it, or keeps it if the web server asked. Once the client
  // is free, nothing else would answer it or close it.
  if (c->sock != -1) {
    chastise(c->send, c->close, c->sock);
    c->sock = -1;
  }

  c->mode = UNUSED;
  c->pass = -1;

  c->next = clients_free;
  clients_free = c;
}

static uw_Basis_channel new_channel(client *c) {
  uw_Basis_channel ch = {c->id, c->n_channels++};
  return ch;
}

static void client_send(uw_context ctx, client *c, uw_buffer *msg, const char *script, int script_len) {
  pthread_mutex_lock(&c->lock);

  if (c->mode != USED) { }
  else if (c->sock != -1) {
    c->send(c->sock, on_success, strlen(on_success));
    c->send(c->sock, begin_msgs, sizeof(begin_msgs) - 1);
    if (script_len > 0) {
      c->send(c->sock, "E\n", 2);
      c->send(c->sock, script, script_len);
      c->send(c->sock, "\n", 1);
    }
    c->send(c->sock, msg->start, uw_buffer_used(msg));
    c->close(c->sock);
    c->sock = -1;
  } else if (uw_buffer_append(&c->msgs, msg->start, uw_buffer_used(msg)))
    uw_log_error(ctx, "Client message buffer size exceeded\n");

  pthread_mutex_unlock(&c->lock);
}


// Global entry points

extern void uw_global_custom();
extern void uw_init_crypto();

static void uw_heap_from_env();
static void uw_heap_init(uw_context ctx);
static void uw_heap_spare(uw_context ctx);
static void uw_heap_moving_back(uw_context ctx, char *to);

void uw_global_init() {
  clients = malloc(0);

  uw_global_custom();
  uw_init_crypto();
  uw_reproducible_init();
  uw_dml_log_init();
  uw_heap_from_env();

  // Fast non-cryptographic strength randomness for Sqlcache.
  srandom(clock());
}

void uw_app_init(uw_app *app) {
  app->client_init();
}

int uw_time = 0, uw_time_max = 0, uw_min_heap = 0;


// Single-request state

typedef struct regions {
  struct regions *next;
} regions;

typedef struct {
  void (*func)(void*);
  void *arg;
} cleanup;

typedef struct {
  unsigned client;
  uw_buffer msgs;
} delta;

typedef enum {
  UNSET, NORMAL, FIL, SUBFORM, SUBFORMS, ENTRY
} input_kind;

typedef struct input {
  input_kind kind;
  union {
    char *normal;
    uw_Basis_file file;
    struct {
      struct input *fields, *parent;
    } subform;
    struct {
      struct input *entries, *parent;
    } subforms;
    struct {
      struct input *fields, *next, *parent;
    } entry;
  } data;
} input;

typedef struct {
  void *data;
  uw_callback commit, rollback;
  uw_callback_with_retry free;
} transactional;

typedef struct {
  char *name;
  void *data;
  void (*free)(void*);
} global;

typedef struct uw_Sqlcache_Update {
  uw_Sqlcache_Cache *cache;
  char **keys;
  uw_Sqlcache_Value *value;
  struct uw_Sqlcache_Update *next;
} uw_Sqlcache_Update;

typedef struct uw_Sqlcache_Unlock {
  uw_Sqlcache_Lock *lock;
  int write;
  struct uw_Sqlcache_Unlock *next;
} uw_Sqlcache_Unlock;

struct uw_context {
  uw_app *app;
  int id;

  char *(*get_header)(void *, const char *);
  void *get_header_data;

  char *(*get_env)(void *, const char *);
  void *get_env_data;

  uw_buffer outHeaders, page, heap, script;
  // The heap is a reservation of address space that never moves (see "The
  // heap" below): heap_reserved bytes mapped at heap.start, of which the
  // first heap_committed have been made readable and writable, and the
  // first heap.back - heap.start may be used now; heap.max is the heap
  // limit.  heap_peak is the most the work going on has had in use at
  // once, and heap_dropped how much it has given back by moving heap.front
  // back, both through all its attempts; heap_recent is what the heap keeps
  // between pieces of work (uw_heap_shrink).
  size_t heap_reserved, heap_committed, heap_peak, heap_dropped, heap_recent;
  int allowed_to_return_indirectly, returning_indirectly;
  input *inputs, *subinputs, *cur_container;
  size_t sz_inputs, n_subinputs, used_subinputs;

  unsigned long long source_count;

  void *db;
  int transaction_started;
  // Whether the last uw_commit got its transaction into the database.  When
  // uw_commit returns 0 with an error, the error came before COMMIT or from
  // it, and nothing was written, or after it, from a commit callback, and
  // everything was.
  int committed;

  // Set by uw_io_prepare: the context runs io computations (uw_runIo).
  int io;
  // Why the last transaction of an io computation failed, for
  // Basis.io_errorMessage; empty when it committed.  Apart from
  // error_message, which says whether the code running now has failed.
  char io_error_message[ERROR_BUF_LEN];
  // The transactionals and cleanups below these marks were registered by the
  // io computation around the transaction running now, not by it: the
  // transaction's commit, rollback and errors leave them alone.  Zero outside
  // an io transaction.
  size_t transactionals_base;
  size_t cleanup_base;
  // A request is running an io function (uw_io_request_begin): its client
  // is kept across the function's transactions, for the request to release.
  int io_request;

  jmp_buf jmp_buf;

  regions *regions;

  cleanup *cleanup, *cleanup_front, *cleanup_back;

  const char *script_header;

  int needs_push, needs_sig, could_write_db, at_most_one_query;

  // Whether the transaction about to begin may wait for a lock on the
  // database that another connection holds (uw_db_may_wait).
  int db_may_wait;

  size_t n_deltas, used_deltas;
  delta *deltas;

  client *client;

  transactional *transactionals;
  size_t n_transactionals, used_transactionals;

  global *globals;
  size_t n_globals;

  char *current_url;

  int deadline;

  void *client_data;

  uw_loggers *loggers;

  // What the context is doing, for its error lines: "start-up", a
  // request's method and path, a task, expunging a client.
  char activity[256];

  int isPost, hasPostBody;
  uw_Basis_postBody postBody;
  uw_Basis_string queryString;

  unsigned nextId;

  int amInitializing;

  char error_message[ERROR_BUF_LEN];

  int usedSig, needsResig;

  char *output_buffer;
  size_t output_buffer_size;

  // Sqlcache.
  int numRecording, recordingCapacity;
  int *recordingOffsets, *scriptRecordingOffsets;
  uw_Sqlcache_Update *cacheUpdate;
  uw_Sqlcache_Update *cacheUpdateTail;
  uw_Sqlcache_Unlock *cacheUnlock;

  int remoteSock;

  int file_cache_missed;
  // Set if we are recovering from a miss in the file cache in handling an SQL
  // query that only returns hashes of files.  If so, this time around we will
  // run queries to return actual file contents instead.
};

size_t uw_headers_max = SIZE_MAX;
size_t uw_page_max = SIZE_MAX;
size_t uw_heap_max = SIZE_MAX;
size_t uw_script_max = SIZE_MAX;

// URWEB_LIMIT_HEAP and URWEB_MIN_HEAP: the heap's limit and the size it
// starts at, in bytes, in place of the .urp's `limit heap` and minHeap, so
// that a deployment can change them without compiling again.  Read after
// uw_global_custom, which has set both from the .urp.  The size a heap
// starts at stays below its limit, wherever each came from: a program
// that would start above it is refused, rather than started with a
// smaller heap than it asked for or a larger one than its limit.

// A number of bytes from the environment variable name, into *n; 0 if it
// is not set (or empty).  Anything but digits is refused.
static int uw_bytes_from_env(const char *name, unsigned long long *n) {
  char *s = getenv(name), *end;

  if (!s || !*s)
    return 0;

  errno = 0;
  *n = strtoull(s, &end, 10);
  if (errno || end == s || *end || strspn(s, "0123456789") != strlen(s)) {
    fprintf(stderr, "%s: '%s' is not a number of bytes\n", name, s);
    exit(1);
  }
  return 1;
}

static void uw_heap_from_env() {
  const char *limit_from = "limit heap in the .urp", *min_from = "minHeap in the .urp";
  unsigned long long n;

  if (uw_bytes_from_env("URWEB_LIMIT_HEAP", &n)) {
    // SIZE_MAX itself is what uw_heap_max holds for no limit at all.
    if (n == 0 || n >= SIZE_MAX) {
      fprintf(stderr, "URWEB_LIMIT_HEAP: %llu is no limit for a heap: it has to be between 1 and %zu\n",
              n, (size_t)SIZE_MAX - 1);
      exit(1);
    }
    uw_heap_max = (size_t)n;
    limit_from = "URWEB_LIMIT_HEAP";
  }

  if (uw_bytes_from_env("URWEB_MIN_HEAP", &n)) {
    if (n > INT_MAX) {
      fprintf(stderr, "URWEB_MIN_HEAP: %llu is more than a heap can start at, %d\n", n, INT_MAX);
      exit(1);
    }
    uw_min_heap = (int)n;
    min_from = "URWEB_MIN_HEAP";
  }

  if ((size_t)uw_min_heap > uw_heap_max) {
    fprintf(stderr, "The heap would start at %d bytes (%s), more than its limit of %zu (%s)\n",
            uw_min_heap, min_from, uw_heap_max, limit_from);
    exit(1);
  }
}

void uw_set_activity(uw_context ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(ctx->activity, sizeof ctx->activity, fmt, ap);
  va_end(ap);
}

void uw_log_error(uw_context ctx, const char *fmt, ...) {
  char small[1024], *msg = small;
  va_list ap, again;
  int n;

  va_start(ap, fmt);
  va_copy(again, ap);
  n = vsnprintf(small, sizeof small, fmt, ap);
  if (n >= (int)sizeof small && (msg = malloc(n + 1)))
    vsnprintf(msg, n + 1, fmt, again);
  else if (!msg)
    msg = small;   // what fitted
  va_end(again);
  va_end(ap);
  ctx->loggers->log_error(ctx->loggers->logger_data, "%s: %s", ctx->activity, n < 0 ? fmt : msg);
  if (msg != small)
    free(msg);
}

uw_context uw_init(int id, uw_loggers *lg) {
  uw_context ctx = malloc(sizeof(struct uw_context));

  ctx->app = NULL;
  ctx->id = id;
  strcpy(ctx->activity, "start-up");

  ctx->get_header = NULL;
  ctx->get_header_data = NULL;

  ctx->get_env = NULL;
  ctx->get_env_data = NULL;

  uw_buffer_init(uw_headers_max, &ctx->outHeaders, 1);
  ctx->outHeaders.start[0] = 0;
  uw_buffer_init(uw_page_max, &ctx->page, 1);
  ctx->page.start[0] = 0;
  ctx->allowed_to_return_indirectly = ctx->returning_indirectly = 0;
  uw_heap_init(ctx);
  uw_buffer_init(uw_script_max, &ctx->script, 1);
  ctx->script.start[0] = 0;

  ctx->inputs = malloc(0);
  ctx->cur_container = NULL;
  ctx->subinputs = malloc(0);
  ctx->sz_inputs = ctx->n_subinputs = ctx->used_subinputs = 0;

  ctx->db = NULL;
  ctx->transaction_started = 0;
  ctx->committed = 0;
  ctx->io = 0;
  ctx->io_error_message[0] = 0;
  ctx->transactionals_base = 0;
  ctx->cleanup_base = 0;
  ctx->io_request = 0;

  ctx->regions = NULL;

  ctx->cleanup_front = ctx->cleanup_back = ctx->cleanup = malloc(0);

  ctx->script_header = "";
  ctx->needs_push = 0;
  ctx->needs_sig = 0;
  ctx->could_write_db = 1;
  ctx->at_most_one_query = 0;
  ctx->db_may_wait = 1;

  ctx->source_count = 0;

  ctx->n_deltas = ctx->used_deltas = 0;
  ctx->deltas = malloc(0);

  ctx->client = NULL;

  ctx->error_message[0] = 0;

  ctx->transactionals = malloc(0);
  ctx->n_transactionals = ctx->used_transactionals = 0;

  ctx->globals = malloc(0);
  ctx->n_globals = 0;

  ctx->current_url = "";

  ctx->deadline = INT_MAX;

  ctx->client_data = uw_init_client_data();

  ctx->loggers = lg;

  ctx->isPost = ctx->hasPostBody = 0;

  ctx->queryString = NULL;

  ctx->nextId = 0;

  ctx->amInitializing = 0;

  ctx->usedSig = 0;
  ctx->needsResig = 0;

  ctx->output_buffer = malloc(1);
  ctx->output_buffer_size = 1;

  ctx->numRecording = 0;
  ctx->recordingCapacity = 0;
  ctx->recordingOffsets = malloc(0);
  ctx->scriptRecordingOffsets = malloc(0);
  ctx->cacheUpdate = NULL;
  ctx->cacheUpdateTail = NULL;

  ctx->remoteSock = -1;

  ctx->cacheUnlock = NULL;

  ctx->file_cache_missed = 0;

  return ctx;
}

size_t uw_inputs_max = SIZE_MAX;

uw_app *uw_get_app(uw_context ctx) {
  return ctx->app;
}

int uw_set_app(uw_context ctx, uw_app *app) {
  ctx->app = app;

  if (app && app->inputs_len > ctx->sz_inputs) {
    if (app->inputs_len > uw_inputs_max)
      return 1;

    ctx->sz_inputs = app->inputs_len;
    ctx->inputs = realloc(ctx->inputs, ctx->sz_inputs * sizeof(input));
    memset(ctx->inputs, 0, ctx->sz_inputs * sizeof(input));
  }

  return 0;
}

void uw_set_client_data(uw_context ctx, void *data) {
  uw_copy_client_data(ctx->client_data, data);
}

void uw_set_db(uw_context ctx, void *db) {
  ctx->db = db;
}

void *uw_get_db(uw_context ctx) {
  return ctx->db;
}


uw_loggers* uw_get_loggers(struct uw_context *ctx) {
  return ctx->loggers;
}

void uw_free(uw_context ctx) {
  size_t i;

  uw_buffer_free(&ctx->outHeaders);
  uw_buffer_free(&ctx->script);
  uw_buffer_free(&ctx->page);
  uw_heap_spare(ctx);
  free(ctx->inputs);
  free(ctx->subinputs);
  free(ctx->cleanup);
  free(ctx->transactionals);
  uw_free_client_data(ctx->client_data);

  for (i = 0; i < ctx->n_deltas; ++i)
    uw_buffer_free(&ctx->deltas[i].msgs);
  free(ctx->deltas);

  for (i = 0; i < ctx->n_globals; ++i)
    if (ctx->globals[i].free)
      ctx->globals[i].free(ctx->globals[i].data);
  free(ctx->globals);

  free(ctx->output_buffer);

  free(ctx->recordingOffsets);
  free(ctx->scriptRecordingOffsets);

  free(ctx);
}

// Forget what the transaction running now holds: its messages, its client
// unless it is the request's (see release_client_of), its container, and
// its cleanups and transactionals, those above the marks.  For a request
// the marks are zero and the client is its own, so this is all of it.
static void uw_drop_transaction_state(uw_context ctx) {
  ctx->used_deltas = 0;
  if (!ctx->io_request)
    ctx->client = NULL;
  ctx->cur_container = NULL;
  ctx->cleanup_front = ctx->cleanup + ctx->cleanup_base;
  ctx->used_transactionals = ctx->transactionals_base;
}

void uw_reset_keep_error_message(uw_context ctx) {
  uw_buffer_reset(&ctx->outHeaders);
  uw_buffer_reset(&ctx->script);
  ctx->script.start[0] = 0;
  uw_buffer_reset(&ctx->page);
  ctx->allowed_to_return_indirectly = ctx->returning_indirectly = 0;
  uw_heap_moving_back(ctx, ctx->heap.start);
  uw_buffer_reset(&ctx->heap);
  ctx->regions = NULL;
  ctx->cleanup_base = 0;
  ctx->transactionals_base = 0;
  ctx->io_request = 0;
  uw_drop_transaction_state(ctx);
  ctx->io_error_message[0] = 0;
  ctx->script_header = "";
  ctx->queryString = NULL;
  ctx->nextId = 0;
  ctx->amInitializing = 0;
  ctx->usedSig = 0;
  ctx->needsResig = 0;
  ctx->remoteSock = -1;
  ctx->numRecording = 0;
}

void uw_reset_keep_request(uw_context ctx) {
  uw_reset_keep_error_message(ctx);
  ctx->error_message[0] = 0;
}

void uw_reset(uw_context ctx) {
  uw_reset_keep_request(ctx);
  if (ctx->app)
    memset(ctx->inputs, 0, ctx->app->inputs_len * sizeof(input));
  memset(ctx->subinputs, 0, ctx->n_subinputs * sizeof(input));
  ctx->used_subinputs = ctx->hasPostBody = ctx->isPost = 0;
  ctx->transaction_started = 0;
}

failure_kind uw_begin_init(uw_context ctx) {
  int r = setjmp(ctx->jmp_buf);

  if (r == 0 && ctx->app)
    ctx->app->db_init(ctx);

  return r;
}

void uw_close(uw_context ctx) {
  ctx->app->db_close(ctx);
}

uw_Basis_string uw_Basis_requestHeader(uw_context ctx, uw_Basis_string h) {
  if (ctx->get_header)
    return ctx->get_header(ctx->get_header_data, h);
  else
    return NULL;
}

void uw_set_headers(uw_context ctx, char *(*get_header)(void *, const char *), void *get_header_data) {
  ctx->get_header = get_header;
  ctx->get_header_data = get_header_data;
}

void uw_set_env(uw_context ctx, char *(*get_env)(void *, const char *), void *get_env_data) {
  ctx->get_env = get_env;
  ctx->get_env_data = get_env_data;
}

static void uw_set_error(uw_context ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vsnprintf(ctx->error_message, ERROR_BUF_LEN, fmt, ap);
}

int uw_has_error(uw_context ctx) {
  return ctx->error_message[0] != 0;
}

// Run the cleanups the transaction registered, those above the mark
// (cleanup_base, which is where the transaction began; zero but for a
// transaction of io code), and take them off the stack.
static void uw_run_cleanups(uw_context ctx) {
  cleanup *cl;

  for (cl = ctx->cleanup + ctx->cleanup_base; cl < ctx->cleanup_front; ++cl)
    cl->func(cl->arg);

  ctx->cleanup_front = ctx->cleanup + ctx->cleanup_base;
}

// uw_error with the message already in place, as uw_set_error_message puts
// it there: a failure found in one place (a callback, a check) is raised in
// another without copying the message aside first.
__attribute__((noreturn)) void uw_reraise(uw_context ctx, failure_kind fk) {
  uw_run_cleanups(ctx);

  longjmp(ctx->jmp_buf, fk);
}

__attribute__((noreturn)) void uw_error(uw_context ctx, failure_kind fk, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vsnprintf(ctx->error_message, ERROR_BUF_LEN, fmt, ap);

  uw_reraise(ctx, fk);
}

size_t uw_cleanup_max = SIZE_MAX;

void uw_push_cleanup(uw_context ctx, void (*func)(void *), void *arg) {
  if (ctx->cleanup_front >= ctx->cleanup_back) {
    int len = ctx->cleanup_back - ctx->cleanup, newLen;
    if (len == 0)
      newLen = 1;
    else
      newLen = len * 2;

    if (newLen > uw_cleanup_max) {
      if (len+1 <= uw_cleanup_max)
        newLen = uw_cleanup_max;
      else
        uw_error(ctx, FATAL, "Exceeded limit on number of cleanup handlers");
    }

    ctx->cleanup = realloc(ctx->cleanup, newLen * sizeof(cleanup));
    ctx->cleanup_front = ctx->cleanup + len;
    ctx->cleanup_back = ctx->cleanup + newLen;
  }

  ctx->cleanup_front->func = func;
  ctx->cleanup_front->arg = arg;
  ++ctx->cleanup_front;
}

char *uw_Basis_htmlifyString(uw_context, const char *);

void uw_login(uw_context ctx) {
  char *id_s, *pass_s;

  if ((id_s = uw_Basis_requestHeader(ctx, "UrWeb-Client"))
      && (pass_s = uw_Basis_requestHeader(ctx, "UrWeb-Pass"))) {
    unsigned id = atoi(id_s);
    int pass = atoi(pass_s);
    client *c = find_client(id);

    if (c == NULL)
      uw_error(ctx, FATAL, "Unknown client ID in HTTP headers (%s, %s)", uw_Basis_htmlifyString(ctx, id_s), uw_Basis_htmlifyString(ctx, pass_s));
    else {
      use_client(c);
      ctx->client = c;

      if (c->mode != USED)
        uw_error(ctx, FATAL, "Stale client ID (%u) in subscription request", id);
      if (c->pass != pass)
        uw_error(ctx, FATAL, "Wrong client password (%u, %d) in subscription request", id, pass);
    }
  } else if (ctx->needs_push) {
    client *c = new_client(ctx);

    if (c == NULL)
      uw_error(ctx, FATAL, "Limit exceeded on number of message-passing clients");

    use_client(c);
    uw_copy_client_data(c->data, ctx->client_data);
    ctx->client = c;
  }
}

// It gets particularly hard to avoid concurrency anomalies in connection with
// the expunger thread, which garbage-collects unused clients, so that their IDs
// can be reused.  To save ourselves from needing to deal with those anomalies,
// we enforce that the expunger never runs simultaneously with other transactions.
//
// The lock is a gate: a count of the transactions in progress and a flag for
// the expunger, the client pruner, of which there is one.  A transaction waits
// while the expunger runs, never while it waits: the expunger goes in once no
// transaction is in progress, and others may arrive in the meantime.  A
// pthread_rwlock_t leaves that to the platform, and where a waiting writer
// goes first, as on macOS, every request that came while the expunger waited
// for a long one (an io RPC holds the lock across its body) waited with it.
// So, as with glibc's rwlock, the expunger may wait long under constant load.
// Every arrival must be matched by one departure.

static pthread_mutex_t expunge_gate = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t expunger_gone = PTHREAD_COND_INITIALIZER;
static pthread_cond_t transactions_gone = PTHREAD_COND_INITIALIZER;
static unsigned transactions_in_progress = 0;
static int expunger_running = 0;

void uw_transaction_arrives() {
  pthread_mutex_lock(&expunge_gate);
  while (expunger_running)
    pthread_cond_wait(&expunger_gone, &expunge_gate);
  ++transactions_in_progress;
  pthread_mutex_unlock(&expunge_gate);
}

void uw_transaction_departs() {
  pthread_mutex_lock(&expunge_gate);
  if (transactions_in_progress == 0)
    // A departure without an arrival.  Counting it would let the expunger in
    // during the next transaction, so it is only reported.
    fprintf(stderr, "Transaction departed without arriving\n");
  else if (--transactions_in_progress == 0)
    pthread_cond_signal(&transactions_gone);
  pthread_mutex_unlock(&expunge_gate);
}

static void uw_expunger_arrives() {
  pthread_mutex_lock(&expunge_gate);
  while (transactions_in_progress > 0)
    pthread_cond_wait(&transactions_gone, &expunge_gate);
  expunger_running = 1;
  pthread_mutex_unlock(&expunge_gate);
}

static void uw_expunger_departs() {
  pthread_mutex_lock(&expunge_gate);
  expunger_running = 0;
  pthread_cond_broadcast(&expunger_gone);
  pthread_mutex_unlock(&expunge_gate);
}

failure_kind uw_begin(uw_context ctx, char *path) {
  int r = setjmp(ctx->jmp_buf);

  if (r == 0)
    ctx->app->handle(ctx, path);

  return r;
}

static void uw_try_reconnecting(uw_context ctx) {
  // Hm, error starting transaction.
  // Maybe the database server died but has since come back up.
  // Let's try starting from scratch.
  if (ctx->db) {
    ctx->app->db_close(ctx);
    ctx->db = NULL;
  }
  ctx->app->db_init(ctx);
}

void uw_try_reconnecting_and_restarting(uw_context ctx) {
  uw_try_reconnecting(ctx);
  uw_error(ctx, BOUNDED_RETRY, "Restarting transaction after fixing database connection");
}

void uw_ensure_transaction(uw_context ctx) {
  if (!ctx->transaction_started && !ctx->at_most_one_query) {
    if (!ctx->db || ctx->app->db_begin(ctx, ctx->could_write_db)) {
      uw_try_reconnecting(ctx);

      if (ctx->app->db_begin(ctx, ctx->could_write_db))
        uw_error(ctx, FATAL, "Error running SQL BEGIN");
    }

    ctx->transaction_started = 1;
  } else if (ctx->at_most_one_query && !ctx->db)
    uw_try_reconnecting(ctx);
}

uw_Basis_client uw_Basis_self(uw_context ctx) {
  if (ctx->client == NULL)
    uw_error(ctx, FATAL, "Call to Basis.self() from page that has only server-side code");

  return ctx->client->id;
}

void uw_pop_cleanup(uw_context ctx) {
  if (ctx->cleanup_front == ctx->cleanup + ctx->cleanup_base)
    uw_error(ctx, FATAL, "Attempt to pop from empty cleanup action stack");

  --ctx->cleanup_front;
  ctx->cleanup_front->func(ctx->cleanup_front->arg);
}

char *uw_error_message(uw_context ctx) {
  return ctx->error_message;
}

void uw_set_error_message(uw_context ctx, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  
  vsnprintf(ctx->error_message, ERROR_BUF_LEN, fmt, ap);
  ctx->error_message[ERROR_BUF_LEN-1] = 0;
}

static input *INP(uw_context ctx) {
  if (ctx->cur_container == NULL)
    return ctx->inputs;
  else if (ctx->cur_container->kind == SUBFORM)
    return ctx->cur_container->data.subform.fields;
  else if (ctx->cur_container->kind == ENTRY)
    return ctx->cur_container->data.entry.fields;
  else
    uw_error(ctx, FATAL, "INP: Wrong kind (%d, %p)", ctx->cur_container->kind, ctx->cur_container);
}

// If *ptr points into the len inputs at old_start, point it at the same one
// among those at new_start.  The test is on addresses: *ptr may point into
// another object (ctx->inputs), and comparing pointers into different
// objects with < is undefined.
static void adjust_pointer(input **ptr, input *old_start, input *new_start, size_t len) {
  uintptr_t p = (uintptr_t)*ptr, old = (uintptr_t)old_start;

  if (p >= old && p - old < len * sizeof(input))
    *ptr = new_start + (p - old) / sizeof(input);
}

static void adjust_input(input *x, input *old_start, input *new_start, size_t len) {
  switch (x->kind) {
  case SUBFORM:
    adjust_pointer(&x->data.subform.fields, old_start, new_start, len);
    adjust_pointer(&x->data.subform.parent, old_start, new_start, len);
    break;
  case SUBFORMS:
    adjust_pointer(&x->data.subforms.entries, old_start, new_start, len);
    adjust_pointer(&x->data.subforms.parent, old_start, new_start, len);
    break;
  case ENTRY:
    adjust_pointer(&x->data.entry.fields, old_start, new_start, len);
    adjust_pointer(&x->data.entry.next, old_start, new_start, len);
    adjust_pointer(&x->data.entry.parent, old_start, new_start, len);
    break;
  default:
    break;
  }
}

size_t uw_subinputs_max = SIZE_MAX;

static input *check_input_space(uw_context ctx, size_t len) {
  size_t i;
  input *r;

  if (ctx->used_subinputs + len >= ctx->n_subinputs) {
    if (ctx->used_subinputs + len > uw_subinputs_max)
      uw_error(ctx, FATAL, "Exceeded limit on number of subinputs");

    // A new block, not realloc: the pointers into the old block are moved
    // while it is still allocated.  After realloc they would point to freed
    // memory, and any use of them, even a comparison, would be undefined.
    // At least double, since each growth copies and rebases the whole table.
    size_t need = ctx->used_subinputs + len, cap = ctx->n_subinputs * 2;
    input *new_subinputs;

    if (cap < need)
      cap = need;
    new_subinputs = malloc(sizeof(input) * cap);

    if (new_subinputs == NULL)
      uw_error(ctx, FATAL, "Out of memory for subinputs");

    if (ctx->used_subinputs > 0)
      memcpy(new_subinputs, ctx->subinputs, sizeof(input) * ctx->used_subinputs);

    for (i = 0; i < ctx->used_subinputs; ++i)
      adjust_input(&new_subinputs[i], ctx->subinputs, new_subinputs, ctx->used_subinputs);
    for (i = 0; i < ctx->app->inputs_len; ++i)
      adjust_input(&ctx->inputs[i], ctx->subinputs, new_subinputs, ctx->used_subinputs);

    adjust_pointer(&ctx->cur_container, ctx->subinputs, new_subinputs, ctx->used_subinputs);

    free(ctx->subinputs);
    ctx->subinputs = new_subinputs;
    ctx->n_subinputs = cap;
  }

  r = &ctx->subinputs[ctx->used_subinputs];

  for (i = 0; i < len; ++i)
    ctx->subinputs[ctx->used_subinputs++].kind = UNSET;

  return r;
}

int uw_set_input(uw_context ctx, const char *name, char *value) {
  //printf("Input name %s\n", name);

  if (!strcasecmp(name, ".b")) {
    int n = ctx->app->input_num(value);
    input *inps;

    if (n < 0) {
      uw_set_error(ctx, "Bad subform name %s", uw_Basis_htmlifyString(ctx, value));
      return -1;
    }

    if (n >= ctx->app->inputs_len) {
      uw_set_error(ctx, "For subform name %s, index %d is out of range", uw_Basis_htmlifyString(ctx, value), n);
      return -1;
    }

    inps = check_input_space(ctx, ctx->app->inputs_len);

    INP(ctx)[n].kind = SUBFORM;
    INP(ctx)[n].data.subform.parent = ctx->cur_container;
    INP(ctx)[n].data.subform.fields = inps;
    ctx->cur_container = &INP(ctx)[n];
  } else if (!strcasecmp(name, ".e")) {
    input *tmp;

    if (ctx->cur_container == NULL) {
      uw_set_error(ctx, "Unmatched subform closer");
      return -1;
    }

    tmp = ctx->cur_container;
    switch (tmp->kind) {
    case SUBFORM:
      ctx->cur_container = tmp->data.subform.parent;
      tmp->data.subform.parent = NULL;
      break;
    case SUBFORMS:
      ctx->cur_container = tmp->data.subforms.parent;
      tmp->data.subforms.parent = NULL;
      break;
    case ENTRY:
      ctx->cur_container = tmp->data.entry.parent;
      break;
    default:
      uw_set_error(ctx, "uw_set_input: Wrong kind");
      return -1;
    }
  } else if (!strcasecmp(name, ".s")) {
    int n = ctx->app->input_num(value);

    if (n < 0) {
      uw_set_error(ctx, "Bad subforms name %s", uw_Basis_htmlifyString(ctx, value));
      return -1;
    }

    if (n >= ctx->app->inputs_len) {
      uw_set_error(ctx, "For subforms name %s, index %d is out of range", uw_Basis_htmlifyString(ctx, value), n);
      return -1;
    }

    INP(ctx)[n].kind = SUBFORMS;
    INP(ctx)[n].data.subforms.parent = ctx->cur_container;
    INP(ctx)[n].data.subforms.entries = NULL;
    ctx->cur_container = &INP(ctx)[n];
  } else if (!strcasecmp(name, ".i")) {
    input *inps;

    if (!ctx->cur_container) {
      uw_set_error(ctx, "New entry without container");
      return -1;
    }

    if (ctx->cur_container->kind != SUBFORMS) {
      uw_set_error(ctx, "Bad kind for entry parent");
      return -1;
    }

    inps = check_input_space(ctx, ctx->app->inputs_len + 1);

    inps->kind = ENTRY;
    inps->data.entry.parent = ctx->cur_container;
    inps->data.entry.next = ctx->cur_container->data.subforms.entries;
    ctx->cur_container->data.subforms.entries = inps;

    inps->data.entry.fields = inps+1;
    ctx->cur_container = inps;
  } else {
    int n = ctx->app->input_num(name);

    if (n < 0)
      return 0;

    if (n >= ctx->app->inputs_len) {
      uw_set_error(ctx, "For input name %s, index %d is out of range", uw_Basis_htmlifyString(ctx, name), n);
      return -1;
    }

    INP(ctx)[n].kind = NORMAL;
    INP(ctx)[n].data.normal = value;
  }

  return 0;
}

char *uw_get_input(uw_context ctx, int n) {
  if (n < 0)
    uw_error(ctx, FATAL, "Negative input index %d", n);
  if (n >= ctx->app->inputs_len)
    uw_error(ctx, FATAL, "Out-of-bounds input index %d", n);

  switch (INP(ctx)[n].kind) {
  case UNSET:
    return NULL;
  case FIL:
    uw_error(ctx, FATAL, "Tried to read a file form input as normal");
  case SUBFORM:
    uw_error(ctx, FATAL, "Tried to read a subform form input as normal");
  case SUBFORMS:
    uw_error(ctx, FATAL, "Tried to read a subforms form input as normal");
  case ENTRY:
    uw_error(ctx, FATAL, "Tried to read an entry form input as normal");
  case NORMAL:
    return INP(ctx)[n].data.normal;
  default:
    uw_error(ctx, FATAL, "Impossible input kind");
  }
}

char *uw_get_optional_input(uw_context ctx, int n) {
  if (n < 0)
    uw_error(ctx, FATAL, "Negative input index %d", n);
  if (n >= ctx->app->inputs_len)
    uw_error(ctx, FATAL, "Out-of-bounds input index %d", n);

  switch (INP(ctx)[n].kind) {
  case UNSET:
    return "";
  case FIL:
    uw_error(ctx, FATAL, "Tried to read a file form input as normal");
  case SUBFORM:
    uw_error(ctx, FATAL, "Tried to read a subform form input as normal");
  case SUBFORMS:
    uw_error(ctx, FATAL, "Tried to read a subforms form input as normal");
  case ENTRY:
    uw_error(ctx, FATAL, "Tried to read an entry form input as normal");
  case NORMAL:
    return INP(ctx)[n].data.normal;
  default:
    uw_error(ctx, FATAL, "Impossible input kind");
  }
}

int uw_set_file_input(uw_context ctx, const char *name, uw_Basis_file f) {
  int n = ctx->app->input_num(name);

  if (n < 0) {
    uw_set_error(ctx, "Bad file input name");
    return -1;
  }

  if (n >= ctx->app->inputs_len) {
    uw_set_error(ctx, "For file input name, index %d is out of range", n);
    return -1;
  }

  ctx->inputs[n].kind = FIL;
  ctx->inputs[n].data.file = f;

  return 0;
}

void *uw_malloc(uw_context ctx, size_t len);

uw_Basis_file uw_get_file_input(uw_context ctx, int n) {
  if (n < 0)
    uw_error(ctx, FATAL, "Negative file input index %d", n);
  if (n >= ctx->app->inputs_len)
    uw_error(ctx, FATAL, "Out-of-bounds file input index %d", n);

  switch (INP(ctx)[n].kind) {
  case UNSET:
    {
      char *data = uw_malloc(ctx, 0);
      uw_Basis_file f = {NULL, "", {0, data}};
      return f;
    }
  case FIL:
    return INP(ctx)[n].data.file;
  case NORMAL:
    uw_error(ctx, FATAL, "Tried to read a normal form input as files");
  case SUBFORM:
    uw_error(ctx, FATAL, "Tried to read a subform form input as files");
  case SUBFORMS:
    uw_error(ctx, FATAL, "Tried to read a subforms form input as files");
  case ENTRY:
    uw_error(ctx, FATAL, "Tried to read an entry form input as files");
  default:
    uw_error(ctx, FATAL, "Impossible input kind");
  }
}

void uw_enter_subform(uw_context ctx, int n) {
  if (n < 0)
    uw_error(ctx, FATAL, "Negative subform index %d", n);
  if (n >= ctx->app->inputs_len)
    uw_error(ctx, FATAL, "Out-of-bounds subform index %d", n);

  switch (INP(ctx)[n].kind) {
  case UNSET:
    uw_error(ctx, FATAL, "Missing subform");
  case FIL:
    uw_error(ctx, FATAL, "Tried to read a file form input as subform");
  case NORMAL:
    uw_error(ctx, FATAL, "Tried to read a normal form input as subform");
  case SUBFORMS:
    uw_error(ctx, FATAL, "Tried to read a subforms form input as subform");
  case ENTRY:
    uw_error(ctx, FATAL, "Tried to read an entry form input as subform");
  case SUBFORM:
    INP(ctx)[n].data.subform.parent = ctx->cur_container;
    ctx->cur_container = &INP(ctx)[n];
    return;
  default:
    uw_error(ctx, FATAL, "Impossible input kind");
  }
}

void uw_leave_subform(uw_context ctx) {
  input *tmp;

  if (ctx->cur_container == NULL)
    uw_error(ctx, FATAL, "Unmatched uw_leave_subform");

  tmp = ctx->cur_container;
  ctx->cur_container = tmp->data.subform.parent;
  tmp->data.subform.parent = NULL;
}

int uw_enter_subforms(uw_context ctx, int n) {
  input *inps;

  if (n < 0)
    uw_error(ctx, FATAL, "Negative subforms index %d", n);
  if (n >= ctx->app->inputs_len)
    uw_error(ctx, FATAL, "Out-of-bounds subforms index %d", n);

  switch (INP(ctx)[n].kind) {
  case UNSET:
    uw_error(ctx, FATAL, "Missing subforms");
  case FIL:
    uw_error(ctx, FATAL, "Tried to read a file form input as subforms");
  case NORMAL:
    uw_error(ctx, FATAL, "Tried to read a normal form input %p as subforms", &INP(ctx)[n]);
  case SUBFORM:
    uw_error(ctx, FATAL, "Tried to read a subform form input as subforms");
  case ENTRY:
    uw_error(ctx, FATAL, "Tried to read an entry form input as subforms");
  case SUBFORMS:
    inps = INP(ctx)[n].data.subforms.entries;
    if (inps) {
      INP(ctx)[n].data.subforms.parent = ctx->cur_container;
      ctx->cur_container = INP(ctx)[n].data.subforms.entries;
      return 1;
    } else
      return 0;
  default:
    uw_error(ctx, FATAL, "Impossible input kind");
  }
}

int uw_next_entry(uw_context ctx) {
  if (ctx->cur_container == NULL)
    uw_error(ctx, FATAL, "uw_next_entry(NULL)");

  switch (ctx->cur_container->kind) {
  case UNSET:
    uw_error(ctx, FATAL, "Missing entry");
  case FIL:
    uw_error(ctx, FATAL, "Tried to read a file form input as entry");
  case NORMAL:
    uw_error(ctx, FATAL, "Tried to read a normal form input as entry");
  case SUBFORM:
    uw_error(ctx, FATAL, "Tried to read a subform form input as entry");
  case SUBFORMS:
    uw_error(ctx, FATAL, "Tried to read a subforms form input as entry");
  case ENTRY:
    if (ctx->cur_container->data.entry.next) {
      ctx->cur_container = ctx->cur_container->data.entry.next;
      return 1;
    } else {
      ctx->cur_container = ctx->cur_container->data.entry.parent->data.subforms.parent;
      return 0;
    }
  default:
    uw_error(ctx, FATAL, "Impossible input kind");
  }
}

void uw_set_script_header(uw_context ctx, const char *s) {
  ctx->script_header = s;
}

const char *uw_get_url_prefix(uw_context ctx) {
  return ctx->app->url_prefix;
}

void uw_set_needs_push(uw_context ctx, int n) {
  ctx->needs_push = n;
}

void uw_set_needs_sig(uw_context ctx, int n) {
  ctx->needs_sig = n;
}

void uw_set_could_write_db(uw_context ctx, int n) {
  ctx->could_write_db = n;
}

// For the backend's BEGIN: whether it may wait for a lock that another
// connection holds, up to its timeout, such as SQLite's write lock for a
// transaction that may write.  Only the expunger's may not: the client
// pruner keeps every request of the process out while it runs.
int uw_db_may_wait(uw_context ctx) {
  return ctx->db_may_wait;
}

void uw_set_at_most_one_query(uw_context ctx, int n) {
  ctx->at_most_one_query = n;
}


// The heap.  A context's heap is a reservation of address space, made once
// in uw_init and never moved: heap_reserved bytes mapped PROT_NONE, which
// counts against no overcommit limit, of which pages are made readable and
// writable as the heap grows (uw_heap_grow).  Growing therefore never
// invalidates a pointer into the heap, so no request is run again for its
// heap to grow, and io code, which cannot be run again, allocates like any
// other.  What is reserved is the heap limit when there is one, else
// UW_HEAP_DEFAULT_RESERVATION, which then is the limit in effect; the
// reservation is all a heap costs until it is used.
#if SIZE_MAX > 0xffffffffULL
#define UW_HEAP_DEFAULT_RESERVATION (8ULL * 1024 * 1024 * 1024)
#else
#define UW_HEAP_DEFAULT_RESERVATION (256UL * 1024 * 1024)
#endif

static size_t page_size(void) {
  static size_t sz = 0;
  if (!sz) {
    long l = sysconf(_SC_PAGESIZE);
    sz = l > 0 ? (size_t)l : 4096;
  }
  return sz;
}

// n rounded up to whole pages, at least one; 0 if that does not fit.
static size_t whole_pages(size_t n) {
  size_t ps = page_size();
  if (n <= ps)
    return ps;
  if (n > SIZE_MAX - (ps - 1))
    return 0;
  return (n + ps - 1) / ps * ps;
}

// The size to grow a buffer of [have] bytes to, for [desired]: doubled
// until that fits, at most [max], and when doubling would go past the
// limit, [desired] itself if it fits; 0 when it does not.
static size_t uw_grown_size(size_t have, size_t desired, size_t max) {
  size_t next = have ? have : 1;

  while (next < desired)
    next *= 2;

  if (next > max)
    next = desired <= max ? desired : 0;

  return next;
}

// The heaps of contexts that were freed, kept for the next contexts to be
// made rather than unmapped.  A heap's reservation is the same size for
// every context, so one can always take another's.  Unmapping would be
// wrong under Fil-C: its munmap refuses a range of 8 GiB, and a range it
// does unmap, in smaller pieces, then counts against the overcommit limit,
// so that the fork of a daemonizing server (-d) fails for want of memory
// once the initializer's context has been freed.  Kept, a heap costs its
// address space and nothing else, and no more heaps are kept than were
// ever in use at once.
typedef struct uw_heap_spare {
  char *start;
  size_t reserved, committed;
  struct uw_heap_spare *next;
} uw_heap_spare_t;

static uw_heap_spare_t *uw_heap_spares;
static pthread_mutex_t uw_heap_spares_mutex = PTHREAD_MUTEX_INITIALIZER;

// The heap of ctx, which is being freed, to the spares: its pages given
// back, and its committed part still writable.
static void uw_heap_spare(uw_context ctx) {
  uw_heap_spare_t *h = malloc(sizeof *h);
  size_t committed = ctx->heap_committed;

  if (!h)
    return;   // the reservation stays mapped and unused
#if defined(MADV_DONTNEED)
  madvise(ctx->heap.start, committed, MADV_DONTNEED);
#elif defined(MADV_FREE)
  madvise(ctx->heap.start, committed, MADV_FREE);
#endif
  h->start = ctx->heap.start;
  h->reserved = ctx->heap_reserved;
  h->committed = committed;
  pthread_mutex_lock(&uw_heap_spares_mutex);
  h->next = uw_heap_spares;
  uw_heap_spares = h;
  pthread_mutex_unlock(&uw_heap_spares_mutex);
}

// A spare heap reserved to the size given, taken from the spares, or NULL;
// how much of it is committed into *committed.
static char *uw_heap_take_spare(size_t reserve, size_t *committed) {
  uw_heap_spare_t **p, *h;
  char *start = NULL;

  pthread_mutex_lock(&uw_heap_spares_mutex);
  for (p = &uw_heap_spares; *p && (*p)->reserved != reserve; p = &(*p)->next);
  if ((h = *p)) {
    *p = h->next;
    start = h->start;
    *committed = h->committed;
  }
  pthread_mutex_unlock(&uw_heap_spares_mutex);
  free(h);
  return start;
}

// The limit in effect, and that in whole pages, which is what a heap
// reserves.
static size_t uw_heap_limit(size_t *reserve) {
  size_t limit = uw_heap_max == SIZE_MAX ? UW_HEAP_DEFAULT_RESERVATION : uw_heap_max;
  *reserve = whole_pages(limit);
  return limit;
}

// A heap's reservation: reserve bytes of address space, or the process
// ends, saying how much was asked for and where that comes from.  There is
// no jmp_buf to fail through where this is called.
static char *uw_heap_reserve(size_t limit, size_t reserve) {
  char *map = MAP_FAILED;

  if (reserve)
    map = mmap(NULL, reserve, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS
#ifdef MAP_NORESERVE
               | MAP_NORESERVE
#endif
               , -1, 0);
  if (map == MAP_FAILED) {
    fprintf(stderr, "Cannot reserve %zu bytes of address space for a heap: %s\n"
            "(Each request-handling thread and task reserves the heap limit, URWEB_LIMIT_HEAP or `limit heap` in the .urp,"
            " or %llu bytes when there is none.)\n",
            limit, strerror(reserve ? errno : EOVERFLOW), (unsigned long long)UW_HEAP_DEFAULT_RESERVATION);
    exit(1);
  }
  return map;
}

void uw_heap_prepare(int n) {
  size_t reserve, limit = uw_heap_limit(&reserve);
  uw_heap_spare_t *h;
  int have = 0;

  pthread_mutex_lock(&uw_heap_spares_mutex);
  for (h = uw_heap_spares; h; h = h->next)
    have += h->reserved == reserve;
  pthread_mutex_unlock(&uw_heap_spares_mutex);

  for (; have < n; ++have) {
    char *map = uw_heap_reserve(limit, reserve);
    if (!(h = malloc(sizeof *h)))
      return;   // the reservation stays mapped and unused, as a freed one would
    h->start = map;
    h->reserved = reserve;
    h->committed = 0;
    pthread_mutex_lock(&uw_heap_spares_mutex);
    h->next = uw_heap_spares;
    uw_heap_spares = h;
    pthread_mutex_unlock(&uw_heap_spares_mutex);
  }
}

// Called by uw_init, before the context has a jmp_buf to fail through, so
// a heap that cannot be reserved or started ends the process (uw_heap_reserve).
static void uw_heap_init(uw_context ctx) {
  size_t reserve, limit = uw_heap_limit(&reserve);
  size_t initial = whole_pages(uw_min_heap), committed = 0;
  char *map = reserve ? uw_heap_take_spare(reserve, &committed) : NULL;

  if (map)
    ;   // a spare: what it had committed is writable, and reads as zeroes
  else
    map = uw_heap_reserve(limit, reserve);

  if (initial > reserve)
    initial = reserve;
  if (initial > committed && mprotect(map + committed, initial - committed, PROT_READ | PROT_WRITE)) {
    fprintf(stderr, "Cannot commit the first %zu bytes of a heap: %s\n", initial, strerror(errno));
    exit(1);
  }

  ctx->heap.start = ctx->heap.front = map;
  ctx->heap.back = map + initial;
  ctx->heap.max = limit;
  ctx->heap_reserved = reserve;
  ctx->heap_committed = initial > committed ? initial : committed;
  ctx->heap_peak = ctx->heap_dropped = ctx->heap_recent = 0;
}

// Room for extra more bytes at heap.front: what is available doubles until
// it holds them, within the limit, and the pages that takes are made
// writable.
static void uw_heap_grow(uw_context ctx, size_t extra) {
  uw_buffer *b = &ctx->heap;
  size_t used = b->front - b->start, have = b->back - b->start, next;

  next = extra > b->max - used || used > b->max ? 0 : uw_grown_size(have, used + extra, b->max);
  if (!next)
    uw_error(ctx, FATAL, "Memory limit exceeded (heap)");
  // Whole pages, within the reservation, which is the limit in whole pages.
  // What was committed once, and given back since (uw_heap_shrink), is
  // still writable, and only the pages beyond it need committing.
  next = whole_pages(next);
  if (next > ctx->heap_committed) {
    if (mprotect(b->start + ctx->heap_committed, next - ctx->heap_committed, PROT_READ | PROT_WRITE))
      uw_error(ctx, FATAL, "Cannot commit %zu more bytes of heap: %s", next - ctx->heap_committed, strerror(errno));
    ctx->heap_committed = next;
  }
  b->back = b->start + next;
}

// The front of the heap is about to move back to [to]: what it reached
// counts towards heap_peak, and what it passes over towards heap_dropped.
// Everything that moves it back says so first.  So heap_peak is the most
// that was in use at any time, and heap_dropped plus what is in use now is
// all that was allocated, the front moving forward only to allocate, at no
// cost to allocation.
static void uw_heap_moving_back(uw_context ctx, char *to) {
  size_t used = ctx->heap.front - ctx->heap.start;
  if (used > ctx->heap_peak)
    ctx->heap_peak = used;
  if (to < ctx->heap.front)
    ctx->heap_dropped += ctx->heap.front - to;
}

void uw_heap_usage(uw_context ctx, size_t *peak, size_t *total) {
  size_t used = ctx->heap.front - ctx->heap.start;
  *peak = used > ctx->heap_peak ? used : ctx->heap_peak;
  *total = ctx->heap_dropped + used;
}

// Between pieces of work -- a request served, a task's tick done, a pass
// of the expunger -- the heap is emptied, and what it keeps available is
// a recent maximum that decays: twice the most this piece used, or half
// of what was kept before, whichever is more, but at least UW_HEAP_RETAIN
// or minHeap.  The pages above it are given back (MADV_DONTNEED; they stay
// mapped and writable and read as zeroes when next touched).  So a thread
// that keeps serving requests of a size keeps the pages they need, and
// one that served a large request gives its memory back over the next
// few, halving each time.  Not between the attempts of one piece of work:
// a request run again after a commit conflict keeps its memory.
#define UW_HEAP_RETAIN (1024 * 1024)

void uw_heap_shrink(uw_context ctx) {
  uw_buffer *b = &ctx->heap;
  size_t have = b->back - b->start, twice, half, keep;

  uw_heap_moving_back(ctx, b->start);
  b->front = b->start;
  ctx->regions = NULL;

  twice = ctx->heap_peak > SIZE_MAX / 2 ? SIZE_MAX : ctx->heap_peak * 2;
  half = ctx->heap_recent / 2;
  ctx->heap_recent = twice > half ? twice : half;
  ctx->heap_peak = ctx->heap_dropped = 0;

  keep = ctx->heap_recent;
  if (keep < UW_HEAP_RETAIN)
    keep = UW_HEAP_RETAIN;
  if (keep < (size_t)uw_min_heap)
    keep = uw_min_heap;
  keep = whole_pages(keep);

  if (keep && keep < have) {
#if defined(MADV_DONTNEED)
    madvise(b->start + keep, have - keep, MADV_DONTNEED);
#elif defined(MADV_FREE)
    madvise(b->start + keep, have - keep, MADV_FREE);
#endif
    b->back = b->start + keep;
  }
}

void uw_check_heap(uw_context ctx, size_t extra) {
  if ((size_t)(ctx->heap.back - ctx->heap.front) < extra)
    uw_heap_grow(ctx, extra);
}

char *uw_heap_front(uw_context ctx) {
  return ctx->heap.front;
}

void uw_set_heap_front(uw_context ctx, char *fr) {
  if (fr < ctx->heap.front)
    uw_heap_moving_back(ctx, fr);
  ctx->heap.front = fr;
}

void uw_begin_initializing(uw_context ctx) {
  ctx->amInitializing = 1;
}

void uw_end_initializing(uw_context ctx) {
  ctx->amInitializing = 0;
}

static void align_heap(uw_context ctx) {
  size_t posn = ctx->heap.front - ctx->heap.start;

  if (posn % sizeof(void *) != 0) {
    size_t bump = sizeof(void *) - posn % sizeof(void *);
    uw_check_heap(ctx, bump);
    ctx->heap.front += bump;
  }
}

void *uw_malloc(uw_context ctx, size_t len) {
  // On some architectures, it's important that all word-sized memory accesses
  // be to word-aligned addresses, so we'll do a little bit of extra work here
  // in anticipation of a possible word-aligned access to the address we'll
  // return.

  void *result;

  if (ctx->amInitializing) {
    int error = posix_memalign(&result, sizeof(void *), len);

    if (!error)
      return result;
    else
      uw_error(ctx, FATAL, "uw_malloc: posix_memalign() returns %d", error);
  } else {
    align_heap(ctx);

    uw_check_heap(ctx, len);

    result = ctx->heap.front;
    ctx->heap.front += len;
    return result;
  }
}

void uw_begin_region(uw_context ctx) {
  align_heap(ctx);

  regions *r = (regions *) ctx->heap.front;

  uw_check_heap(ctx, sizeof(regions));

  ctx->heap.front += sizeof(regions);

  r->next = ctx->regions;
  ctx->regions = r;
}

void uw_end_region(uw_context ctx) {
  regions *r = ctx->regions;

  if (r == NULL)
    uw_error(ctx, FATAL, "Region stack underflow");

  uw_heap_moving_back(ctx, (char *) r);
  ctx->heap.front = (char *) r;
  ctx->regions = r->next;
}

void uw_memstats(uw_context ctx) {
  printf("Headers: %lu/%lu\n", (unsigned long)uw_buffer_used(&ctx->outHeaders), (unsigned long)uw_buffer_avail(&ctx->outHeaders));
  printf("Script: %lu/%lu\n", (unsigned long)uw_buffer_used(&ctx->script), (unsigned long)uw_buffer_avail(&ctx->script));
  printf("Page: %lu/%lu\n", (unsigned long)uw_buffer_used(&ctx->page), (unsigned long)uw_buffer_avail(&ctx->page));
  printf("Heap: %lu/%lu\n", (unsigned long)uw_buffer_used(&ctx->heap), (unsigned long)uw_buffer_avail(&ctx->heap));
  {
    size_t peak, total;
    uw_heap_usage(ctx, &peak, &total);
    printf("Heap peak/total: %lu/%lu\n", (unsigned long)peak, (unsigned long)total);
  }
}

int uw_pagelen(uw_context ctx) {
  return ctx->page.front - ctx->page.start;
}

int uw_send(uw_context ctx, int sock) {
  size_t target_length = (ctx->outHeaders.front - ctx->outHeaders.start) + 2 + (ctx->page.front - ctx->page.start);

  if (ctx->output_buffer_size < target_length) {
    do {
      ctx->output_buffer_size *= 2;
    } while (ctx->output_buffer_size < target_length);
    ctx->output_buffer = realloc(ctx->output_buffer, ctx->output_buffer_size);
  }

  memcpy(ctx->output_buffer, ctx->outHeaders.start, ctx->outHeaders.front - ctx->outHeaders.start);
  memcpy(ctx->output_buffer + (ctx->outHeaders.front - ctx->outHeaders.start), "\r\n", 2);
  memcpy(ctx->output_buffer + (ctx->outHeaders.front - ctx->outHeaders.start) + 2, ctx->page.start, ctx->page.front - ctx->page.start);

  return uw_really_send(sock, ctx->output_buffer, target_length);
}

int uw_print(uw_context ctx, int fd) {
  int n = uw_really_write(fd, ctx->outHeaders.start, ctx->outHeaders.front - ctx->outHeaders.start);

  if (n < 0)
    return n;

  n = uw_really_write(fd, "\r\n", 2);

  if (n < 0)
    return n;

  return uw_really_write(fd, ctx->page.start, ctx->page.front - ctx->page.start);
}

int uw_output(uw_context ctx, int (*output)(void *data, char *buf, size_t len), void *data) {
  int n = output(data, ctx->outHeaders.start, ctx->outHeaders.front - ctx->outHeaders.start);

  if (n < 0)
    return n;

  n = output(data, "\r\n", 2);

  if (n < 0)
    return n;

  return output(data, ctx->page.start, ctx->page.front - ctx->page.start);
}

static void uw_check_headers(uw_context ctx, size_t extra) {
  ctx_uw_buffer_check(ctx, "headers", &ctx->outHeaders, extra);
}

void uw_write_header(uw_context ctx, uw_Basis_string s) {
  int len = strlen(s);

  uw_check_headers(ctx, len + 1);
  strcpy(ctx->outHeaders.front, s);
  ctx->outHeaders.front += len;
}

// Whether the headers so far give the length, under any spelling of the
// name: a file of the file directive is sent with Content-Length, and a
// server that added its own Content-length would send two, which a strict
// client refuses.
int uw_has_contentLength(uw_context ctx) {
  const char *p = ctx->outHeaders.start;

  while (p && *p) {
    if (!strncasecmp(p, "Content-Length:", 15))
      return 1;
    p = strstr(p, "\r\n");
    if (p)
      p += 2;
  }
  return 0;
}

void uw_clear_headers(uw_context ctx) {
  uw_buffer_reset(&ctx->outHeaders);
}

void uw_Basis_clear_page(uw_context ctx) {
  uw_buffer_reset(&ctx->page);
}

static void uw_check_script(uw_context ctx, size_t extra) {
  ctx_uw_buffer_check(ctx, "script", &ctx->script, extra);
}

void uw_write_script(uw_context ctx, uw_Basis_string s) {
  int len = strlen(s);

  uw_check_script(ctx, len + 1);
  strcpy(ctx->script.front, s);
  ctx->script.front += len;
}

const char *uw_get_real_script(uw_context ctx) {
  if (strstr(ctx->outHeaders.start, "Set-Cookie: ")) {
    uw_write_script(ctx, "sig=\"");
    uw_write_script(ctx, ctx->app->cookie_sig(ctx));
    uw_write_script(ctx, "\";");
  }

  return ctx->script.start;
}

uw_Basis_string uw_Basis_maybe_onload(uw_context ctx, uw_Basis_string s) {
  if (s[0] == 0)
    return "";
  else {
    char *r = uw_malloc(ctx, 11 + strlen(s));
    sprintf(r, " onload='%s'", s);
    return r;
  }
}

uw_Basis_string uw_Basis_maybe_onunload(uw_context ctx, uw_Basis_string s) {
  if (ctx->script_header[0] == 0)
    return "";
  else {
    char *r = uw_malloc(ctx, 37 + strlen(s));
    sprintf(r, " onunload='unloading=true;%s;unload()'", s);
    return r;
  }
}

const char *uw_Basis_get_settings(uw_context ctx, uw_unit u) {
  (void)u;
  if (ctx->client == NULL) {
    if (ctx->needs_sig) {
      char *sig = ctx->app->cookie_sig(ctx);
      char *r = uw_malloc(ctx, strlen(sig) + 8);
      sprintf(r, "sig=\"%s\";", sig);
      return r;
    }
    else
      return "";
  } else {
    char *sig = ctx->needs_sig ? ctx->app->cookie_sig(ctx) : "";
    char *r = uw_malloc(ctx, 59 + 3 * INTS_MAX + strlen(ctx->app->url_prefix)
                        + (ctx->needs_sig ? strlen(sig) + 7 : 0));
    sprintf(r, "isPost=%s;client_id=%u;client_pass=%d;url_prefix=\"%s\";timeout=%d;%s%s%slistener();",
            (ctx->isPost ? "true" : "false"),
            ctx->client->id,
            ctx->client->pass,
            ctx->app->url_prefix,
            ctx->app->timeout,
            ctx->needs_sig ? "sig=\"" : "",
            sig,
            ctx->needs_sig ? "\";" : "");
    return r;
  }
}

uw_Basis_bool uw_Basis_isprint(uw_context ctx, uw_Basis_char ch);

static void jsifyChar(char **buffer_ptr, uw_context ctx, uw_Basis_char c1) {
  char* buffer = *buffer_ptr;
  
  switch (c1) {
  case '"':
    strcpy(buffer, "\\\"");
    buffer += 2;
    break;
  case '\'':
    strcpy(buffer, "\\047");
    buffer += 4;
    break;
  case '\\':
    strcpy(buffer, "\\\\");
    buffer += 2;
    break;
  case '<':
    strcpy(buffer, "\\074");
    buffer += 4;
    break;
  case '&':
    strcpy(buffer, "\\046");
    buffer += 4;
    break;
  default:
    if (uw_Basis_isprint(ctx, c1)) {
      int offset = 0;
      U8_APPEND_UNSAFE(buffer, offset, c1);
      buffer += offset;
    } else {
      if(65536 > c1) {
	sprintf(buffer, "\\u%04x", c1);
	buffer += 6;
      } else {
	sprintf(buffer, "\\u{%06x}", c1);
	buffer += 10;
      }
    }
  }

  *buffer_ptr = buffer;
}

uw_Basis_string uw_Basis_jsifyString(uw_context ctx, uw_Basis_string s) {
  char *r, *s2;
  uw_Basis_char c;

  uw_check_heap(ctx, strlen(s) * 10 + 3);

  r = s2 = ctx->heap.front;
  *s2++ = '"';

  int offset = 0;
  while(s[offset] != 0)
    {
      U8_NEXT(s, offset, -1, c);
      
      jsifyChar(&s2, ctx, c);      
    }

  strcpy(s2, "\"");
  ctx->heap.front = s2 + 2;

  return r;
}

uw_Basis_int uw_Basis_ord(uw_context ctx, uw_Basis_char c);

uw_Basis_string uw_Basis_jsifyChar(uw_context ctx, uw_Basis_char c1) {
  char *r, *s2;

  uw_check_heap(ctx, 10);

  r = s2 = ctx->heap.front;
  
  *s2++ = '"';
  
  jsifyChar(&s2, ctx, c1);

  strcpy(s2, "\"");
  ctx->heap.front = s2 + 2;

  return r;
}

uw_Basis_string uw_Basis_jsifyString_ws(uw_context ctx, uw_Basis_string s) {
  char *r, *s2;

  uw_check_script(ctx, strlen(s) * 4 + 3);

  r = s2 = ctx->script.front;
  *s2++ = '"';

  for (; *s; s++) {
    unsigned char c = *s;

    switch (c) {
    case '\'':
      strcpy(s2, "\\");
      s2 += 2;
      break;
    case '\\':
      strcpy(s2, "\\\\");
      s2 += 2;
      break;
    case '<':
      strcpy(s2, "\\074");
      s2 += 4;
      break;
    case '&':
      strcpy(s2, "\\046");
      s2 += 4;
      break;
    default:
      if (isprint((int)c) || c >= 128)
        *s2++ = c;
      else {
        sprintf(s2, "\\%03o", c);
        s2 += 4;
      }
    }
  }

  strcpy(s2, "\"");
  ctx->script.front = s2 + 1;

  return r;
}

char *uw_Basis_jsifyChannel(uw_context ctx, uw_Basis_channel chn) {
  if (ctx->client == NULL || chn.cli != ctx->client->id)
    return "null";
  else {
    int len;
    char *r;

    uw_check_heap(ctx, INTS_MAX + 1);
    r = ctx->heap.front;
    sprintf(r, "%u%n", chn.chn, &len);
    ctx->heap.front += len+1;
    return r;
  }
}

uw_Basis_source uw_Basis_new_client_source(uw_context ctx, uw_Basis_string s) {
  int len;
  size_t s_len = strlen(s);

  if(ctx->id < 0)
    uw_error(ctx, FATAL, "Attempt to create client source using inappropriate context");

  uw_check_script(ctx, 15 + 2 * INTS_MAX + s_len);
  sprintf(ctx->script.front, "s%d_%llu=sc(exec(%n", ctx->id, ctx->source_count, &len);
  ctx->script.front += len;
  strcpy(ctx->script.front, s);
  ctx->script.front += s_len;
  strcpy(ctx->script.front, "));");
  ctx->script.front += 3;

  uw_Basis_source r = {ctx->id, ctx->source_count++};
  return r;
}

uw_unit uw_Basis_set_client_source(uw_context ctx, uw_Basis_source src, uw_Basis_string s) {
  int len;
  size_t s_len = strlen(s);

  uw_check_script(ctx, 15 + 2 * INTS_MAX + s_len);
  sprintf(ctx->script.front, "sv(s%d_%llu,exec(%n", src.context, src.source, &len);
  ctx->script.front += len;
  strcpy(ctx->script.front, s);
  ctx->script.front += s_len;
  strcpy(ctx->script.front, "));");
  ctx->script.front += 3;

  return uw_unit_v;
}

static void uw_check(uw_context ctx, size_t extra) {
  ctx_uw_buffer_check(ctx, "page", &ctx->page, extra);
}

static void uw_writec_unsafe(uw_context ctx, char c) {
  *(ctx->page.front)++ = c;
  *ctx->page.front = 0;
}

void uw_writec(uw_context ctx, char c) {
  uw_check(ctx, 2);
  uw_writec_unsafe(ctx, c);
}

void uw_Basis_writec(uw_context ctx, char c) {
  uw_writec(ctx, c);
}

static void uw_write_unsafe(uw_context ctx, const char* s) {
  int len = strlen(s);
  memcpy(ctx->page.front, s, len);
  ctx->page.front += len;
}

void uw_write(uw_context ctx, const char* s) {
  uw_check(ctx, strlen(s) + 1);
  uw_write_unsafe(ctx, s);
  *ctx->page.front = 0;
}

void uw_recordingStart(uw_context ctx) {
  if (ctx->numRecording == ctx->recordingCapacity) {
    ++ctx->recordingCapacity;
    ctx->recordingOffsets = realloc(ctx->recordingOffsets, sizeof(int) * ctx->recordingCapacity);
    ctx->scriptRecordingOffsets = realloc(ctx->scriptRecordingOffsets, sizeof(int) * ctx->recordingCapacity);
  }
  ctx->recordingOffsets[ctx->numRecording] = ctx->page.front - ctx->page.start;
  ctx->scriptRecordingOffsets[ctx->numRecording] = ctx->script.front - ctx->script.start;
  ++ctx->numRecording;
}

char *uw_recordingRead(uw_context ctx) {
  char *recording = ctx->page.start + ctx->recordingOffsets[ctx->numRecording-1];
  return strdup(recording);
}

char *uw_recordingReadScript(uw_context ctx) {
  char *recording = ctx->script.start + ctx->scriptRecordingOffsets[--ctx->numRecording];
  return strdup(recording);
}

char *uw_Basis_attrifyInt(uw_context ctx, uw_Basis_int n) {
  char *result;
  int len;
  uw_check_heap(ctx, INTS_MAX);
  result = ctx->heap.front;
  sprintf(result, "%lld%n", n, &len);
  ctx->heap.front += len+1;
  return result;
}

char *uw_Basis_attrifyFloat(uw_context ctx, uw_Basis_float n) {
  char *result;
  int len;
  uw_check_heap(ctx, FLOATS_MAX);
  result = ctx->heap.front;
  sprintf(result, "%.16g%n", n, &len);
  ctx->heap.front += len+1;
  return result;
}

char *uw_Basis_attrifyString(uw_context ctx, uw_Basis_string s) {
  int len = strlen(s);
  char *result, *p;
  uw_check_heap(ctx, len * 6 + 1);

  result = p = ctx->heap.front;

  for (; *s; s++) {
    unsigned char c = *s;

    if (c == '"') {
      strcpy(p, "&quot;");
      p += 6;
    } else if (c == '&') {
      strcpy(p, "&amp;");
      p += 5;
    }
    else
      *p++ = c;
  }

  *p++ = 0;
  ctx->heap.front = p;
  return result;
}

char *uw_Basis_attrifyChar(uw_context ctx, uw_Basis_char c) {
  char *result, *p;
  uw_check_heap(ctx, 7);

  result = p = ctx->heap.front;

  if (c == '"') {
    strcpy(p, "&quot;");
    p += 6;
  } else if (c == '&') {
    strcpy(p, "&amp;");
    p += 5;
  }
  else
    *p++ = c;

  *p++ = 0;
  ctx->heap.front = p;
  return result;
}

char *uw_Basis_attrifyCss_class(uw_context ctx, uw_Basis_css_class s) {
  (void)ctx;
  return s;
}

static void uw_Basis_attrifyInt_w_unsafe(uw_context ctx, uw_Basis_int n) {
  int len;

  sprintf(ctx->page.front, "%lld%n", n, &len);
  ctx->page.front += len;
}

uw_unit uw_Basis_attrifyInt_w(uw_context ctx, uw_Basis_int n) {
  uw_check(ctx, INTS_MAX);
  uw_Basis_attrifyInt_w_unsafe(ctx, n);

  return uw_unit_v;
}

uw_unit uw_Basis_attrifyFloat_w(uw_context ctx, uw_Basis_float n) {
  int len;

  uw_check(ctx, FLOATS_MAX);
  sprintf(ctx->page.front, "%g%n", n, &len);
  ctx->page.front += len;

  return uw_unit_v;
}

uw_unit uw_Basis_attrifyString_w(uw_context ctx, uw_Basis_string s) {
  uw_check(ctx, strlen(s) * 6);

  for (; *s; s++) {
    unsigned char c = *s;

    if (c == '"')
      uw_write_unsafe(ctx, "&quot;");
    else if (c == '&')
      uw_write_unsafe(ctx, "&amp;");
    else
      uw_writec_unsafe(ctx, c);
  }

  return uw_unit_v;
}

uw_unit uw_Basis_attrifyChar_w(uw_context ctx, uw_Basis_char c) {
  uw_check(ctx, 6);

  if (c == '"')
    uw_write_unsafe(ctx, "&quot;");
  else if (c == '&')
    uw_write_unsafe(ctx, "&amp;");
  else
    uw_writec_unsafe(ctx, c);

  return uw_unit_v;
}


char *uw_Basis_urlifyInt(uw_context ctx, uw_Basis_int n) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%lld%n", n, &len);
  ctx->heap.front += len+1;
  return r;
}

char *uw_Basis_urlifyChannel(uw_context ctx, uw_Basis_channel chn) {
  if (ctx->client == NULL || chn.cli != ctx->client->id)
    return "";
  else {
    int len;
    char *r;

    uw_check_heap(ctx, INTS_MAX + 1);
    r = ctx->heap.front;
    sprintf(r, "%u%n", chn.chn, &len);
    ctx->heap.front += len+1;
    return r;
  }
}

char *uw_Basis_urlifyFloat(uw_context ctx, uw_Basis_float n) {
  int len;
  char *r;

  uw_check_heap(ctx, FLOATS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%g%n", n, &len);
  ctx->heap.front += len+1;
  return r;
}

static void aux_urlifyChar(char** ptr, uw_Basis_char c) {
  char* p = *ptr;
  
  if((uint32_t)(c) <= 0x7f) {
    sprintf(p, ".%02X", (uint8_t)(c));
    p += 3;
  } else {
    if((uint32_t)(c) <= 0x7ff) {
      sprintf(p, ".%02X", (uint8_t)(((c)>>6)|0xc0));
      p += 3;
    } else {
      if((uint32_t)(c) <= 0xffff) { 
	sprintf(p, ".%02X", (uint8_t)(((c)>>12)|0xe0));
	p += 3;
      } else { 
	sprintf(p, ".%02X", (uint8_t)(((c)>>18)|0xf0));
	p += 3;
	sprintf(p, ".%02X", (uint8_t)((((c)>>12)&0x3f)|0x80));
	p += 3;
      } 
      sprintf(p, ".%02X", (uint8_t)((((c)>>6)&0x3f)|0x80));
      p += 3;
    } 
    sprintf(p, ".%02X", (uint8_t)(((c)&0x3f)|0x80));
    p += 3;
  }

  *ptr = p;
}

char *uw_Basis_urlifyString(uw_context ctx, uw_Basis_string s) {
  char *r, *p;

  if (s[0] == '\0')
    return "_";

  uw_check_heap(ctx, strlen(s) * 12 + 1 + !!(s[0] == '_'));

  r = p = ctx->heap.front;
  if (s[0] == '_')
    *p++ = '_';

  uw_Basis_char c;
  int offset = 0, curr = 0;
  while (s[offset] != 0) {
    U8_NEXT(s, offset, -1, c);
  
    if (U8_IS_SINGLE(s[curr]) && s[curr] == ' ')
      *p++ = '+';
    else if (U8_IS_SINGLE(s[curr]) && isalnum(s[curr]))
      *p++ = s[curr];
    else {
      aux_urlifyChar(&p, c);
    }
    curr = offset;
  }

  *p++ = 0;
  ctx->heap.front = p;
  return r;
}

char *uw_Basis_urlifyBool(uw_context ctx, uw_Basis_bool b) {
  (void)ctx;
  if (!b)
    return "0";
  else
    return "1";
}

char *uw_Basis_urlifySource(uw_context ctx, uw_Basis_source src) {
  char *r;
  int len;
  uw_check_heap(ctx, 2 * INTS_MAX + 2);
  r = ctx->heap.front;
  sprintf(r, "%d/%llu%n", src.context, src.source, &len);
  ctx->heap.front += len+1;
  return r;
}

static void uw_Basis_urlifyInt_w_unsafe(uw_context ctx, uw_Basis_int n) {
  int len;

  sprintf(ctx->page.front, "%lld%n", n, &len);
  ctx->page.front += len;
}

uw_unit uw_Basis_urlifyInt_w(uw_context ctx, uw_Basis_int n) {
  uw_check(ctx, INTS_MAX);
  uw_Basis_urlifyInt_w_unsafe(ctx, n);

  return uw_unit_v;
}

uw_unit uw_Basis_urlifyChannel_w(uw_context ctx, uw_Basis_channel chn) {
  if (ctx->client != NULL && chn.cli == ctx->client->id) {
    int len;

    uw_check(ctx, INTS_MAX + 1);
    sprintf(ctx->page.front, "%u%n", chn.chn, &len);
    ctx->page.front += len;
  }

  return uw_unit_v;
}

uw_unit uw_Basis_urlifyFloat_w(uw_context ctx, uw_Basis_float n) {
  int len;

  uw_check(ctx, FLOATS_MAX);
  sprintf(ctx->page.front, "%g%n", n, &len);
  ctx->page.front += len;

  return uw_unit_v;
}

uw_Basis_string uw_Basis_urlifyTime(uw_context ctx, uw_Basis_time t) {
  return uw_Basis_urlifyInt(ctx, (uw_Basis_int)t.seconds * 1000000 + t.microseconds);
}

uw_unit uw_Basis_urlifyTime_w(uw_context ctx, uw_Basis_time t) {
  return uw_Basis_urlifyInt_w(ctx, (uw_Basis_int)t.seconds * 1000000 + t.microseconds);
}

uw_unit uw_Basis_urlifyChar_w(uw_context ctx, uw_Basis_char c) {
  if (c == '\0') {
    uw_check(ctx, 1);
    uw_writec_unsafe(ctx, '_');
    return uw_unit_v;
  }

  uw_check(ctx, 12 + !!(c == '_'));

  if (c == '_')
    uw_writec_unsafe(ctx, '_');
  
  if (c == ' ')
    uw_writec_unsafe(ctx, '+');
  else if ((uint32_t)c <= 0x7f && isalnum(c))
    uw_writec_unsafe(ctx, c);
  else {
    aux_urlifyChar(&(ctx->page.front), c);
  }
  
  return uw_unit_v;
}

uw_unit uw_Basis_urlifyString_w(uw_context ctx, uw_Basis_string s) {
  if (s[0] == '\0') {
    uw_check(ctx, 1);
    uw_writec_unsafe(ctx, '_');
    return uw_unit_v;
  }

  uw_check(ctx, strlen(s) * 12 + !!(s[0] == '_'));

  if (s[0] == '_')
    uw_writec_unsafe(ctx, '_');

  uw_Basis_char c;
  int offset = 0, curr = 0;
  while (s[offset] != 0) {
    U8_NEXT(s, offset, -1, c);   
    
    if (U8_IS_SINGLE(s[curr]) && s[curr] == ' ')
      uw_writec_unsafe(ctx, '+');
    else if (U8_IS_SINGLE(s[curr]) && isalnum(s[curr]))
      uw_writec_unsafe(ctx, s[curr]);
    else {      
      aux_urlifyChar(&(ctx->page.front),  c);
    }
    curr = offset;
  }

  return uw_unit_v;
}

uw_unit uw_Basis_urlifyBool_w(uw_context ctx, uw_Basis_bool b) {
  if (!b)
    uw_writec(ctx, '0');
  else
    uw_writec(ctx, '1');

  return uw_unit_v;
}

uw_unit uw_Basis_urlifySource_w(uw_context ctx, uw_Basis_source src) {
  int len;

  uw_check(ctx, 2 * INTS_MAX + 2);
  sprintf(ctx->page.front, "%d/%llu%n", src.context, src.source, &len);
  ctx->page.front += len;

  return uw_unit_v;
}


static char *uw_unurlify_advance(char *s) {
  char *new_s = strchr(s, '/');

  if (new_s)
    *new_s++ = 0;
  else
    new_s = strchr(s, 0);

  return new_s;
}

uw_Basis_int uw_Basis_unurlifyInt(uw_context ctx, char **s) {
  (void)ctx;

  char *new_s = uw_unurlify_advance(*s);
  uw_Basis_int r;

  r = atoll(*s);
  *s = new_s;
  return r;
}

uw_Basis_float uw_Basis_unurlifyFloat(uw_context ctx, char **s) {
  (void)ctx;

  char *new_s = uw_unurlify_advance(*s);
  uw_Basis_float r;

  r = atof(*s);
  *s = new_s;
  return r;
}

// The time n / per_second seconds after the epoch: whole seconds rounded
// down, and the microseconds after them, 0 to 999999 whatever the sign.  C's
// / and % round towards zero, which for -1500 ms would give -1 s and -500000
// microseconds, in a field that is unsigned.  per_second divides 1000000.
static uw_Basis_time time_of_count(uw_Basis_int n, uw_Basis_int per_second) {
  uw_Basis_int seconds = n / per_second, rest = n % per_second;

  if (rest < 0) {
    --seconds;
    rest += per_second;
  }

  uw_Basis_time r = {seconds, rest * (1000000 / per_second)};
  return r;
}

uw_Basis_time uw_Basis_unurlifyTime(uw_context ctx, char **s) {
  return time_of_count(uw_Basis_unurlifyInt(ctx, s), 1000000);
}

static uw_Basis_string uw_unurlifyString_to(int fromClient, uw_context ctx, char *r, char *s) {
  char *s1, *s2 = s;
  int n;

  if (!fromClient) {
    if (*s2 == '_')
      ++s2;
    else if ((s2[0] == '%' || s2[0] == '.') && s2[1] == '5' && (s2[2] == 'f' || s2[2] == 'F'))
      s2 += 3;
  }

  for (s1 = r; *s2; ++s1, ++s2) {
    unsigned char c = *s2;

    switch (c) {
    case '+':
      *s1 = ' ';
      break;
    case '%':
      if (s2[1] == 0)
        uw_error(ctx, FATAL, "Missing first character of escaped URL byte");
      if (s2[2] == 0)
        uw_error(ctx, FATAL, "Missing second character of escaped URL byte");
      if (sscanf(s2+1, "%02X", &n) != 1)
        uw_error(ctx, FATAL, "Invalid escaped URL byte starting at: %s", uw_Basis_htmlifyString(ctx, s2));
      *s1 = n;
      s2 += 2;
      break;
    case '.':
      if (!fromClient) {
        if (s2[1] == 0)
          uw_error(ctx, FATAL, "Missing first character of escaped URL byte");
        if (s2[2] == 0)
          uw_error(ctx, FATAL, "Missing second character of escaped URL byte");
        if (sscanf(s2+1, "%02X", &n) != 1)
          uw_error(ctx, FATAL, "Invalid escaped URL byte starting at: %s", uw_Basis_htmlifyString(ctx, s2));
        *s1 = n;
        s2 += 2;
        break;
      }
    default:
      *s1 = c;
    }
  }
  *s1++ = 0;
  return s1;
}

uw_Basis_bool uw_Basis_unurlifyBool(uw_context ctx, char **s) {
  (void)ctx;

  char *new_s = uw_unurlify_advance(*s);
  uw_Basis_bool r;

  if (*s[0] == 0 || !strcmp(*s, "0") || !strcmp(*s, "off"))
    r = uw_Basis_False;
  else
    r = uw_Basis_True;

  *s = new_s;
  return r;
}

uw_Basis_string uw_Basis_unurlifyString(uw_context ctx, char **s) {
  char *new_s = uw_unurlify_advance(*s);
  char *r;
  int len;

  len = strlen(*s);
  uw_check_heap(ctx, len + 1);

  r = ctx->heap.front;
  ctx->heap.front = uw_unurlifyString_to(0, ctx, ctx->heap.front, *s);
  *s = new_s;
  return r;
}

uw_Basis_char uw_Basis_unurlifyChar(uw_context ctx, char **s) {
  char *new_s = uw_unurlify_advance(*s);
  char *r;
  int len;

  len = strlen(*s);
  uw_check_heap(ctx, len + 1);

  r = ctx->heap.front;
  ctx->heap.front = uw_unurlifyString_to(0, ctx, ctx->heap.front, *s);
  *s = new_s;
  {
    // The bytes are UTF-8, as urlifyChar_w wrote them: exactly one
    // well-formed character is one char.
    int32_t i = 0, n = strlen(r);
    UChar32 c;

    if (n > 0) {
      U8_NEXT(r, i, n, c);
      if (c >= 0 && i == n)
        return c;
    }
  }
  uw_error(ctx, FATAL, "Unurlified character is not exactly one character");
}

uw_Basis_unit uw_Basis_unurlifyUnit(uw_context ctx, char **s) {
  (void)ctx;
  *s = uw_unurlify_advance(*s);
  return uw_unit_v;
}

uw_Basis_string uw_Basis_unurlifyString_fromClient(uw_context ctx, char **s) {
  char *new_s = uw_unurlify_advance(*s);
  char *r;
  int len;

  len = strlen(*s);
  uw_check_heap(ctx, len + 1);

  r = ctx->heap.front;
  ctx->heap.front = uw_unurlifyString_to(1, ctx, ctx->heap.front, *s);
  *s = new_s;
  return r;
}


char *uw_Basis_htmlifyInt(uw_context ctx, uw_Basis_int n) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%lld%n", n, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_unit uw_Basis_htmlifyInt_w(uw_context ctx, uw_Basis_int n) {
  int len;

  uw_check(ctx, INTS_MAX);
  sprintf(ctx->page.front, "%lld%n", n, &len);
  ctx->page.front += len;

  return uw_unit_v;
}

char *uw_Basis_htmlifySpecialChar(uw_context ctx, uw_Basis_char ch) {
  unsigned int n = ch;
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX+3 + 1);
  r = ctx->heap.front;
  len = sprintf(r, "&#%u;", n);
  ctx->heap.front += len+1;

  return r;
}

uw_unit uw_Basis_htmlifySpecialChar_w(uw_context ctx, uw_Basis_char ch) {
  unsigned int n = ch;
  int len = 0;

  uw_check(ctx, INTS_MAX+3);

  if(uw_Basis_isprint(ctx, ch)) {
    U8_APPEND_UNSAFE(ctx->page.front, len, ch);
  }

  // either it's a non-printable character, or we failed to convert to UTF-8
  if(len == 0) {
    len = sprintf(ctx->page.front, "&#%u;", n);
  }
  ctx->page.front += len;

  return uw_unit_v;
}

char *uw_Basis_htmlifyFloat(uw_context ctx, uw_Basis_float n) {
  int len;
  char *r;

  uw_check_heap(ctx, FLOATS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%g%n", n, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_unit uw_Basis_htmlifyFloat_w(uw_context ctx, uw_Basis_float n) {
  int len;

  uw_check(ctx, FLOATS_MAX);
  sprintf(ctx->page.front, "%g%n", n, &len);
  ctx->page.front += len;

  return uw_unit_v;
}

char *uw_Basis_jsifyTime(uw_context ctx, uw_Basis_time t) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%lld%n", (uw_Basis_int)t.seconds * 1000000 + t.microseconds, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_unit uw_Basis_jsifyInt_w(uw_context ctx, uw_Basis_int n) {
  int len;

  uw_check(ctx, INTS_MAX);
  sprintf(ctx->page.front, "%lld%n", (uw_Basis_int)n, &len);
  ctx->page.front += len;

  return uw_unit_v;
}

char *uw_Basis_htmlifyString(uw_context ctx, const char *s) {
  char *r, *s2;
  uw_Basis_char c1;
  int oldoffset = 0, offset = 0, offset2 = 0, len = 0;
  
  uw_check_heap(ctx, strlen(s) * (INTS_MAX + 3) + 1);

  r = s2 = ctx->heap.front;
  
  while (s[offset] != 0) {
    oldoffset = offset;
    U8_NEXT(s, offset, -1, c1);

    if ((offset - oldoffset == 1) && uw_Basis_isprint(ctx, c1)) {
      switch (c1) {
      case '<':
	strcpy(s2, "&lt;");
	s2 += 4;
	break;
      case '&':
	strcpy(s2, "&amp;");
	s2 += 5;
	break;
      default:
	offset2 = 0;
	U8_APPEND_UNSAFE(s2, offset2, c1);
	s2 += offset2;
      }      
    } else {
      len = sprintf(s2, "&#%u;", c1);
      s2 += len;
    }
  }

  *s2++ = 0;
  ctx->heap.front = s2;
  return r;
}

uw_unit uw_Basis_htmlifyString_w(uw_context ctx, uw_Basis_string s) {
  uw_check(ctx, strlen(s) * 6);
  int offset = 0, oldoffset = 0;
  uw_Basis_char c1;
  
  while(s[offset] != 0){
    oldoffset = offset;
    U8_NEXT(s, offset, -1, c1);
 
    if ((offset - oldoffset == 1) && uw_Basis_isprint(ctx, c1)) {
	
      switch (c1) {
      case '<':
	uw_write_unsafe(ctx, "&lt;");
	break;
      case '&':
	uw_write_unsafe(ctx, "&amp;");
	break;
      default:
	uw_writec_unsafe(ctx, c1);
      }
    }
    else {
      uw_Basis_htmlifySpecialChar_w(ctx, c1);
    }    
  }

  return uw_unit_v;
}

uw_Basis_string uw_Basis_htmlifyBool(uw_context ctx, uw_Basis_bool b) {
  (void)ctx;
  if (!b)
    return "False";
  else
    return "True";
}

uw_unit uw_Basis_htmlifyBool_w(uw_context ctx, uw_Basis_bool b) {
  if (!b) {
    uw_check(ctx, 6);
    strcpy(ctx->page.front, "False");
    ctx->page.front += 5;
  } else {
    uw_check(ctx, 5);
    strcpy(ctx->page.front, "True");
    ctx->page.front += 4;
  }

  return uw_unit_v;
}

#define TIME_FMT "%x %X"
#define TIME_FMT_PG "%Y-%m-%d %T"
#define TIME_FMT_JS "%Y/%m/%d %T"
#define TIME_FMT_NOSEC "%Y-%m-%d %H:%M"

uw_Basis_string uw_Basis_timeToString(uw_context, uw_Basis_time);

uw_Basis_string uw_Basis_htmlifyTime(uw_context ctx, uw_Basis_time t) {
  return uw_Basis_htmlifyString(ctx, uw_Basis_timeToString(ctx, t));
}

uw_unit uw_Basis_htmlifyTime_w(uw_context ctx, uw_Basis_time t) {
  return uw_Basis_htmlifyString_w(ctx, uw_Basis_timeToString(ctx, t));
}

char *uw_Basis_htmlifySource(uw_context ctx, uw_Basis_source src) {
  int len;
  char *r;

  uw_check_heap(ctx, 2 * INTS_MAX + 2);
  r = ctx->heap.front;
  sprintf(r, "s%d_%llu%n", src.context, src.source, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_unit uw_Basis_htmlifySource_w(uw_context ctx, uw_Basis_source src) {
  int len;

  uw_check(ctx, 2 * INTS_MAX + 1);
  sprintf(ctx->page.front, "s%d_%llu%n", src.context, src.source, &len);
  ctx->page.front += len;

  return uw_unit_v;
}

uw_Basis_char uw_Basis_strsubUtf8(uw_context ctx, uw_Basis_string s, uw_Basis_int n) {
  if (n < 0 || n >= strlen(s))
    uw_error(ctx, FATAL, "Out-of-bounds strsubUtf8");

  return (unsigned char)s[n];
}

uw_Basis_char uw_Basis_strsub(uw_context ctx, uw_Basis_string s, uw_Basis_int n) {
  uw_Basis_char c;
  int offset = 0;
  
  while (n >= 0) {
    
    if (s[offset] == 0)
      uw_error(ctx, FATAL, "Out-of-bounds strsub");

    U8_NEXT(s, offset, -1, c);
    
    if (n == 0)
      return c;

    --n;
  }

  uw_error(ctx, FATAL, "Negative strsub bound");
}

uw_Basis_string uw_Basis_strsuffixUtf8(uw_context ctx, uw_Basis_string s, uw_Basis_int n) {
  int offset = 0;
  while (n >= 0) {
    if (s[offset] == 0 || n == 0)
      return s + offset;

    ++s;
    --n;
  }

  uw_error(ctx, FATAL, "Negative strsuffixUtf8 bound");
}

uw_Basis_string uw_Basis_strsuffix(uw_context ctx, uw_Basis_string s, uw_Basis_int n) {
  int offset = 0;
  while (n >= 0) {
    if (s[offset] == 0 || n == 0)
      return s + offset;

    U8_FWD_1(s, offset, -1);
    --n;
  }

  uw_error(ctx, FATAL, "Negative strsuffix bound");
}

uw_Basis_int uw_Basis_strlenUtf8(uw_context ctx, uw_Basis_string s) {
  (void)ctx;
  return strlen(s);
}

uw_Basis_int uw_Basis_strlen(uw_context ctx, uw_Basis_string s) {
  (void)ctx;
  int offset = 0, iterations = 0;
  while (s[offset] != 0) {
    U8_FWD_1(s, offset, -1);
    ++iterations;
  }
  return iterations;
}

uw_Basis_bool uw_Basis_strlenGe(uw_context ctx, uw_Basis_string s, uw_Basis_int n) {
  (void)ctx;
  int offset = 0;
  while (n > 0) {
    if (s[offset] == 0)
      return uw_Basis_False;
        
    U8_FWD_1(s, offset, -1);
    --n;
  }

  return uw_Basis_True;
}

static int aux_strchr(uw_Basis_string s, uw_Basis_char ch, int *o_offset) {
  int u8idx = 0, offset = 0, offsetpr = 0;
  uw_Basis_char c;
    
  while (s[offset] != 0) {
    U8_NEXT(s, offset, -1, c);
    if (c == ch) {
      *o_offset = offsetpr;
      return u8idx;
    }

    offsetpr = offset;
    ++u8idx;
  }

  *o_offset = -1;
  return -1;
}

uw_Basis_string uw_Basis_strchr(uw_context ctx, uw_Basis_string s, uw_Basis_char ch) {
  (void)ctx;
  int offset = -1;
  if (aux_strchr(s, ch, &offset) > -1) {
    return s + offset;
  }
  return NULL;  
}

uw_Basis_int uw_Basis_strcspn(uw_context ctx, uw_Basis_string s, uw_Basis_string chs) {
  (void)ctx;
  int offset = 0, u8idx = 0, offsetChs = 0;
  uw_Basis_char c;
  
  while (s[offset] != 0) {
    U8_NEXT(s, offset, -1, c);
    if (aux_strchr(chs, c, &offsetChs) > -1) {
      return u8idx;
    }
    ++u8idx;
  }

  return u8idx;
}

uw_Basis_int *uw_Basis_strindex(uw_context ctx, uw_Basis_string s, uw_Basis_char ch) {
  (void)ctx;
  int offset = -1;
  int r = aux_strchr(s, ch, &offset);
  if (r == -1)
    return NULL;
  else {
    uw_Basis_int *nr = uw_malloc(ctx, sizeof(uw_Basis_int));
    *nr = r;
    return nr;
  }
}

uw_Basis_int *uw_Basis_strsindex(uw_context ctx, const char *haystack, const char *needle) {
  const char *r = strstr(haystack, needle);
  if (r == NULL)
    return NULL;
  else {
    uw_Basis_int *nr = uw_malloc(ctx, sizeof(uw_Basis_int));
    int src = r - haystack, offset = 0, utf8idx = 0;
    while (offset < src) {
      U8_FWD_1(haystack, offset, -1);
      ++utf8idx;
    }
    
    *nr = utf8idx;
    return nr;
  }
}

uw_Basis_string uw_Basis_strcat(uw_context ctx, uw_Basis_string s1, uw_Basis_string s2) {
  int len = strlen(s1) + strlen(s2) + 1;
  char *s;

  uw_check_heap(ctx, len);

  s = ctx->heap.front;

  strcpy(s, s1);
  strcat(s, s2);
  ctx->heap.front += len;

  return s;
}

uw_Basis_string uw_Basis_substring(uw_context ctx, uw_Basis_string s, uw_Basis_int start, uw_Basis_int len) {
  int full_len = uw_Basis_strlen(ctx, s);
  
  if (start < 0)
    uw_error(ctx, FATAL, "substring: Negative start index");
  if (len < 0)
    uw_error(ctx, FATAL, "substring: Negative length");
  if (start + len > full_len)
    uw_error(ctx, FATAL, "substring: Start index plus length is too large");

  int offset = 0;
  U8_FWD_N(s, offset, -1, start);
  
  if (start + len == full_len) {
    return s + offset;
  } else {
    int end = offset;
    U8_FWD_N(s, end, -1, len);

    int actual_len = end - offset;

    uw_Basis_string r = uw_malloc(ctx, actual_len + 1);
    memcpy(r, s + offset, actual_len);
    r[actual_len] = 0;
    return r;
  }
}

uw_Basis_string uw_Basis_str1(uw_context ctx, uw_Basis_char ch) {
  char *r;
  int req = U8_LENGTH(ch);
  int offset = 0;
  
  uw_check_heap(ctx, req + 1);
  r = ctx->heap.front;

  U8_APPEND_UNSAFE(r, offset, ch);  
  r[req] = 0;

  ctx->heap.front += req + 1;
  return r; 
}

uw_Basis_string uw_strdup(uw_context ctx, uw_Basis_string s1) {
  int len = strlen(s1) + 1;
  char *s;

  uw_check_heap(ctx, len);

  s = ctx->heap.front;

  strcpy(s, s1);
  ctx->heap.front += len;

  return s;
}

uw_Basis_string uw_dup_and_clear_error_message(uw_context ctx) {
  if (ctx->error_message[0]) {
    char *s = uw_strdup(ctx, ctx->error_message);
    ctx->error_message[0] = 0;
    return s;
  } else
    return NULL;
}

uw_Basis_string uw_maybe_strdup(uw_context ctx, uw_Basis_string s1) {
  if (s1)
    return uw_strdup(ctx, s1);
  else
    return NULL;
}

char *uw_memdup(uw_context ctx, const char *p, size_t len) {
  char *r = uw_malloc(ctx, len);
  memcpy(r, p, len);
  return r;
}

char *uw_sqlfmtInt = "%lld::int8%n";

char *uw_Basis_sqlifyInt(uw_context ctx, uw_Basis_int n) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX + 6);
  r = ctx->heap.front;
  sprintf(r, uw_sqlfmtInt, n, &len);
  ctx->heap.front += len+1;
  return r;
}

char *uw_Basis_sqlifyIntN(uw_context ctx, uw_Basis_int *n) {
  if (n == NULL)
    return "NULL";
  else
    return uw_Basis_sqlifyInt(ctx, *n);
}

char *uw_sqlfmtFloat = "%.16g::float8%n";

char *uw_Basis_sqlifyFloat(uw_context ctx, uw_Basis_float n) {
  int len;
  char *r;

  uw_check_heap(ctx, FLOATS_MAX + 8);
  r = ctx->heap.front;
  sprintf(r, uw_sqlfmtFloat, n, &len);
  ctx->heap.front += len+1;
  return r;
}

char *uw_Basis_sqlifyFloatN(uw_context ctx, uw_Basis_float *n) {
  if (n == NULL)
    return "NULL";
  else
    return uw_Basis_sqlifyFloat(ctx, *n);
}

int uw_Estrings = 1, uw_sql_type_annotations = 1, uw_sql_utc_times = 0;
char *uw_sqlsuffixString = "::text";
char *uw_sqlsuffixChar = "::char";
// The largest char that the database backend reads back as it was.  The
// MySQL backend reads a char as a single byte, so it sets 0x7F.
uw_Basis_char uw_sqlmaxChar = UCHAR_MAX_VALUE;

// A string as a quoted SQL literal, followed by suffix.
static uw_Basis_string sqlify_text(uw_context ctx, const char *s, const char *suffix) {
  char *r, *s2;

  // A byte takes up to four in the literal: an octal escape (\ooo) in an
  // E'' string, or else two for a doubled quote.  Then come the quotes, the
  // E, the suffix and the NUL.
  uw_check_heap(ctx, strlen(s) * (uw_Estrings ? 4 : 2) + 3 + uw_Estrings + strlen(suffix));

  r = s2 = ctx->heap.front;
  if (uw_Estrings)
    *s2++ = 'E';
  *s2++ = '\'';

  for (; *s; s++) {
    char c = *s;

    switch (c) {
    case '\'':
      if (uw_Estrings)
        strcpy(s2, "\\'");
      else
        strcpy(s2, "''");
      s2 += 2;
      break;
    case '\\':
      if (uw_Estrings) {
        strcpy(s2, "\\\\");
        s2 += 2;
      } else
        *s2++ = '\\';
      break;
    default:
      if (isprint((unsigned char)c))
        *s2++ = c;
      else if (uw_Estrings) {
        sprintf(s2, "\\%03o", (unsigned char)c);
        s2 += 4;
      }
      else
        *s2++ = c; // I hope this is safe to do... don't know how to support UTF-8 outside Postgres otherwise!
    }
  }

  *s2++ = '\'';
  strcpy(s2, suffix);
  ctx->heap.front = s2 + 1 + strlen(suffix);
  return r;
}

uw_Basis_string uw_Basis_sqlifyString(uw_context ctx, uw_Basis_string s) {
  return sqlify_text(ctx, s, uw_sqlsuffixString);
}

// A char goes into the database as a string of one character, in UTF-8 as
// any string does, and uw_Basis_unsqlChar reads it back.  NUL cannot be
// stored.  Text cannot hold it, and an empty text would not do instead,
// since PostgreSQL pads an empty char to a blank.
uw_Basis_string uw_Basis_sqlifyChar(uw_context ctx, uw_Basis_char c) {
  char s[U8_MAX_LENGTH + 1];
  int len = 0;

  if (c == 0)
    uw_error(ctx, FATAL, "Can't store the NUL character in the database");
  if ((uint32_t)c > UCHAR_MAX_VALUE || U_IS_SURROGATE(c))
    uw_error(ctx, FATAL, "Can't store character %u in the database: it has no UTF-8 encoding", (unsigned)c);
  if ((uint32_t)c > (uint32_t)uw_sqlmaxChar)
    uw_error(ctx, FATAL, "Can't store character %u: the database backend reads back no char beyond %u", (unsigned)c, (unsigned)uw_sqlmaxChar);

  U8_APPEND_UNSAFE(s, len, c);
  s[len] = 0;
  return sqlify_text(ctx, s, uw_sqlsuffixChar);
}

char *uw_sqlsuffixBlob = "::bytea";

uw_Basis_string uw_Basis_sqlifyBlob(uw_context ctx, uw_Basis_blob b) {
  char *r, *s2;
  size_t i;

  uw_check_heap(ctx, b.size * 5 + 4 + strlen(uw_sqlsuffixBlob));

  r = s2 = ctx->heap.front;
  if (uw_Estrings)
    *s2++ = 'E';
  else
    *s2++ = 'X';
  *s2++ = '\'';

  for (i = 0; i < b.size; ++i) {
    unsigned char c = b.data[i];

    if (uw_Estrings) {
      switch (c) {
      case '\'':
        strcpy(s2, "\\'");
        s2 += 2;
        break;
      case '\\':
        strcpy(s2, "\\\\\\\\");
        s2 += 4;
        break;
      default:
        if (isprint((int)c))
          *s2++ = c;
        else {
          sprintf(s2, "\\\\%03o", c);
          s2 += 5;
        }
      }
    } else {
      sprintf(s2, "%02X", c);
      s2 += 2;
    }
  }

  *s2++ = '\'';
  strcpy(s2, uw_sqlsuffixBlob);
  ctx->heap.front = s2 + 1 + strlen(uw_sqlsuffixBlob);
  return r;
}

char *uw_Basis_sqlifyChannel(uw_context ctx, uw_Basis_channel chn) {
  int len;
  char *r;
  unsigned long long combo = ((unsigned long long)chn.cli << 32) | chn.chn;

  uw_check_heap(ctx, INTS_MAX + 7);
  r = ctx->heap.front;
  sprintf(r, uw_sqlfmtInt, combo, &len);
  ctx->heap.front += len+1;
  return r;
}

char *uw_Basis_attrifyChannel(uw_context ctx, uw_Basis_channel chn) {
  int len;
  char *r;
  unsigned long long combo = ((unsigned long long)chn.cli << 32) | chn.chn;

  uw_check_heap(ctx, INTS_MAX + 1);
  r = ctx->heap.front;
  sprintf(r, "%lld%n", combo, &len);
  ctx->heap.front += len+1;
  return r;
}

char *uw_sqlfmtUint4 = "%u::int4%n";

char *uw_Basis_sqlifyClient(uw_context ctx, uw_Basis_client cli) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX + 7);
  r = ctx->heap.front;
  sprintf(r, uw_sqlfmtUint4, cli, &len);
  ctx->heap.front += len+1;
  return r;
}

char *uw_Basis_attrifyClient(uw_context ctx, uw_Basis_client cli) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX + 1);
  r = ctx->heap.front;
  sprintf(r, "%u%n", cli, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_Basis_string uw_Basis_sqlifyStringN(uw_context ctx, uw_Basis_string s) {
  if (s == NULL)
    return "NULL";
  else
    return uw_Basis_sqlifyString(ctx, s);
}

char *uw_Basis_sqlifyBool(uw_context ctx, uw_Basis_bool b) {
  (void)ctx;
  if (!b)
    return "FALSE";
  else
    return "TRUE";
}

char *uw_Basis_sqlifyBoolN(uw_context ctx, uw_Basis_bool *b) {
  if (b == NULL)
    return "NULL";
  else
    return uw_Basis_sqlifyBool(ctx, *b);
}

// The years of a time SQLite's date functions read: four digits.
static const long long sql_year_min = 0, sql_year_max = 9999;

// A time as SQLite keeps it, having no type for times: text in UTC,
// "YYYY-MM-DD HH:MM:SS" as CURRENT_TIMESTAMP gives it, and the microseconds
// as ".ffffff" after the seconds if there are any.  Texts of this form sort
// as their times do, so SQL can compare times as it compares text.  A time
// whose year has no four digits is refused, rather than stored as text that
// nothing reads back.
char *uw_Basis_ensqlTimeUtc(uw_context ctx, uw_Basis_time t) {
  struct tm stm;
  long long year;
  int len;
  char *r;

  if (!gmtime_r(&t.seconds, &stm))
    uw_error(ctx, FATAL, "Can't store time %lld in the database", (long long)t.seconds);
  year = stm.tm_year + 1900LL;
  if (year < sql_year_min || year > sql_year_max)
    uw_error(ctx, FATAL, "Can't store a time in the year %lld in the database: SQLite takes the years %lld to %lld",
             year, sql_year_min, sql_year_max);

  uw_check_heap(ctx, TIMES_MAX);
  r = ctx->heap.front;
  len = snprintf(r, TIMES_MAX, "%04lld-%02d-%02d %02d:%02d:%02d",
                 year, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
  if (t.microseconds)
    len += snprintf(r + len, TIMES_MAX - len, ".%06u", t.microseconds);
  ctx->heap.front += len + 1;
  return r;
}

static int sql_time_digits(const char **p, int n, int *v) {
  int i;

  *v = 0;
  for (i = 0; i < n; ++i) {
    if (!isdigit((unsigned char)(*p)[i]))
      return 0;
    *v = *v * 10 + ((*p)[i] - '0');
  }
  *p += n;
  return 1;
}

// A time SQLite holds as text, in any form its date functions take with a
// date: "YYYY-MM-DD", or that with "HH:MM", "HH:MM:SS" or "HH:MM:SS.SSS"
// (any number of digits) after a blank or a T, and then a time zone "Z",
// "+HH:MM" or "-HH:MM" if any.  Without a time zone it is UTC, as
// uw_Basis_ensqlTimeUtc and CURRENT_TIMESTAMP write it.
uw_Basis_time uw_Basis_unsqlTimeUtc(uw_context ctx, const char *s) {
  const char *p = s;
  int year, month, day, hour = 0, minute = 0, second = 0, zone = 0;
  unsigned microseconds = 0;
  struct tm stm = {};

  if (!sql_time_digits(&p, 4, &year) || *p != '-')
    goto bad;
  ++p;
  if (!sql_time_digits(&p, 2, &month) || *p != '-')
    goto bad;
  ++p;
  if (!sql_time_digits(&p, 2, &day))
    goto bad;

  if (*p == ' ' || *p == 'T') {
    ++p;
    if (!sql_time_digits(&p, 2, &hour) || *p != ':')
      goto bad;
    ++p;
    if (!sql_time_digits(&p, 2, &minute))
      goto bad;

    if (*p == ':') {
      ++p;
      if (!sql_time_digits(&p, 2, &second))
        goto bad;

      if (*p == '.') {
        int n;

        ++p;
        if (!isdigit((unsigned char)*p))
          goto bad;
        for (n = 0; isdigit((unsigned char)*p); ++p, ++n)
          if (n < 6)
            microseconds = microseconds * 10 + (*p - '0');
        for (; n < 6; ++n)
          microseconds *= 10;
      }
    }

    if (*p == 'Z')
      ++p;
    else if (*p == '+' || *p == '-') {
      int sign = *p == '-' ? -1 : 1, zh, zm;

      ++p;
      if (!sql_time_digits(&p, 2, &zh) || *p != ':')
        goto bad;
      ++p;
      if (!sql_time_digits(&p, 2, &zm) || zh > 23 || zm > 59)
        goto bad;
      zone = sign * (zh * 3600 + zm * 60);
    }
  }

  if (*p || month < 1 || month > 12 || day < 1 || day > 31
      || hour > 23 || minute > 59 || second > 59)
    goto bad;

  stm.tm_year = year - 1900;
  stm.tm_mon = month - 1;
  stm.tm_mday = day;
  stm.tm_hour = hour;
  stm.tm_min = minute;
  stm.tm_sec = second;
  {
    // The zone is how far the time given is ahead of UTC.
    uw_Basis_time r = { timegm(&stm) - zone, microseconds };
    return r;
  }

 bad:
  uw_error(ctx, FATAL, "Can't parse time: %s", uw_Basis_htmlifyString(ctx, (uw_Basis_string)s));
}

char *uw_Basis_sqlifyTime(uw_context ctx, uw_Basis_time t) {
  size_t len;
  char *r, *s;
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (uw_sql_utc_times) {
    s = uw_Basis_ensqlTimeUtc(ctx, t);
    r = uw_malloc(ctx, strlen(s) + 3);
    sprintf(r, "'%s'", s);
    return r;
  }

  if (localtime_r(&t.seconds, &stm)) {
    s = uw_malloc(ctx, TIMES_MAX);
    len = strftime(s, TIMES_MAX, TIME_FMT_PG, &stm);
    if (uw_sql_type_annotations) {
      if (t.microseconds) {
        r = uw_malloc(ctx, len + 21);
        sprintf(r, "'%s.%06u'::timestamp", s, t.microseconds);
      } else {
        r = uw_malloc(ctx, len + 14);
        sprintf(r, "'%s'::timestamp", s);
      }
    } else {
      r = uw_malloc(ctx, len + 3);
      sprintf(r, "'%s'", s);
    }
    return r;
  } else
    return "<Invalid time>";
}

char *uw_Basis_attrifyTime(uw_context ctx, uw_Basis_time t) {
  size_t len;
  char *r;
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (localtime_r(&t.seconds, &stm)) {
    uw_check_heap(ctx, TIMES_MAX);
    r = ctx->heap.front;
    len = strftime(r, TIMES_MAX, TIME_FMT, &stm);
    ctx->heap.front += len+1;
    return r;
  } else
    return "<Invalid time>";
}

char *uw_Basis_ensqlTime(uw_context ctx, uw_Basis_time t) {
  size_t len;
  char *r;
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (localtime_r(&t.seconds, &stm)) {
    uw_check_heap(ctx, TIMES_MAX);
    r = ctx->heap.front;
    len = strftime(r, TIMES_MAX-7, TIME_FMT_PG, &stm);
    ctx->heap.front += len;
    sprintf(ctx->heap.front, ".%06u", t.microseconds);
    ctx->heap.front += 8;
    return r;
  } else
    return "<Invalid time>";
}

char *uw_Basis_sqlifyTimeN(uw_context ctx, uw_Basis_time *t) {
  if (t == NULL)
    return "NULL";
  else
    return uw_Basis_sqlifyTime(ctx, *t);
}

char *uw_Basis_ensqlBool(uw_Basis_bool b) {
  static uw_Basis_int tru = 1;
  static uw_Basis_int fals = 0;

  if (!b)
    return (char *)&fals;
  else
    return (char *)&tru;
}

uw_Basis_string uw_Basis_intToString(uw_context ctx, uw_Basis_int n) {
  int len;
  char *r;

  uw_check_heap(ctx, INTS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%lld%n", n, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_Basis_string uw_Basis_floatToString(uw_context ctx, uw_Basis_float n) {
  int len;
  char *r;

  uw_check_heap(ctx, FLOATS_MAX);
  r = ctx->heap.front;
  sprintf(r, "%g%n", n, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_Basis_string uw_Basis_charToString(uw_context ctx, uw_Basis_char ch) {
  return uw_Basis_str1(ctx, ch);
}

uw_Basis_string uw_Basis_boolToString(uw_context ctx, uw_Basis_bool b) {
  (void)ctx;
  if (!b)
    return "False";
  else
    return "True";
}

uw_Basis_string uw_Basis_timef(uw_context ctx, const char *fmt, uw_Basis_time t) {
  size_t len;
  char *r;
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (localtime_r(&t.seconds, &stm)) {
    uw_check_heap(ctx, TIMES_MAX);
    r = ctx->heap.front;
    len = strftime(r, TIMES_MAX, fmt, &stm);
    ctx->heap.front += len+1;
    return r;
  } else
    return "<Invalid time>";
}

uw_Basis_string uw_Basis_timeToString(uw_context ctx, uw_Basis_time t) {
  return uw_Basis_timef(ctx, ctx->app->time_format, t);
}

uw_Basis_int *uw_Basis_stringToInt(uw_context ctx, uw_Basis_string s) {
  char *endptr;
  uw_Basis_int n = strtoll(s, &endptr, 10);

  if (*s != '\0' && *endptr == '\0') {
    uw_Basis_int *r = uw_malloc(ctx, sizeof(uw_Basis_int));
    *r = n;
    return r;
  } else
    return NULL;
}

uw_Basis_float *uw_Basis_stringToFloat(uw_context ctx, uw_Basis_string s) {
  char *endptr;
  uw_Basis_float n = strtod(s, &endptr);

  if (*s != '\0' && *endptr == '\0') {
    uw_Basis_float *r = uw_malloc(ctx, sizeof(uw_Basis_float));
    *r = n;
    return r;
  } else
    return NULL;
}

uw_Basis_char *uw_Basis_stringToChar(uw_context ctx, uw_Basis_string s) {
  if (s[0] == 0) {
    uw_Basis_char *r = uw_malloc(ctx, sizeof *r);
    r[0] = 0;
    return r;
  } else if (uw_Basis_strlenGe(ctx, s, 2))
    return NULL;
  else {
    uw_Basis_char *r = uw_malloc(ctx, sizeof *r);
    int offset = 0;
    U8_NEXT(s, offset, -1, *r);
    return r;
  }
}

uw_Basis_bool *uw_Basis_stringToBool(uw_context ctx, uw_Basis_string s) {
  (void)ctx;
  static uw_Basis_bool tru = uw_Basis_True;
  static uw_Basis_bool fals = uw_Basis_False;

  if (!strcasecmp (s, "True"))
    return &tru;
  else if (!strcasecmp (s, "False"))
    return &fals;
  else
    return NULL;
}

uw_Basis_time *uw_Basis_stringToTime(uw_context ctx, uw_Basis_string s) {
  char *dot = strchr(s, '.'), *end = strchr(s, 0);
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (dot) {
    *dot = 0;
    if (strptime(s, TIME_FMT_PG, &stm) == end) {
      *dot = '.';
      uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));
      r->seconds = mktime(&stm);
      r->microseconds = 0;
      return r;
    }
    else {
      *dot = '.';
      return NULL;
    }
  }
  else {
    if (strptime(s, ctx->app->time_format, &stm) == end) {
      uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));
      r->seconds = mktime(&stm);
      r->microseconds = 0;
      return r;
    } else if (strptime(s, TIME_FMT_PG, &stm) == end) {
      uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));
      r->seconds = mktime(&stm);
      r->microseconds = 0;
      return r;
    } else if (strptime(s, TIME_FMT, &stm) == end) {
      uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));
      r->seconds = mktime(&stm);
      r->microseconds = 0;
      return r;
    } else if (strptime(s, TIME_FMT_JS, &stm) == end) {
      uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));
      r->seconds = mktime(&stm);
      r->microseconds = 0;
      return r;
    }
    else
      return NULL;
  }
}

uw_Basis_time *uw_Basis_stringToTimef(uw_context ctx, const char *fmt, uw_Basis_string s) {
  char *end = strchr(s, 0);
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (strptime(s, fmt, &stm) == end) {
    uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));
    r->seconds = mktime(&stm);
    r->microseconds = 0;
    return r;
  }
  else
    return NULL;
}

uw_Basis_int uw_Basis_stringToInt_error(uw_context ctx, uw_Basis_string s) {
  char *endptr;
  uw_Basis_int n = strtoll(s, &endptr, 10);

  if (*s != '\0' && *endptr == '\0')
    return n;
  else
    uw_error(ctx, FATAL, "Can't parse int: %s", uw_Basis_htmlifyString(ctx, s));
}

#include <errno.h>

uw_Basis_channel uw_Basis_stringToChannel_error(uw_context ctx, uw_Basis_string s) {
  unsigned long long n;

  if (sscanf(s, "%llu", &n) < 1)
    uw_error(ctx, FATAL, "Can't parse channel: %s", uw_Basis_htmlifyString(ctx, s));
  else {
    uw_Basis_channel ch = {n >> 32, n & ((1ull << 32) - 1)};
    return ch;
  }
}

uw_Basis_client uw_Basis_stringToClient_error(uw_context ctx, uw_Basis_string s) {
  char *endptr;
  unsigned long n = strtoul(s, &endptr, 10);

  if (*s != '\0' && *endptr == '\0')
    return n;
  else
    uw_error(ctx, FATAL, "Can't parse client: %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_float uw_Basis_stringToFloat_error(uw_context ctx, uw_Basis_string s) {
  char *endptr;
  uw_Basis_float n = strtod(s, &endptr);

  if (*s != '\0' && *endptr == '\0')
    return n;
  else
    uw_error(ctx, FATAL, "Can't parse float: %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_char uw_Basis_stringToChar_error(uw_context ctx, uw_Basis_string s) {
  if (s[0] == 0)
    return 0;
  else if (uw_Basis_strlenGe(ctx, s, 2))
    uw_error(ctx, FATAL, "Can't parse char: %s", uw_Basis_htmlifyString(ctx, s));
  else {
    uw_Basis_char c;
    int offset = 0;
    U8_NEXT(s, offset, -1, c);
    return c;
  }
}

uw_Basis_bool uw_Basis_stringToBool_error(uw_context ctx, uw_Basis_string s) {
  if (!strcasecmp(s, "T") || !strcasecmp (s, "True"))
    return uw_Basis_True;
  else if (!strcasecmp(s, "F") || !strcasecmp (s, "False"))
    return uw_Basis_False;
  else
    uw_error(ctx, FATAL, "Can't parse bool: %s", uw_Basis_htmlifyString(ctx, s));
}

// A char from the database: the first character of the text, as
// uw_Basis_sqlifyChar writes it, or NUL for an empty text.
uw_Basis_char uw_Basis_unsqlChar(uw_context ctx, const char *s) {
  uw_Basis_char c;
  int offset = 0;

  if (s[0] == 0)
    return 0;
  U8_NEXT(s, offset, -1, c);
  if (c < 0)
    uw_error(ctx, FATAL, "Can't read a char from the database: its text is not UTF-8");
  return c;
}

uw_Basis_time uw_Basis_unsqlTime(uw_context ctx, uw_Basis_string s) {
  char *dot = strchr(s, '.'), *end = strchr(s, 0);
  int dot_is_not_really_dot = 0;
  struct tm stm = {};
  stm.tm_isdst = -1;

  // Extra logic to skip Postgres content that we want to ignore
  if (!dot) {
    dot = strchr(s, ' ');
    if (dot) {
      dot = strchr(dot, '-');
      if (dot)
        dot_is_not_really_dot = 1;
    }
  }

  if (dot) {
    *dot = 0;
    if (strptime(s, TIME_FMT_PG, &stm)) {
      *dot = dot_is_not_really_dot ? '-' : '.';
      char usec[] = "000000";
      if (!dot_is_not_really_dot) {
        int len = strlen(dot+1);
        memcpy(usec, dot+1, len < 6 ? len : 6);
      }
      uw_Basis_time r = { mktime(&stm), atoi(usec) };
      return r;
    }
    else {
      *dot = '.';
      uw_error(ctx, FATAL, "Can't parse time: %s", uw_Basis_htmlifyString(ctx, s));
    }
  }
  else {
    if (strptime(s, TIME_FMT_PG, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else if (strptime(s, TIME_FMT, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else if (strptime(s, TIME_FMT_NOSEC, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else
      uw_error(ctx, FATAL, "Can't parse time: %s", uw_Basis_htmlifyString(ctx, s));
  }
}

uw_Basis_time uw_Basis_stringToTime_error(uw_context ctx, uw_Basis_string s) {
  char *dot = strchr(s, '.'), *end = strchr(s, 0);
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (dot) {
    *dot = 0;
    if (strptime(s, TIME_FMT_PG, &stm)) {
      *dot = '.';
      {
        uw_Basis_time r = { mktime(&stm) };
        return r;
      }
    }
    else {
      *dot = '.';
      uw_error(ctx, FATAL, "Can't parse time: %s", uw_Basis_htmlifyString(ctx, s));
    }
  }
  else {
    if (strptime(s, ctx->app->time_format, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else if (strptime(s, TIME_FMT_PG, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else if (strptime(s, TIME_FMT, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else if (strptime(s, TIME_FMT_JS, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else if (strptime(s, TIME_FMT_NOSEC, &stm) == end) {
      uw_Basis_time r = { mktime(&stm) };
      return r;
    } else
      uw_error(ctx, FATAL, "Can't parse time: %s", uw_Basis_htmlifyString(ctx, s));
  }
}

uw_Basis_time uw_Basis_stringToTimef_error(uw_context ctx, const char *fmt, uw_Basis_string s) {
  char *end = strchr(s, 0);
  struct tm stm = {};
  stm.tm_isdst = -1;

  if (strptime(s, fmt, &stm) == end) {
    uw_Basis_time r = { mktime(&stm) };
    return r;
  } else if (strptime(s, TIME_FMT_NOSEC, &stm) == end) {
    uw_Basis_time r = { mktime(&stm) };
    return r;
  } else
    uw_error(ctx, FATAL, "Can't parse time: %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_blob uw_Basis_stringToBlob_error(uw_context ctx, uw_Basis_string s, size_t len) {
  char *r = ctx->heap.front;
  uw_Basis_blob b = {len, r};

  uw_check_heap(ctx, len);

  if (s[0] == '\\' && s[1] == 'x') {
    s += 2;

    while (*s) {
      char a = s[0];
      s += 1;
      char b;
      if (*s){
        b = s[0];
      } else {
        b = 0;
      }
      int n;
      char buf[3] = {a, b, 0};
      n = strtol(buf, NULL, 16);
      *r++ = n;
      s += 1;
    }
  } else {
    while (*s) {
      if (s[0] == '\\') {
        if (s[1] == '\\') {
          *r++ = '\\';
          s += 2;
        } else if (isdigit((unsigned char)s[1]) && isdigit((unsigned char)s[2]) && isdigit((unsigned char)s[3])) {
          *r++ = (s[1] - '0') * 8 * 8 + ((s[2] - '0') * 8) + (s[3] - '0');
          s += 4;
        }
        else {
          *r++ = '\\';
          ++s;
        }
      } else {
        *r++ = s[0];
        ++s;
      }
    }
  }

  b.size = r - ctx->heap.front;
  ctx->heap.front = r;

  return b;
}

#define THE_PAST "expires=Sat, 01-Jan-2011 00:00:00 GMT"

uw_Basis_string uw_Basis_get_cookie(uw_context ctx, uw_Basis_string c) {
  int len = strlen(c);
  char *p = ctx->outHeaders.start;

  while ((p = strstr(p, "\nSet-Cookie: "))) {
    char *p2;
    p += 13;
    p2 = strchr(p, '=');

    if (p2) {
      size_t sz = strcspn(p2+1, ";\r\n");

      if (!strncasecmp(p, c, p2 - p)) {
        if (sz == 0 && strstr(p2+2, THE_PAST))
          return NULL;
        else {
          char *ret = uw_malloc(ctx, sz + 1);
          memcpy(ret, p2+1, sz);
          ret[sz] = 0;
          return ret;
        }
      }
    }
  }

  if ((p = uw_Basis_requestHeader(ctx, "Cookie"))) {
    char *p2;

    while (1) {
      if (!strncmp(p, c, len) && p[len] == '=') {
        if ((p2 = strchr(p, ';'))) {
          size_t n = p2 - (p + len);
          char *r = uw_malloc(ctx, n);
          memcpy(r, p + 1 + len, n-1);
          r[n-1] = 0;
          return r;
        } else
          return p + 1 + len;
      } else if ((p = strchr(p, ';')))
        p += 2;
      else
        return NULL;
    }
  }

  return NULL;
}

static void set_cookie(uw_context ctx) {
  if (ctx->usedSig)
    ctx->needsResig = 1;
}

uw_unit uw_Basis_set_cookie(uw_context ctx, uw_Basis_string prefix, uw_Basis_string c, uw_Basis_string v, uw_Basis_time *expires, uw_Basis_bool secure, uw_Basis_bool httponly) {
  uw_write_header(ctx, "Set-Cookie: ");
  uw_write_header(ctx, c);
  uw_write_header(ctx, "=");
  uw_write_header(ctx, v);
  uw_write_header(ctx, "; path=");
  uw_write_header(ctx, prefix);
  if (expires) {
    char formatted[30];
    struct tm tm = {};
    tm.tm_isdst = -1;

    gmtime_r(&expires->seconds, &tm);

    strftime(formatted, sizeof formatted, "%a, %d-%b-%Y %T GMT", &tm);

    uw_write_header(ctx, "; expires=");
    uw_write_header(ctx, formatted);
  }
  if (secure)
    uw_write_header(ctx, "; secure");
  if (httponly)
    uw_write_header(ctx, "; httponly");
  uw_write_header(ctx, "\r\n");
  set_cookie(ctx);

  return uw_unit_v;
}

uw_unit uw_Basis_clear_cookie(uw_context ctx, uw_Basis_string prefix, uw_Basis_string c) {
  uw_write_header(ctx, "Set-Cookie: ");
  uw_write_header(ctx, c);
  uw_write_header(ctx, "=; path=");
  uw_write_header(ctx, prefix);
  uw_write_header(ctx, "; " THE_PAST "\r\n");
  set_cookie(ctx);

  return uw_unit_v;
}

size_t uw_deltas_max = SIZE_MAX;

static delta *allocate_delta(uw_context ctx, unsigned client) {
  unsigned i;
  delta *d;

  for (i = 0; i < ctx->used_deltas; ++i)
    if (ctx->deltas[i].client == client)
      return &ctx->deltas[i];

  if (ctx->used_deltas >= ctx->n_deltas) {
    if (ctx->n_deltas + 1 > uw_deltas_max)
      uw_error(ctx, FATAL, "Exceeded limit on number of deltas");

    ctx->deltas = realloc(ctx->deltas, sizeof(delta) * ++ctx->n_deltas);
    uw_buffer_init(uw_messages_max, &ctx->deltas[ctx->n_deltas-1].msgs, 0);
  }

  d = &ctx->deltas[ctx->used_deltas++];
  d->client = client;
  uw_buffer_reset(&d->msgs);
  return d;
}

uw_Basis_channel uw_Basis_new_channel(uw_context ctx, uw_unit u) {
  (void)u;

  if (ctx->client == NULL)
    uw_error(ctx, FATAL, "Attempt to create channel on request not associated with a persistent connection");

  return new_channel(ctx->client);
}

uw_unit uw_Basis_send(uw_context ctx, uw_Basis_channel chn, uw_Basis_string msg) {
  delta *d = allocate_delta(ctx, chn.cli);
  size_t len;
  int preLen;
  char pre[INTS_MAX + 2];

  len = strlen(msg);

  sprintf(pre, "%u\n%n", chn.chn, &preLen);

  ctx_uw_buffer_append(ctx, "messages", &d->msgs, pre, preLen);
  ctx_uw_buffer_append(ctx, "messages", &d->msgs, msg, len);
  ctx_uw_buffer_append(ctx, "messages", &d->msgs, "\n", 1);

  return uw_unit_v;
}

// Release the client a transaction held, at its commit or rollback -- unless
// the transaction is one of an io function run by a request
// (uw_io_request_begin), whose client is the request's: it stays held, and
// named in the context, until the request itself commits or rolls back.
static void release_client_of(uw_context ctx) {
  if (ctx->client && !ctx->io_request)
    release_client(ctx->client);
}

// The transactionals the transaction registered, those above the mark
// (transactionals_base, zero but for a transaction of io code), latest
// first: their free callbacks; their rollback callbacks and then their free
// callbacks; their commit callbacks, those with a rollback callback or those
// without, which is 0 once one of them reports an error.
static void uw_free_transactionals(uw_context ctx, int will_retry) {
  int i;

  for (i = ctx->used_transactionals-1; i >= (int)ctx->transactionals_base; --i)
    if (ctx->transactionals[i].free)
      ctx->transactionals[i].free(ctx->transactionals[i].data, will_retry);
}

static void uw_undo_transactionals(uw_context ctx, int will_retry) {
  int i;

  for (i = ctx->used_transactionals-1; i >= (int)ctx->transactionals_base; --i)
    if (ctx->transactionals[i].rollback != NULL)
      ctx->transactionals[i].rollback(ctx->transactionals[i].data);

  uw_free_transactionals(ctx, will_retry);
}

static int uw_commit_transactionals(uw_context ctx, int with_rollback) {
  int i;

  for (i = ctx->used_transactionals-1; i >= (int)ctx->transactionals_base; --i)
    if ((ctx->transactionals[i].rollback != NULL) == with_rollback && ctx->transactionals[i].commit) {
      ctx->transactionals[i].commit(ctx->transactionals[i].data);
      if (uw_has_error(ctx))
        return 0;
    }

  return 1;
}

int uw_rollback(uw_context ctx, int will_retry) {
  release_client_of(ctx);
  uw_run_cleanups(ctx);
  uw_undo_transactionals(ctx, will_retry);

  if (ctx->app && ctx->transaction_started) {
    ctx->transaction_started = 0;
    return ctx->app->db_rollback(ctx);
  } else
    return 0;
}

const char uw_begin_xhtml[] = "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n<html xmlns=\"http://www.w3.org/1999/xhtml\" xml:lang=\"en\" lang=\"en\">";
const char uw_begin_html5[] = "<!DOCTYPE html><html>";

extern int uw_hash_blocksize;

static const char sig_intro[] = "<input type=\"hidden\" name=\"Sig\" value=\"";

static char *find_sig(char *haystack) {
  int i;
  char *s = strstr(haystack, sig_intro);

  if (!s || strlen(haystack) - (s - haystack) - (sizeof sig_intro - 1) < uw_hash_blocksize*2+1)
    return NULL;

  s += sizeof sig_intro - 1;

  for (i = 0; i < uw_hash_blocksize*2; ++i)
    if (!isxdigit((unsigned char)s[i]))
      return NULL;

  if (s[i] != '"')
    return NULL;

  return s;
}

static pthread_mutex_t message_send_mutex = PTHREAD_MUTEX_INITIALIZER;

int uw_commit(uw_context ctx) {
  int i;
  char *sig;

  ctx->committed = 0;

  if (uw_has_error(ctx) || !uw_commit_transactionals(ctx, 1)) {
    uw_rollback(ctx, 0);
    return 0;
  }

  // Here's an important lock to provide the abstraction that all messages from one transaction are sent as an atomic unit.
  if (ctx->used_deltas > 0)
    pthread_mutex_lock(&message_send_mutex);

  if (ctx->transaction_started) {
    int code = ctx->app->db_commit(ctx);

    if (code) {
      if (ctx->used_deltas > 0)
        pthread_mutex_unlock(&message_send_mutex);

      release_client_of(ctx);

      if (code == -1) {
        // This case is for a serialization failure, which is not really an "error."
        // The transaction will restart, so we should rollback any transactionals
        // that triggered above.
        uw_undo_transactionals(ctx, 1);

        // The database has ended the transaction.  A retry has to BEGIN again,
        // which uw_ensure_transaction does only while this is clear, and the
        // retry of a request resets with uw_reset_keep_request, which leaves it.
        ctx->transaction_started = 0;

        return 1;
      }

      uw_free_transactionals(ctx, 0);

      uw_set_error_message(ctx, "Error running SQL COMMIT");
      return 0;
    }
  }

  ctx->transaction_started = 0;
  ctx->committed = 1;

  if (!uw_commit_transactionals(ctx, 0)) {
    if (ctx->used_deltas > 0)
      pthread_mutex_unlock(&message_send_mutex);

    release_client_of(ctx);
    uw_undo_transactionals(ctx, 0);

    return 0;
  }

  for (i = 0; i < ctx->used_deltas; ++i) {
    delta *d = &ctx->deltas[i];
    client *c = find_client(d->client);

    assert (c != NULL);

    client_send(ctx, c, &d->msgs, ctx->script.start, uw_buffer_used(&ctx->script));
  }

  if (ctx->used_deltas > 0)
    pthread_mutex_unlock(&message_send_mutex);

  release_client_of(ctx);
  uw_free_transactionals(ctx, 0);

  uw_check(ctx, 1);
  *ctx->page.front = 0;

  if (!ctx->returning_indirectly
      && (ctx->app->is_html5
          ? !strncmp(ctx->page.start, uw_begin_html5, sizeof uw_begin_html5 - 1)
          : !strncmp(ctx->page.start, uw_begin_xhtml, sizeof uw_begin_xhtml - 1))) {
    char *s;

    // Splice script data into appropriate part of page, also adding <head> if needed.
    s = ctx->page.start + (ctx->app->is_html5 ? sizeof uw_begin_html5 - 1 : sizeof uw_begin_xhtml - 1);
    s = strchr(s, '<');
    if (s == NULL) {
      // Weird.  Document has no tags!

      uw_write(ctx, "<head></head><body></body>");
      uw_check(ctx, 1);
      *ctx->page.front = 0;
    } else if (!strncmp(s, "<head>", 6)) {
      // <head> is present.  Let's add the <script> tags immediately after it.

      // Any freeform JavaScript to include?
      if (uw_buffer_used(&ctx->script) > 0) {
        size_t lenH = strlen(ctx->script_header), len = uw_buffer_used(&ctx->script);
        size_t lenP = lenH + 40 + len;
        size_t startOff = s + 6 - ctx->page.start;
        char *start;

        ctx_uw_buffer_check(ctx, "page", &ctx->page, uw_buffer_used(&ctx->page) + lenP);
        start = ctx->page.start + startOff;
        memmove(start + lenP, start, uw_buffer_used(&ctx->page) - (start - ctx->page.start) + 1);
        ctx->page.front += lenP;
        memcpy(start, ctx->script_header, lenH);
        memcpy(start + lenH, "<script type=\"text/javascript\">", 31);
        memcpy(start + lenH + 31, ctx->script.start, len);
        memcpy(start + lenH + 31 + len, "</script>", 9);
      } else {
        size_t lenH = strlen(ctx->script_header);
        size_t startOff = s + 6 - ctx->page.start;
        char *start;

        ctx_uw_buffer_check(ctx, "page", &ctx->page, uw_buffer_used(&ctx->page) + lenH);
        start = ctx->page.start + startOff;
        memmove(start + lenH, start, uw_buffer_used(&ctx->page) - (start - ctx->page.start) + 1);
        ctx->page.front += lenH;
        memcpy(start, ctx->script_header, lenH);
      }
    } else {
      // No <head>.  At this point, add it, with <script> tags inside.

      if (uw_buffer_used(&ctx->script) > 0) {
        size_t lenH = strlen(ctx->script_header), len = uw_buffer_used(&ctx->script);
        size_t lenP = lenH + 53 + len;
        size_t startOff = s - ctx->page.start;
        char *start;

        ctx_uw_buffer_check(ctx, "page", &ctx->page, uw_buffer_used(&ctx->page) + lenP);
        start = ctx->page.start + startOff;
        memmove(start + lenP, start, uw_buffer_used(&ctx->page) - (start - ctx->page.start) + 1);
        ctx->page.front += lenP;
        memcpy(start, "<head>", 6);
        memcpy(start + 6, ctx->script_header, lenH);
        memcpy(start + 6 + lenH, "<script type=\"text/javascript\">", 31);
        memcpy(start + 6 + lenH + 31, ctx->script.start, len);
        memcpy(start + 6 + lenH + 31 + len, "</script></head>", 16);
      } else {
        size_t lenH = strlen(ctx->script_header);
        size_t lenP = lenH + 13;
        size_t startOff = s - ctx->page.start;
        char *start;

        ctx_uw_buffer_check(ctx, "page", &ctx->page, uw_buffer_used(&ctx->page) + lenP);
        start = ctx->page.start + startOff;
        memmove(start + lenP, start, uw_buffer_used(&ctx->page) - (start - ctx->page.start) + 1);
        ctx->page.front += lenP;
        memcpy(start, "<head>", 6);
        memcpy(start + 6, ctx->script_header, lenH);
        memcpy(start + 6 + lenH, "</head>", 7);
      }
    }
  }

  if (ctx->needsResig) {
    sig = find_sig(ctx->page.start);
    if (sig) {
      char *realsig = ctx->app->cookie_sig(ctx);

      do {
        memcpy(sig, realsig, 2*uw_hash_blocksize);
        sig = find_sig(sig);
      } while (sig);
    }
  }

  ctx->file_cache_missed = 0;

  return 0;
}


size_t uw_transactionals_max = SIZE_MAX;

int uw_register_transactional(uw_context ctx, void *data, uw_callback commit, uw_callback rollback,
                               uw_callback_with_retry free) {
  if (ctx->used_transactionals >= ctx->n_transactionals) {
    if (ctx->used_transactionals+1 > uw_transactionals_max)
      // Exceeded limit on number of transactionals.
      return -1;
    ctx->transactionals = realloc(ctx->transactionals, sizeof(transactional) * (ctx->used_transactionals+1));
    ++ctx->n_transactionals;
  }

  ctx->transactionals[ctx->used_transactionals].data = data;
  ctx->transactionals[ctx->used_transactionals].commit = commit;
  ctx->transactionals[ctx->used_transactionals].rollback = rollback;
  ctx->transactionals[ctx->used_transactionals++].free = free;

  return 0;
}


// "Garbage collection"

void uw_do_expunge(uw_context ctx, uw_Basis_client cli, void *data);
void uw_post_expunge(uw_context ctx, void *data);

static failure_kind uw_expunge(uw_context ctx, uw_Basis_client cli, void *data) {
  int r = setjmp(ctx->jmp_buf);

  if (r == 0) {
    // The expunger deletes the rows that hold the client: its transaction
    // always writes.  It does not wait for a database that another process
    // holds, since every request waits for the pruner meanwhile: it fails,
    // and the client is tried again at the next pass.
    uw_set_could_write_db(ctx, 1);
    ctx->db_may_wait = 0;
    uw_do_expunge(ctx, cli, data);
    // The commit can fail without raising.  Before the database's COMMIT the
    // transaction was rolled back, and at COMMIT it may still be open: either
    // way nothing was written, so the expunge failed.  After COMMIT, in a
    // commit callback, it is done (see uw_prune_clients).
    if (uw_has_error(ctx) && !ctx->committed) {
      if (ctx->transaction_started) {
        ctx->transaction_started = 0;
        ctx->app->db_rollback(ctx);
      }
      r = FATAL;
    }
  }
  // A BEGIN that found the database busy fails before any transaction
  // exists, and a conflict at COMMIT has ended it: nothing to roll back.
  else if (ctx->transaction_started) {
    ctx->transaction_started = 0;
    ctx->app->db_rollback(ctx);
  }
  ctx->db_may_wait = 1;

  uw_post_expunge(ctx, data);

  return r;
}

void uw_prune_clients(uw_context ctx) {
  client *c, *next, *prev = NULL;
  time_t cutoff;
  int stop = 0;

  cutoff = time(NULL) - ctx->app->timeout;

  uw_expunger_arrives();
  
  pthread_mutex_lock(&clients_mutex);
  pruning = 1;

  for (c = clients_used; c && !stop; c = next) {
    next = c->next;
    pthread_mutex_lock(&c->lock);
    if (c->last_contact < cutoff && c->refcount == 0) {
      failure_kind fk;
      uw_set_activity(ctx, "expunging client %u", c->id);
      uw_reset(ctx);
      fk = uw_expunge(ctx, c->id, c->data);
      if (fk == SUCCESS) {
        if (uw_has_error(ctx))
          uw_log_error(ctx, "Expunge done, but a commit callback failed: %s\n", uw_error_message(ctx));
        if (prev)
          prev->next = next;
        else
          clients_used = next;
        free_client(c);
      } else {
        // Nothing was written, so the client is as it was: it stays, to be
        // tried again at the next pass unless it is heard from by then.
        uw_log_error(ctx, "Expunge failed, to be tried again at the next pass: %s\n", uw_error_message(ctx));
        prev = c;
        // A database that is busy, or a conflict: the pass ends here, since
        // it keeps every request out while it waits on the database, and
        // the next client's expunge would most likely meet the same.
        if (fk == UNLIMITED_RETRY || fk == BOUNDED_RETRY)
          stop = 1;
      }
    }
    else
      prev = c;
    pthread_mutex_unlock(&c->lock);
  }

  pruning = 0;
  pthread_mutex_unlock(&clients_mutex);

  uw_expunger_departs();
}

// The initializer's transaction begins at its first database operation, as a
// page's does, once the generated code has said whether it may write.
failure_kind uw_initialize(uw_context ctx) {
  int r = setjmp(ctx->jmp_buf);

  if (r == 0) {
    ctx->app->initializer(ctx);

    // uw_commit returns 1 for a serialization conflict alone, after which the
    // database has rolled the transaction back: the initializer runs again,
    // as for a conflict before COMMIT.  Any other failure leaves an error
    // message, and the start-up fails, whether or not the database committed.
    if (uw_commit(ctx))
      uw_error(ctx, UNLIMITED_RETRY, "Commit conflict");
    if (uw_has_error(ctx))
      uw_reraise(ctx, FATAL);
  }

  return r;
}

static int url_bad(uw_Basis_string s) {
  for (; *s; ++s)
    if (!isgraph((unsigned char)*s))
      return 1;

  return 0;
}

uw_Basis_string uw_Basis_bless(uw_context ctx, uw_Basis_string s) {
  if (url_bad(s))
    uw_error(ctx, FATAL, "Invalid URL %s", uw_Basis_htmlifyString(ctx, s));
  if (ctx->app->check_url(s))
    return s;
  else
    uw_error(ctx, FATAL, "Disallowed URL %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_string uw_Basis_checkUrl(uw_context ctx, uw_Basis_string s) {
  if (url_bad(s))
    return NULL;
  if (ctx->app->check_url(s))
    return s;
  else
    return NULL;
}

static int mime_format(const char *s) {
  for (; *s; ++s)
    if (!isalnum((unsigned char)*s) && *s != '/' && *s != '-' && *s != '.' && *s != '+')
      return 0;

  return 1;
}

uw_Basis_string uw_Basis_blessMime(uw_context ctx, uw_Basis_string s) {
  if (!mime_format(s))
    uw_error(ctx, FATAL, "MIME type \"%s\" contains invalid character", uw_Basis_htmlifyString(ctx, s));

  if (ctx->app->check_mime(s))
    return s;
  else
    uw_error(ctx, FATAL, "Disallowed MIME type %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_string uw_Basis_checkMime(uw_context ctx, uw_Basis_string s) {
  if (!mime_format(s))
    return NULL;

  if (ctx->app->check_mime(s))
    return s;
  else
    return NULL;
}

uw_Basis_string uw_Basis_blessRequestHeader(uw_context ctx, uw_Basis_string s) {
  if (!mime_format(s))
    uw_error(ctx, FATAL, "Request header \"%s\" contains invalid character", uw_Basis_htmlifyString(ctx, s));

  if (ctx->app->check_requestHeader(s))
    return s;
  else
    uw_error(ctx, FATAL, "Disallowed request header %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_string uw_Basis_checkRequestHeader(uw_context ctx, uw_Basis_string s) {
  if (!mime_format(s))
    return NULL;

  if (ctx->app->check_requestHeader(s))
    return s;
  else
    return NULL;
}

uw_Basis_string uw_Basis_blessResponseHeader(uw_context ctx, uw_Basis_string s) {
  if (!mime_format(s))
    uw_error(ctx, FATAL, "Response header \"%s\" contains invalid character", uw_Basis_htmlifyString(ctx, s));

  if (ctx->app->check_responseHeader(s))
    return s;
  else
    uw_error(ctx, FATAL, "Disallowed response header %s", uw_Basis_htmlifyString(ctx, s));
}

static int envVar_format(const char *s) {
  for (; *s; ++s)
    if (!isalnum((unsigned char)*s) && *s != '_' && *s != '.')
      return 0;

  return 1;
}

uw_Basis_string uw_Basis_checkResponseHeader(uw_context ctx, uw_Basis_string s) {
  if (!mime_format(s))
    return NULL;

  if (ctx->app->check_responseHeader(s))
    return s;
  else
    return NULL;
}

uw_Basis_string uw_Basis_blessEnvVar(uw_context ctx, uw_Basis_string s) {
  if (!envVar_format(s))
    uw_error(ctx, FATAL, "Environment variable \"%s\" contains invalid character", uw_Basis_htmlifyString(ctx, s));

  if (ctx->app->check_envVar(s))
    return s;
  else
    uw_error(ctx, FATAL, "Disallowed environment variable %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_string uw_Basis_checkEnvVar(uw_context ctx, uw_Basis_string s) {
  if (!envVar_format(s))
    return NULL;

  if (ctx->app->check_envVar(s))
    return s;
  else
    return NULL;
}

static int meta_format(const char *s) {
  for (; *s; ++s)
    if (!isalpha((unsigned char)*s) && *s != '-')
      return 0;

  return 1;
}

uw_Basis_string uw_Basis_blessMeta(uw_context ctx, uw_Basis_string s) {
  if (!meta_format(s))
    uw_error(ctx, FATAL, "Meta name \"%s\" contains invalid character", uw_Basis_htmlifyString(ctx, s));

  if (ctx->app->check_meta(s))
    return s;
  else
    uw_error(ctx, FATAL, "Disallowed meta name %s", uw_Basis_htmlifyString(ctx, s));
}

uw_Basis_string uw_Basis_checkMeta(uw_context ctx, uw_Basis_string s) {
  if (!meta_format(s))
    return NULL;

  if (ctx->app->check_meta(s))
    return s;
  else
    return NULL;
}

uw_Basis_string uw_Basis_getHeader(uw_context ctx, uw_Basis_string name) {
  return uw_Basis_requestHeader(ctx, name);
}

static int mime_value_format(const char *s) {
  for (; *s; ++s)
    if (*s == '\r' || *s == '\n')
      return 0;

  return 1;
}

uw_unit uw_Basis_setHeader(uw_context ctx, uw_Basis_string name, uw_Basis_string value) {
  if (!mime_value_format(value))
    uw_error(ctx, FATAL, "Invalid value for HTTP response header");

  uw_write_header(ctx, name);
  uw_write_header(ctx, ": ");
  uw_write_header(ctx, value);
  uw_write_header(ctx, "\r\n");

  return uw_unit_v;
}

uw_Basis_string uw_Basis_getenv(uw_context ctx, uw_Basis_string name) {
  if (ctx->get_env)
    return ctx->get_env(ctx->get_env_data, name);
  else
    return getenv(name);
}

uw_Basis_string uw_unnull(uw_Basis_string s) {
  return s ? s : "";
}

uw_Basis_string uw_Basis_makeSigString(uw_context ctx, uw_Basis_string sig) {
  uw_Basis_string r = uw_malloc(ctx, 2 * uw_hash_blocksize + 1);
  int i;

  for (i = 0; i < uw_hash_blocksize; ++i)
    sprintf(&r[2*i], "%.02X", ((unsigned char *)sig)[i]);

  return r;
}

/* This bit of crafty code is intended to prevent GCC from performing
 * optimizations that would enable timing attacks.  See:
 *   http://www.impredicative.com/pipermail/ur/2011-July/000659.html
 */
int uw_streq(uw_Basis_string s1, uw_Basis_string s2) {
  int i, x = 0, len1 = strlen(s1);

  if (len1 != strlen(s2)) return 0;

  for (i = 0; i < len1; ++i) {
        __asm__ __volatile__ ("");
        x |= s1[i] ^ s2[i];
  }

  return x == 0;
}

uw_Basis_string uw_Basis_sigString(uw_context ctx, uw_unit u) {
  (void)u;
  ctx->usedSig = 1;
  return ctx->app->cookie_sig(ctx);
}

uw_Basis_string uw_Basis_fileName(uw_context ctx, uw_Basis_file f) {
  (void)ctx;
  return f.name;
}

uw_Basis_string uw_Basis_fileMimeType(uw_context ctx, uw_Basis_file f) {
  (void)ctx;
  return f.type;
}

uw_Basis_int uw_Basis_blobSize(uw_context ctx, uw_Basis_blob b) {
  (void)ctx;
  return b.size;
}

uw_Basis_blob uw_Basis_textBlob(uw_context ctx, uw_Basis_string s) {
  (void)ctx;
  uw_Basis_blob b = {strlen(s), s};

  return b;
}

uw_Basis_string uw_Basis_textOfBlob(uw_context ctx, uw_Basis_blob b) {
  size_t i;
  uw_Basis_string r;

  for (i = 0; i < b.size; ++i)
    if (b.data[i] == 0)
      return NULL;

  r = uw_malloc(ctx, b.size + 1);
  memcpy(r, b.data, b.size);
  r[b.size] = 0;
  return r;
}

uw_Basis_blob uw_Basis_fileData(uw_context ctx, uw_Basis_file f) {
  (void)ctx;
  return f.data;
}

// The application's served file at this URI, or NULL.
static const uw_served_file *served_file(uw_context ctx, const char *uri) {
  const uw_served_file *f;

  for (f = ctx->app->served_files; f && f->uri; ++f)
    if (!strcmp(f->uri, uri))
      return f;

  return NULL;
}

// A served file as a uw_Basis_file: named by the last segment of its URI (no
// name when that is empty), typed as the directive said or "" when nothing
// was known, and its data the bytes in the table, which nothing writes to.
static uw_Basis_file file_of_served(uw_context ctx, const uw_served_file *f) {
  const char *name = strrchr(f->uri, '/');
  uw_Basis_file r;

  name = name ? name + 1 : f->uri;
  r.name = *name ? uw_strdup(ctx, (char *)name) : NULL;
  r.type = f->type ? (char *)f->type : "";
  r.data.size = f->size;
  r.data.data = (char *)f->data;
  return r;
}

uw_Basis_file uw_Basis_blessServedFile(uw_context ctx, uw_Basis_string uri) {
  const uw_served_file *f = served_file(ctx, uri);

  if (!f)
    uw_error(ctx, FATAL, "No file directive serves %s", uw_Basis_htmlifyString(ctx, uri));
  return file_of_served(ctx, f);
}

uw_Basis_file *uw_Basis_checkServedFile(uw_context ctx, uw_Basis_string uri) {
  const uw_served_file *f = served_file(ctx, uri);
  uw_Basis_file *r;

  if (!f)
    return NULL;
  r = uw_malloc(ctx, sizeof *r);
  *r = file_of_served(ctx, f);
  return r;
}

uw_Basis_string uw_Basis_postType(uw_context ctx, uw_Basis_postBody pb) {
  (void)ctx;
  return pb.type;
}

uw_Basis_string uw_Basis_postData(uw_context ctx, uw_Basis_postBody pb) {
  (void)ctx;
  return pb.data;
}

static char *old_headers(uw_context ctx) {
  if (uw_buffer_used(&ctx->outHeaders) == 0)
    return NULL;
  else {
    char *s;
    int is_good;

    if (strncasecmp(ctx->outHeaders.start, "Content-type: ", 14)) {
      s = strchr(ctx->outHeaders.start, '\n');
      is_good = !strncasecmp(s+1, "Content-type: ", 14);
    } else {
      s = ctx->outHeaders.start;
      is_good = 1;
    }

    if (!is_good)
      return NULL;
    else {
      s = strchr(s+15, '\n');
      if (s == NULL)
        return NULL;
      else
        return uw_strdup(ctx, s+1);
    }
  }
}

__attribute__((noreturn)) void uw_return_blob(uw_context ctx, uw_Basis_blob b, uw_Basis_string mimeType) {
  int len;
  char *oldh;

  if (!ctx->allowed_to_return_indirectly)
    uw_error(ctx, FATAL, "Tried to return a blob from an RPC");

  ctx->returning_indirectly = 1;
  oldh = old_headers(ctx);
  uw_buffer_reset(&ctx->outHeaders);
  uw_buffer_reset(&ctx->page);

  uw_write_header(ctx, on_success);
  uw_write_header(ctx, "Content-Type: ");
  uw_write_header(ctx, mimeType);
  uw_write_header(ctx, "\r\nContent-length: ");
  ctx_uw_buffer_check(ctx, "headers", &ctx->outHeaders, INTS_MAX);
  sprintf(ctx->outHeaders.front, "%lu%n", (unsigned long)b.size, &len);
  ctx->outHeaders.front += len;
  uw_write_header(ctx, "\r\n");
  if (oldh) uw_write_header(ctx, oldh);

  ctx_uw_buffer_append(ctx, "page", &ctx->page, b.data, b.size);

  uw_run_cleanups(ctx);

  longjmp(ctx->jmp_buf, RETURN_INDIRECTLY);
}

void uw_replace_page(uw_context ctx, const char *data, size_t size) {
  uw_buffer_reset(&ctx->page);
  ctx_uw_buffer_append(ctx, "page", &ctx->page, data, size);
}

int uw_serve_file(uw_context ctx, const char *request, const char *last_modified) {
  const uw_served_file *f = served_file(ctx, request);
  char header[64];

  if (!f)
    return 0;

  if (f->type) {
    uw_write_header(ctx, "Content-Type: ");
    uw_write_header(ctx, (char *)f->type);
    uw_write_header(ctx, "\r\n");
  }
  uw_write_header(ctx, "Last-Modified: ");
  uw_write_header(ctx, (char *)last_modified);
  uw_write_header(ctx, "\r\n");
  snprintf(header, sizeof header, "Content-Length: %lu\r\n", (unsigned long)f->size);
  uw_write_header(ctx, header);
  uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
  uw_replace_page(ctx, f->data, f->size);
  return 1;
}

__attribute__((noreturn)) void uw_return_blob_from_page(uw_context ctx, uw_Basis_string mimeType) {
  int len;
  char *oldh;

  if (!ctx->allowed_to_return_indirectly)
    uw_error(ctx, FATAL, "Tried to return a blob from an RPC");

  ctx->returning_indirectly = 1;
  oldh = old_headers(ctx);
  uw_buffer_reset(&ctx->outHeaders);

  uw_write_header(ctx, on_success);
  uw_write_header(ctx, "Content-Type: ");
  uw_write_header(ctx, mimeType);
  uw_write_header(ctx, "\r\nContent-length: ");
  ctx_uw_buffer_check(ctx, "headers", &ctx->outHeaders, INTS_MAX);
  sprintf(ctx->outHeaders.front, "%lu%n", (unsigned long)uw_buffer_used(&ctx->page), &len);
  ctx->outHeaders.front += len;
  uw_write_header(ctx, "\r\n");
  if (oldh) uw_write_header(ctx, oldh);

  uw_run_cleanups(ctx);

  longjmp(ctx->jmp_buf, RETURN_INDIRECTLY);
}

__attribute__((noreturn)) void uw_redirect(uw_context ctx, uw_Basis_string url) {
  char *s;
  char *oldh;

  if (!ctx->allowed_to_return_indirectly)
    uw_error(ctx, FATAL, "Tried to redirect from an RPC");

  ctx->returning_indirectly = 1;
  oldh = old_headers(ctx);
  uw_buffer_reset(&ctx->page);
  ctx_uw_buffer_check(ctx, "page", &ctx->page, uw_buffer_used(&ctx->outHeaders)+1);
  memcpy(ctx->page.start, ctx->outHeaders.start, uw_buffer_used(&ctx->outHeaders));
  ctx->page.start[uw_buffer_used(&ctx->outHeaders)] = 0;
  uw_buffer_reset(&ctx->outHeaders);

  if (on_success[0])
    uw_write_header(ctx, on_redirect);
  else
    uw_write_header(ctx, "Status: 303 See Other\r\n");
  // The headers set so far are the redirect's, too.  oldh has all of them,
  // cookies included; failing that, the cookies are picked out of the copy
  // made above.
  s = on_success[0] ? strchr(ctx->page.start, '\n') : ctx->page.start;
  if (s && !oldh) {
    char *s2;
    if (s[0] == '\n') ++s;
    for (; (s2 = strchr(s, '\n')); s = s2+1) {
      *s2 = 0;
      if (!strncmp(s, "Set-Cookie: ", 12)) {
        uw_write_header(ctx, s);
        uw_write_header(ctx, "\n");
      }
    }
  }

  // No blank line here: whoever sends the response ends the headers, after
  // adding its own.
  uw_write_header(ctx, "Location: ");
  uw_write_header(ctx, url);
  uw_write_header(ctx, "\r\n");
  if (oldh) uw_write_header(ctx, oldh);

  uw_run_cleanups(ctx);

  longjmp(ctx->jmp_buf, RETURN_INDIRECTLY);
}

uw_Basis_string uw_Basis_unAs(uw_context ctx, uw_Basis_string s) {
  uw_Basis_string ret = uw_malloc(ctx, strlen(s) + 1), r = ret;

  for (; *s; ++s) {
    if (s[0] == '\'') {
      *r++ = '\'';
      for (++s; *s; ++s) {
        if (s[0] == '\'') {
          *r++ = '\'';
          break;
        } else if (s[0] == '\\') {
          *r++ = '\\';
          *r++ = s[1];
          ++s;
        } else
          *r++ = s[0];
      }
      if (*s == 0) break;
    } else if (s[0] == 'T' && s[1] == '_' && s[2] == 'T' && s[3] == '.')
      s += 3;
    else
      *r++ = s[0];
  }

  *r = 0;
  return ret;
}

uw_Basis_string uw_Basis_mstrcat(uw_context ctx, ...) {
  va_list ap;
  size_t len = 1;
  char *s, *r, *s2;

  va_start(ap, ctx);
  for (s = va_arg(ap, char*); s; s = va_arg(ap, char*))
    len += strlen(s);
  va_end(ap);

  r = uw_malloc(ctx, len);
  va_start(ap, ctx);
  for (s2 = r, s = va_arg(ap, char*); s; s = va_arg(ap, char*))
    while (*s)
      *s2++ = *s++;
  va_end(ap);
  *s2 = 0;

  return r;
}

const uw_Basis_time uw_Basis_minTime = {};

uw_Basis_time uw_Basis_now(uw_context ctx) {
  (void)ctx;
  int64_t pinned;
  uw_Basis_time r = { uw_reproducible_epoch(&pinned) ? pinned : time(NULL) };
  return r;
}

uw_Basis_time uw_Basis_addSeconds(uw_context ctx, uw_Basis_time tm, uw_Basis_int n) {
  (void)ctx;
  tm.seconds += n;
  return tm;
}

uw_Basis_int uw_Basis_diffInSeconds(uw_context ctx, uw_Basis_time tm1, uw_Basis_time tm2) {
  (void)ctx;
  return difftime(tm2.seconds, tm1.seconds);
}

uw_Basis_int uw_Basis_toMilliseconds(uw_context ctx, uw_Basis_time tm) {
  (void)ctx;
  return tm.seconds * 1000 + tm.microseconds / 1000;
}

uw_Basis_time uw_Basis_fromMilliseconds(uw_context ctx, uw_Basis_int n) {
  (void)ctx;
  return time_of_count(n, 1000);
}

uw_Basis_int uw_Basis_diffInMilliseconds(uw_context ctx, uw_Basis_time tm1, uw_Basis_time tm2) {
  return uw_Basis_toMilliseconds(ctx, tm2) - uw_Basis_toMilliseconds(ctx, tm1);
}

uw_Basis_int uw_Basis_toSeconds(uw_context ctx, uw_Basis_time tm) {
  (void)ctx;
  return tm.seconds;
}

uw_Basis_time uw_Basis_fromDatetime(uw_context ctx, uw_Basis_int year, uw_Basis_int month, uw_Basis_int day, uw_Basis_int hour, uw_Basis_int minute, uw_Basis_int second) {
  (void)ctx;
  struct tm tm = { .tm_year = year - 1900, .tm_mon = month, .tm_mday = day,
                   .tm_hour = hour, .tm_min = minute, .tm_sec = second,
                   .tm_isdst = -1 };
  uw_Basis_time r = { mktime(&tm) };
  return r;
}

uw_Basis_int uw_Basis_datetimeYear(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_year + 1900;
}

uw_Basis_int uw_Basis_datetimeMonth(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_mon;
}

uw_Basis_int uw_Basis_datetimeDay(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_mday;
}

uw_Basis_int uw_Basis_datetimeHour(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_hour;
}

uw_Basis_int uw_Basis_datetimeMinute(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_min;
}

uw_Basis_int uw_Basis_datetimeSecond(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_sec;
}

uw_Basis_int uw_Basis_datetimeDayOfWeek(uw_context ctx, uw_Basis_time time) {
  (void)ctx;
  struct tm tm;
  localtime_r(&time.seconds, &tm);
  return tm.tm_wday;
}


void *uw_get_global(uw_context ctx, char *name) {
  int i;

  for (i = 0; i < ctx->n_globals; ++i)
    if (!strcmp(name, ctx->globals[i].name))
      return ctx->globals[i].data;

  return NULL;
}

size_t uw_globals_max = SIZE_MAX;

void uw_set_global(uw_context ctx, char *name, void *data, void (*free)(void*)) {
  int i;

  for (i = 0; i < ctx->n_globals; ++i)
    if (!strcmp(name, ctx->globals[i].name)) {
      if (ctx->globals[i].free)
        ctx->globals[i].free(ctx->globals[i].data);
      ctx->globals[i].data = data;
      ctx->globals[i].free = free;
      return;
    }

  if (ctx->n_globals+1 > uw_globals_max)
    uw_error(ctx, FATAL, "Exceeded limit on number of globals");

  ++ctx->n_globals;
  ctx->globals = realloc(ctx->globals, ctx->n_globals * sizeof(global));
  ctx->globals[ctx->n_globals-1].name = name;
  ctx->globals[ctx->n_globals-1].data = data;
  ctx->globals[ctx->n_globals-1].free = free;
}

uw_Basis_bool uw_Basis_isalnum(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_POSIX_ALNUM);
}

uw_Basis_bool uw_Basis_isalpha(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_ALPHABETIC);
}

uw_Basis_bool uw_Basis_isblank(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_POSIX_BLANK);
}

uw_Basis_bool uw_Basis_iscntrl(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!(u_charType(c)==U_CONTROL_CHAR);
}

uw_Basis_bool uw_Basis_isdigit(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_isdigit(c);
}

uw_Basis_bool uw_Basis_isgraph(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_POSIX_GRAPH);
}

uw_Basis_bool uw_Basis_islower(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_LOWERCASE);
}

uw_Basis_bool uw_Basis_isprint(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_POSIX_PRINT);
}

uw_Basis_bool uw_Basis_ispunct(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_ispunct(c);
}

uw_Basis_bool uw_Basis_isspace(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_WHITE_SPACE);
}

uw_Basis_bool uw_Basis_isupper(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!u_hasBinaryProperty(c, UCHAR_UPPERCASE);
}

uw_Basis_bool uw_Basis_isxdigit(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!(c <= 0x7f && u_isxdigit(c));
}

uw_Basis_char uw_Basis_tolower(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return u_tolower(c);
}

uw_Basis_char uw_Basis_toupper(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return u_toupper(c);
}

uw_Basis_int uw_Basis_ord(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return (uw_Basis_int)c;
}

uw_Basis_bool uw_Basis_iscodepoint(uw_context ctx, uw_Basis_int n) {
  (void)ctx;
  return !!(0 <= n && n <= 0x10FFFF);
}

uw_Basis_bool uw_Basis_issingle(uw_context ctx, uw_Basis_char c) {
  (void)ctx;
  return !!(c < 128);
}

uw_Basis_char uw_Basis_chr(uw_context ctx, uw_Basis_int n) {
  if (!uw_Basis_iscodepoint(ctx, n)) {
    uw_error(ctx, FATAL, "The integer %lld is not a valid char codepoint", n);
  }
  return (uw_Basis_char)n;
}

uw_Basis_string uw_Basis_currentUrl(uw_context ctx) {
  return ctx->current_url;
}

uw_Basis_string uw_Basis_anchorUrl(uw_context ctx, uw_Basis_string s) {
  return uw_Basis_strcat(ctx, uw_Basis_strcat(ctx, ctx->current_url, "#"), s);
}

void uw_set_currentUrl(uw_context ctx, char *s) {
  ctx->current_url = s;
}

void uw_set_deadline(uw_context ctx, int n) {
  ctx->deadline = n;
}

void uw_check_deadline(uw_context ctx) {
  if (uw_time > ctx->deadline)
    uw_error(ctx, FATAL, "Maximum running time exceeded");
}

size_t uw_database_max = SIZE_MAX;

uw_Basis_int uw_Basis_naughtyDebug(uw_context ctx, uw_Basis_string s) {
  if (ctx->loggers->log_debug)
    ctx->loggers->log_debug(ctx->loggers->logger_data, "%s\n", s);
  else
    fprintf(stderr, "%s\n", s);
  return 0;
}

uw_Basis_unit uw_Basis_debug(uw_context ctx, uw_Basis_string s) {
  if (ctx->loggers->log_debug)
    ctx->loggers->log_debug(ctx->loggers->logger_data, "%s\n", s);
  else
    fprintf(stderr, "%s\n", s);
  return uw_unit_v;
}

uw_Basis_int uw_Basis_rand(uw_context ctx) {
  return my_rand(ctx);
}

// The start-up check of the database, run in the caller's transaction.  What
// is wrong comes as the context's error message, which must not stay set: the
// request would be taken for failed.  A database found busy runs the request
// again, as a busy statement does.
uw_Basis_string uw_Basis_validateDb(uw_context ctx) {
  uw_ensure_transaction(ctx);
  if (ctx->app->db_check(ctx))
    return uw_dup_and_clear_error_message(ctx);
  else
    return NULL;
}

void uw_noPostBody(uw_context ctx) {
  ctx->hasPostBody = 0;
}

void uw_postBody(uw_context ctx, uw_Basis_postBody pb) {
  ctx->hasPostBody = 1;
  ctx->postBody = pb;
}

int uw_hasPostBody(uw_context ctx) {
  return ctx->hasPostBody;
}

void uw_isPost(uw_context ctx) {
  ctx->isPost = 1;
}

uw_Basis_bool uw_Basis_currentUrlHasPost(uw_context ctx) {
  return ctx->isPost;
}

uw_Basis_bool uw_Basis_currentUrlHasQueryString(uw_context ctx) {
  return ctx->queryString != NULL && ctx->queryString[0] != 0;
}

void uw_setQueryString(uw_context ctx, uw_Basis_string s) {
  ctx->queryString = s;
}

uw_Basis_string uw_queryString(uw_context ctx) {
  return ctx->queryString;
}

uw_Basis_postBody uw_getPostBody(uw_context ctx) {
  if (ctx->hasPostBody)
    return ctx->postBody;
  else
    uw_error(ctx, FATAL, "Asked for POST body when none exists");
}

int uw_retry_again(failure_kind fk, int *attempts_left) {
  switch (fk) {
  case UNLIMITED_RETRY:
    return 1;
  case BOUNDED_RETRY:
    return --*attempts_left > 0;
  default:
    return 0;
  }
}

// A periodic task's transaction begins at its first database operation, as a
// page's does, once the task's code has said whether it may write.
failure_kind uw_runCallback(uw_context ctx, void (*callback)(uw_context)) {
  int r = setjmp(ctx->jmp_buf);

  if (r == 0)
    callback(ctx);

  return r;
}

// io computations.  The body runs with no transaction open and is never run
// again after a failure; it allocates like any other code, since a heap
// never moves (see "The heap").  uw_io_prepare marks the context as one
// that runs them, which uw_runIo and uw_io_request_begin require.
void uw_io_prepare(uw_context ctx) {
  ctx->io = 1;
}

// A failure that reaches an io computation itself, outside any transaction.
// FATAL stays what it is.  A retry asks for the code to be run again, which
// nothing can do for io code, so it is fatal too, and the message says why.
// A redirect or a returned file has no request to go to here.
static failure_kind uw_io_failure(uw_context ctx, int fk) {
  char msg[ERROR_BUF_LEN];

  switch (fk) {
  case BOUNDED_RETRY:
  case UNLIMITED_RETRY:
    strcpy(msg, ctx->error_message);
    uw_set_error_message(ctx, "%s (retry requested outside any transaction in io code)", msg);
    return FATAL;
  case RETURN_INDIRECTLY:
    uw_set_error_message(ctx, "Tried to redirect or to return a file from io code");
    return FATAL;
  default:
    return fk;
  }
}

failure_kind uw_runIo(uw_context ctx, void (*callback)(uw_context)) {
  int r;

  if (!ctx->io) {
    uw_set_error_message(ctx, "uw_runIo on a context not prepared for io (uw_io_prepare)");
    return FATAL;
  }

  r = setjmp(ctx->jmp_buf);

  if (r == 0) {
    callback(ctx);
    return SUCCESS;
  }

  return uw_io_failure(ctx, r);
}

jmp_buf *uw_jmp_buf(uw_context ctx) {
  return &ctx->jmp_buf;
}

// A transaction inline in io code.  The generated code is
//
//   uw_io_transaction t;  T r;
//   uw_io_transaction_begin(ctx, &t);
//   while (1) {
//     int fk = setjmp(*uw_jmp_buf(ctx));
//     if (fk == 0) { r = BODY; uw_io_transaction_commit(ctx, &t); break; }
//     else if (!uw_io_transaction_retry(ctx, &t, fk)) { [r = NULL;] break; }
//   }
//   uw_io_transaction_end(ctx, &t, reraise);
//
// which is uw_request's protocol for a handler, with the body inline.
//
// The context is the computation's, and the transaction has no uw_reset of
// its own, so what belongs to the transaction is marked off at its start and
// cut back when it is over.  Its transactionals and cleanups are the ones
// above the marks; uw_commit, uw_rollback and uw_error stop there, and what
// the computation registered outside any transaction (an FFI transactional,
// say) is left for the end of the computation.  An attempt that does not
// commit also takes back the response headers it set, the script it wrote
// and what it allocated: the heap front and the region stack go back to where
// they were, since nothing of a rolled-back attempt is used afterwards.

// The client of an io function run by a request is the request's, not the
// transaction's, and stays named for the next transaction (see
// release_client_of).
static void uw_io_transaction_cut(uw_context ctx, uw_io_transaction *t, int committed) {
  uw_drop_transaction_state(ctx);

  if (committed)
    ctx->io_error_message[0] = 0;
  else {
    ctx->outHeaders.front = ctx->outHeaders.start + t->headers_used;
    *ctx->outHeaders.front = 0;
    ctx->script.front = ctx->script.start + t->script_used;
    *ctx->script.front = 0;
    uw_heap_moving_back(ctx, t->heap_front);
    ctx->heap.front = t->heap_front;
    ctx->regions = t->regions;
    // The page recordings the SQL cache opened for the attempt's misses are
    // still open; its stores went with its rollback, since the cache's
    // transactional is the attempt's own (the lock calls are the first
    // statements of the body).
    ctx->numRecording = t->num_recording;
  }
}

void uw_io_transaction_begin(uw_context ctx, uw_io_transaction *t) {
  memcpy(&t->outer, &ctx->jmp_buf, sizeof(jmp_buf));
  t->attempts_left = UW_RETRY_ATTEMPTS;
  t->failed = 0;
  t->committing = 0;
  t->headers_used = uw_buffer_used(&ctx->outHeaders);
  t->script_used = uw_buffer_used(&ctx->script);
  t->heap_front = ctx->heap.front;
  t->regions = ctx->regions;
  t->transactionals_base = ctx->transactionals_base;
  t->cleanup_base = ctx->cleanup_base;
  t->num_recording = ctx->numRecording;
  ctx->transactionals_base = ctx->used_transactionals;
  ctx->cleanup_base = ctx->cleanup_front - ctx->cleanup;
  ctx->used_deltas = 0;
  ctx->error_message[0] = 0;
  if (!ctx->io_request)
    uw_transaction_arrives();
}

void uw_io_transaction_commit(uw_context ctx, uw_io_transaction *t) {
  int conflict;

  t->committing = 1;
  conflict = uw_commit(ctx);
  t->committing = 0;

  // On every failure uw_commit has already undone and freed the transaction's
  // transactionals, so they are taken off the stack before anything else
  // could undo or free them again.
  if (conflict) {
    // A serialization failure at COMMIT: the database rolled the transaction
    // back.  Run the body again.
    ctx->used_transactionals = ctx->transactionals_base;
    uw_error(ctx, UNLIMITED_RETRY, "Commit conflict; retrying");
  }
  if (uw_has_error(ctx)) {
    // COMMIT itself failed, perhaps with the transaction still open, or a
    // transactional's commit callback failed.  Not run again: a callback that
    // runs after COMMIT (one without a rollback callback) fails with the
    // database already committed.
    ctx->used_transactionals = ctx->transactionals_base;
    uw_reraise(ctx, FATAL);
  }

  uw_io_transaction_cut(ctx, t, 1);
}

int uw_io_transaction_retry(uw_context ctx, uw_io_transaction *t, int fk) {
  int again = 0;
  void *ld = ctx->loggers->logger_data;

  if (t->committing) {
    // The longjmp came out of uw_commit: a transactional's commit callback
    // called uw_error, which it must not (it is to set the message and
    // return).  The database may have committed by then, so whatever the
    // kind, the body is not run again.
    t->committing = 0;
    ctx->loggers->log_error(ld, "Error raised by a commit callback in io transaction: %s\n", ctx->error_message);
  } else
    switch (fk) {
    case UNLIMITED_RETRY:
      ctx->loggers->log_debug(ld, "Error triggers unlimited retry in io transaction: %s\n", ctx->error_message);
      again = uw_retry_again(fk, &t->attempts_left);
      break;
    case BOUNDED_RETRY:
      ctx->loggers->log_debug(ld, "Error triggers bounded retry in io transaction: %s\n", ctx->error_message);
      again = uw_retry_again(fk, &t->attempts_left);
      if (!again)
        ctx->loggers->log_error(ld, "Fatal error in io transaction (out of retries): %s\n", ctx->error_message);
      break;
    case RETURN_INDIRECTLY:
      uw_set_error_message(ctx, "Tried to redirect or to return a file from a transaction in io code");
      break;
    default:
      break;
    }

  if (uw_rollback(ctx, again))
    ctx->loggers->log_error(ld, "Error running SQL ROLLBACK in io transaction\n");
  uw_io_transaction_cut(ctx, t, 0);

  if (again)
    ctx->error_message[0] = 0;
  else {
    t->failed = 1;
    strcpy(ctx->io_error_message, ctx->error_message);
  }
  return again;
}

void uw_io_transaction_end(uw_context ctx, uw_io_transaction *t, int reraise) {
  memcpy(&ctx->jmp_buf, &t->outer, sizeof(jmp_buf));
  ctx->transactionals_base = t->transactionals_base;
  ctx->cleanup_base = t->cleanup_base;
  if (!ctx->io_request)
    uw_transaction_departs();
  if (t->failed) {
    if (reraise)
      uw_error(ctx, FATAL, "%s", ctx->io_error_message);
    else
      // Handled by the program: the computation goes on, with the message
      // kept for io_errorMessage alone.
      ctx->error_message[0] = 0;
  }
}

uw_Basis_string uw_Basis_io_errorMessage(uw_context ctx) {
  return uw_strdup(ctx, ctx->io_error_message);
}

// The io function of an RPC, inline in the handler the compiler generates:
//
//   uw_io_request r;
//   uw_io_request_begin(ctx, &r);
//   { int fk = setjmp(*uw_jmp_buf(ctx));
//     if (fk == 0) { T v = BODY; uw_io_request_end(ctx, &r, 0); ANSWER v; }
//     else uw_io_request_end(ctx, &r, fk); }
//
// The request came in under uw_request's protocol for a handler: the
// expunger's lock taken, uw_login done, no transaction begun yet (that
// happens at the first query).  The body is an io computation: it opens no
// transaction of the request's, runs its own (uw_io_transaction_*), and is
// never run again, so a failure that reaches this level is re-raised to
// uw_request as FATAL whatever its kind -- a retry from inside a transaction
// never gets here, and one from outside cannot be honoured.  The context must
// be prepared for io (uw_io_prepare), which the runtime does for every
// request context of an application with io handlers.
//
// The client uw_login found or made stays held and named across the inner
// transactions (release_client_of), for the request's own commit or rollback
// to release, so that self and the channels work in every one of them.  The
// expunger's lock, which uw_request took, stays held across the body as well,
// and the inner transactions do not take it again.  So the expunger does not
// run between them, and a channel the body read in one still has its client
// in the next, which an io task, taking the lock for each transaction alone,
// cannot count on.  The price is an expunger kept waiting while an io RPC
// waits on the network, as it is kept waiting by any request.  Requests are
// not held up by a waiting expunger (see uw_transaction_arrives).
void uw_io_request_begin(uw_context ctx, uw_io_request *r) {
  if (!ctx->io)
    uw_error(ctx, FATAL, "io code in a request on a context not prepared for io (uw_io_prepare)");
  memcpy(&r->outer, &ctx->jmp_buf, sizeof(jmp_buf));
  ctx->io_request = 1;
}

void uw_io_request_end(uw_context ctx, uw_io_request *r, int fk) {
  memcpy(&ctx->jmp_buf, &r->outer, sizeof(jmp_buf));
  ctx->io_request = 0;
  if (fk)
    longjmp(ctx->jmp_buf, uw_io_failure(ctx, fk));
}

uw_Basis_bool uw_Basis_eq_time(uw_context ctx, uw_Basis_time t1, uw_Basis_time t2) {
  (void)ctx;
  return !!(t1.seconds == t2.seconds && t1.microseconds == t2.microseconds);
}

uw_Basis_bool uw_Basis_lt_time(uw_context ctx, uw_Basis_time t1, uw_Basis_time t2) {
  (void)ctx;
  return !!(t1.seconds < t2.seconds || (t1.seconds == t2.seconds && t1.microseconds < t2.microseconds));
}

uw_Basis_bool uw_Basis_le_time(uw_context ctx, uw_Basis_time t1, uw_Basis_time t2) {
  return !!(uw_Basis_eq_time(ctx, t1, t2) || uw_Basis_lt_time(ctx, t1, t2));
}

uw_Basis_time *uw_Basis_readUtc(uw_context ctx, uw_Basis_string s) {
  struct tm stm = {};
  char *end = strchr(s, 0);
  stm.tm_isdst = -1;

  if (strptime(s, TIME_FMT_PG, &stm) == end || strptime(s, TIME_FMT, &stm) == end || strptime(s, TIME_FMT_JS, &stm) == end) {
    uw_Basis_time *r = uw_malloc(ctx, sizeof(uw_Basis_time));

    r->seconds = timegm(&stm);
    r->microseconds = 0;

    return r;
  }
  else
    return NULL;
}

// The handler's transaction begins at its first database operation, as a
// page's does, once the generated uw_onError has said whether it may write.
// Beginning it here would take the value of the handler that failed.
failure_kind uw_begin_onError(uw_context ctx, char *msg) {
  int r = setjmp(ctx->jmp_buf);

  if (ctx->app->on_error) {
    if (r == 0) {
      uw_buffer_reset(&ctx->outHeaders);
      if (on_success[0])
        uw_write_header(ctx, "HTTP/1.1 ");
      else
        uw_write_header(ctx, "Status: ");
      uw_write_header(ctx, "500 Internal Server Error\r\n");
      uw_write_header(ctx, "Content-type: text/html\r\n");
      uw_write(ctx, ctx->app->is_html5 ? uw_begin_html5 : uw_begin_xhtml);
      ctx->app->on_error(ctx, msg);
      uw_write(ctx, "</html>");
    }

    return r;
  } else
    uw_error(ctx, FATAL, "Tried to run nonexistent onError handler");
}

void uw_mayReturnIndirectly(uw_context ctx) {
  ctx->allowed_to_return_indirectly = 1;
}

uw_Basis_string uw_Basis_fresh(uw_context ctx) {
  int len;
  char *r;

  uw_check_heap(ctx, 2+INTS_MAX);
  r = ctx->heap.front;
  sprintf(r, "uw%u%n", ctx->nextId++, &len);
  ctx->heap.front += len+1;
  return r;
}

uw_Basis_float uw_Basis_floatFromInt(uw_context ctx, uw_Basis_int n) {
  (void)ctx;
  return n;
}

uw_Basis_int uw_Basis_ceil(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return ceil(n);
}

uw_Basis_int uw_Basis_trunc(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return trunc(n);
}

uw_Basis_int uw_Basis_round(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return round(n);
}

uw_Basis_int uw_Basis_floor(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return floor(n);
}

uw_Basis_float uw_Basis_pow(uw_context ctx, uw_Basis_float n, uw_Basis_float m) {
  (void)ctx;
  return pow(n,m);
}

uw_Basis_float uw_Basis_sqrt(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return sqrt(n);
}

uw_Basis_float uw_Basis_sin(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return sin(n);
}

uw_Basis_float uw_Basis_cos(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return cos(n);
}

uw_Basis_float uw_Basis_log(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return log(n);
}

uw_Basis_float uw_Basis_exp(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return exp(n);
}

uw_Basis_float uw_Basis_asin(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return asin(n);
}

uw_Basis_float uw_Basis_acos(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return acos(n);
}

uw_Basis_float uw_Basis_atan(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return atan(n);
}

uw_Basis_float uw_Basis_atan2(uw_context ctx, uw_Basis_float n, uw_Basis_float m) {
  (void)ctx;
  return atan2(n, m);
}

uw_Basis_float uw_Basis_abs(uw_context ctx, uw_Basis_float n) {
  (void)ctx;
  return fabs(n);
}

uw_Basis_string uw_Basis_atom(uw_context ctx, uw_Basis_string s) {
  char *p;

  for (p = s; *p; ++p) {
    char c = *p;
    if (!U8_IS_SINGLE(c) || (!isalnum((int)c) && c != '+' && c != '-' && c != '.' && c != '%' && c != '#'))
      uw_error(ctx, FATAL, "Disallowed character in CSS atom");
  }

  return s;
}

uw_Basis_string uw_Basis_css_url(uw_context ctx, uw_Basis_string s) {
  char *p;

  for (p = s; *p; ++p) {
    char c = *p;
    if (!U8_IS_SINGLE(c) || (!isalnum((int)c) && c != ':' && c != '/' && c != '.' && c != '_' && c != '+'
			     && c != '-' && c != '%' && c != '?' && c != '&' && c != '=' && c != '#'))
      uw_error(ctx, FATAL, "Disallowed character in CSS URL");
  }

  return s;
}

// CSS identifiers, per the <ident-token> production of CSS Syntax Level 3
// (minus backslash escapes; see checkProperty in mono_opt.sml for the full
// rationale).  A non-ASCII code point is a run of UTF-8 bytes >= 0x80, all of
// which count as identifier characters.
static int css_ident_start(unsigned char c) {
  return isalpha(c) || c == '_' || c >= 0x80;
}

static int css_ident_char(unsigned char c) {
  return css_ident_start(c) || isdigit(c) || c == '-';
}

uw_Basis_string uw_Basis_property(uw_context ctx, uw_Basis_string s) {
  unsigned char *p = (unsigned char *)s;

  if (!*p)
    uw_error(ctx, FATAL, "Empty CSS property");

  if (*p == '-') {
    ++p;
    if (*p == '-') {
      // '--' <ident>*, e.g. a custom property; the bare name "--" is reserved.
      ++p;
      if (!*p)
        uw_error(ctx, FATAL, "Reserved CSS property name '--'");
    } else if (!css_ident_start(*p))
      uw_error(ctx, FATAL, "Bad initial character in CSS property");
  } else if (!css_ident_start(*p))
    uw_error(ctx, FATAL, "Bad initial character in CSS property");

  for (; *p; ++p)
    if (!css_ident_char(*p))
      uw_error(ctx, FATAL, "Disallowed character in CSS property");

  return s;
}

uw_Basis_string uw_Basis_fieldName(uw_context ctx, uw_Basis_postField f) {
  (void)ctx;
  return f.name;
}

uw_Basis_string uw_Basis_fieldValue(uw_context ctx, uw_Basis_postField f) {
  (void)ctx;
  return f.value;
}

uw_Basis_string uw_Basis_remainingFields(uw_context ctx, uw_Basis_postField f) {
  (void)ctx;
  return f.remaining;
}

uw_Basis_postField *uw_Basis_firstFormField(uw_context ctx, uw_Basis_string s) {
  char *unurl;
  uw_Basis_postField *f;

  if (!ctx->hasPostBody)
    uw_error(ctx, FATAL, "firstFormField called when there is no POST body");

  if (s < ctx->postBody.data || s >= ctx->postBody.data + ctx->postBody.len)
    return NULL;

  f = uw_malloc(ctx, sizeof(uw_Basis_postField));
  unurl = s;
  f->name = uw_Basis_unurlifyString_fromClient(ctx, &unurl);
  s = strchr(s, 0);
  if (!s)
    uw_error(ctx, FATAL, "firstFormField: Missing null terminator");
  ++s;
  unurl = s;
  f->value = uw_Basis_unurlifyString_fromClient(ctx, &unurl);
  s = strchr(s, 0);
  if (!s)
    uw_error(ctx, FATAL, "firstFormField: Missing null terminator");
  f->remaining = s+1;

  return f;
}

uw_Basis_string uw_Basis_blessData(uw_context ctx, uw_Basis_string s) {
  char *p = s;

  for (; *p; ++p)
    if (!U8_IS_SINGLE(*p) || (!isalnum(*p) && *p != '-' && *p != '_'))
      uw_error(ctx, FATAL, "Illegal HTML5 data-* attribute: %s", s);

  return s;
}

int uw_remoteSock(uw_context ctx) {
  return ctx->remoteSock;
}

void uw_set_remoteSock(uw_context ctx, int sock) {
  ctx->remoteSock = sock;
}


// Sqlcache

static void uw_Sqlcache_freeValue(uw_Sqlcache_Value *value) {
  if (value) {
    free(value->result);
    free(value->output);
    free(value->scriptOutput);
    free(value);
  }
}

static void uw_Sqlcache_freeEntry(uw_Sqlcache_Entry* entry) {
  if (entry) {
    free(entry->key);
    uw_Sqlcache_freeValue(entry->value);
    free(entry);
  }
}

// TODO: pick a number.
static unsigned int uw_Sqlcache_maxSize = 1234567890;

static void uw_Sqlcache_delete(uw_Sqlcache_Cache *cache, uw_Sqlcache_Entry *entry) {
  if (entry) {
    HASH_DEL(cache->table, entry);
    uw_Sqlcache_freeEntry(entry);
  }
}

static uw_Sqlcache_Entry *uw_Sqlcache_find(uw_Sqlcache_Cache *cache, char *key, size_t len, int bump) {
  uw_Sqlcache_Entry *entry = NULL;
  HASH_FIND(hh, cache->table, key, len, entry);
  if (entry && bump) {
    // Bump for LRU purposes.
    HASH_DEL(cache->table, entry);
    // Important that we use [entry->key], because [key] might be ephemeral.
    HASH_ADD_KEYPTR(hh, cache->table, entry->key, len, entry);
  }
  return entry;
}

static void uw_Sqlcache_add(uw_Sqlcache_Cache *cache, uw_Sqlcache_Entry *entry, size_t len) {
  HASH_ADD_KEYPTR(hh, cache->table, entry->key, len, entry);
  if (HASH_COUNT(cache->table) > uw_Sqlcache_maxSize) {
    // Deletes the first element of the cache.
    uw_Sqlcache_delete(cache, cache->table);
  }
}

static unsigned long uw_Sqlcache_getTimeNow(uw_Sqlcache_Cache *cache) {
  // TODO: verify that this makes time comparisons do the Right Thing.
  return cache->timeNow++;
}

static unsigned long uw_Sqlcache_timeMax(unsigned long x, unsigned long y) {
  return x > y ? x : y;
}

static char uw_Sqlcache_keySep = '_';

static char *uw_Sqlcache_allocKeyBuffer(char **keys, size_t numKeys) {
  size_t len = 0;
  while (numKeys-- > 0) {
    char* k = keys[numKeys];
    if (!k) {
      // Can only happen when flushing, in which case we don't need anything past the null key.
      break;
    }
    // Leave room for separator.
    len += 1 + strlen(k);
  }
  char *buf = malloc(len+1);
  // If nothing is copied into the buffer, it should look like it has length 0.
  buf[0] = 0;
  return buf;
}

static char *uw_Sqlcache_keyCopy(char *buf, char *key) {
  *buf++ = uw_Sqlcache_keySep;
  return stpcpy(buf, key);
}

// The NUL-terminated prefix of [key] below always looks something like "_k1_k2_k3..._kn".

uw_Sqlcache_Value *uw_Sqlcache_check(uw_context ctx, uw_Sqlcache_Cache *cache, char **keys) {
  (void)ctx;
  int doBump = random() % 1024 == 0;
  if (doBump) {
    pthread_rwlock_wrlock(&cache->lockIn);
  } else {
    pthread_rwlock_rdlock(&cache->lockIn);
  }
  size_t numKeys = cache->numKeys;
  char *key = uw_Sqlcache_allocKeyBuffer(keys, numKeys);
  char *buf = key;
  time_t timeInvalid = cache->timeInvalid;
  uw_Sqlcache_Entry *entry = NULL;
  if (numKeys == 0) {
    entry = cache->table;
    if (!entry) {
      free(key);
      pthread_rwlock_unlock(&cache->lockIn);
      return NULL;
    }
  } else {
    while (numKeys-- > 0) {
      buf = uw_Sqlcache_keyCopy(buf, keys[numKeys]);
      size_t len = buf - key;
      entry = uw_Sqlcache_find(cache, key, len, doBump);
      if (!entry) {
        free(key);
        pthread_rwlock_unlock(&cache->lockIn);
        return NULL;
      }
      timeInvalid = uw_Sqlcache_timeMax(timeInvalid, entry->timeInvalid);
    }
    free(key);
  }
  uw_Sqlcache_Value *value = entry->value;
  pthread_rwlock_unlock(&cache->lockIn);
  // ASK: though the argument isn't trivial, this is safe, right?
  // Returning outside the lock is safe because updates happen at commit time.
  // Those are the only times the returned value or its strings can get freed.
  // Handler output is a new string, so it's safe to free this at commit time.
  return value && timeInvalid < value->timeValid ? value : NULL;
}

static void uw_Sqlcache_storeCommitOne(uw_Sqlcache_Cache *cache, char **keys, uw_Sqlcache_Value *value) {
  pthread_rwlock_wrlock(&cache->lockIn);
  size_t numKeys = cache->numKeys;
  time_t timeNow = uw_Sqlcache_getTimeNow(cache);
  uw_Sqlcache_Entry *entry = NULL;
  if (numKeys == 0) {
    entry = cache->table;
    if (!entry) {
      entry = calloc(1, sizeof(uw_Sqlcache_Entry));
      entry->key = NULL;
      entry->value = NULL;
      entry->timeInvalid = 0;
      cache->table = entry;
    }
  } else {
    char *key = uw_Sqlcache_allocKeyBuffer(keys, numKeys);
    char *buf = key;
    while (numKeys-- > 0) {
      buf = uw_Sqlcache_keyCopy(buf, keys[numKeys]);
      size_t len = buf - key;

      entry = uw_Sqlcache_find(cache, key, len, 1);
      if (!entry) {
        entry = calloc(1, sizeof(uw_Sqlcache_Entry));
        entry->key = strdup(key);
        entry->value = NULL;
        entry->timeInvalid = 0;
        uw_Sqlcache_add(cache, entry, len);
      }
    }
    free(key);
  }
  if (!entry->value || entry->value->timeValid < value->timeValid) {
    uw_Sqlcache_freeValue(entry->value);
    entry->value = value;
    entry->value->timeValid = timeNow;
  } else
    // Older than what the entry has: not stored, and ours to free.
    uw_Sqlcache_freeValue(value);
  pthread_rwlock_unlock(&cache->lockIn);
}

static void uw_Sqlcache_flushCommitOne(uw_Sqlcache_Cache *cache, char **keys) {
  (void)cache;
  (void)keys;
}

static void uw_Sqlcache_commit(void *data) {
  uw_context ctx = (uw_context)data;
  uw_Sqlcache_Update *update = ctx->cacheUpdate;
  while (update) {
    uw_Sqlcache_Cache *cache = update->cache;
    char **keys = update->keys;
    if (update->value) {
      uw_Sqlcache_storeCommitOne(cache, keys, update->value);
      // The cache has it now, or has freed it.
      update->value = NULL;
    } else {
      uw_Sqlcache_flushCommitOne(cache, keys);
    }
    update = update->next;
  }
}

static void uw_Sqlcache_unlockOut(uw_Sqlcache_Lock *l, int write) {
  pthread_mutex_lock(&l->mutex);
  if (write)
    l->writer = 0;
  else
    --l->readers;
  pthread_cond_broadcast(&l->changed);
  pthread_mutex_unlock(&l->mutex);
}

static void uw_Sqlcache_free(void *data, int dontCare) {
  (void)dontCare;
  uw_context ctx = (uw_context)data;
  uw_Sqlcache_Update *update = ctx->cacheUpdate;
  while (update) {
    char** keys = update->keys;
    size_t numKeys = update->cache->numKeys;
    while (numKeys-- > 0) {
      free(keys[numKeys]);
    }
    free(keys);
    // A value still here did not reach the cache: the transaction was rolled
    // back, so the commit callback never ran.
    uw_Sqlcache_freeValue(update->value);
    uw_Sqlcache_Update *nextUpdate = update->next;
    free(update);
    update = nextUpdate;
  }
  ctx->cacheUpdate = NULL;
  ctx->cacheUpdateTail = NULL;
  uw_Sqlcache_Unlock *unlock = ctx->cacheUnlock;
  while (unlock) {
    uw_Sqlcache_unlockOut(unlock->lock, unlock->write);
    uw_Sqlcache_Unlock *nextUnlock = unlock->next;
    free(unlock);
    unlock = nextUnlock;
  }
  ctx->cacheUnlock = NULL;
}

static void uw_Sqlcache_pushUnlock(uw_context ctx, uw_Sqlcache_Lock *lock, int write) {
  if (!ctx->cacheUnlock) {
    // Just need one registered commit for both updating and unlocking.
    uw_register_transactional(ctx, ctx, uw_Sqlcache_commit, NULL, uw_Sqlcache_free);
  }
  uw_Sqlcache_Unlock *unlock = malloc(sizeof(uw_Sqlcache_Unlock));
  unlock->lock = lock;
  unlock->write = write;
  unlock->next = ctx->cacheUnlock;
  ctx->cacheUnlock = unlock;
}

void uw_Sqlcache_rlock(uw_context ctx, uw_Sqlcache_Cache *cache) {
  uw_Sqlcache_Lock *l = &cache->lockOut;

  pthread_mutex_lock(&l->mutex);
  while (l->writer || l->writers_waiting)
    pthread_cond_wait(&l->changed, &l->mutex);
  ++l->readers;
  pthread_mutex_unlock(&l->mutex);
  uw_Sqlcache_pushUnlock(ctx, l, 0);
}

void uw_Sqlcache_wlock(uw_context ctx, uw_Sqlcache_Cache *cache) {
  uw_Sqlcache_Lock *l = &cache->lockOut;

  pthread_mutex_lock(&l->mutex);
  ++l->writers_waiting;
  while (l->writer || l->readers)
    pthread_cond_wait(&l->changed, &l->mutex);
  --l->writers_waiting;
  l->writer = 1;
  pthread_mutex_unlock(&l->mutex);
  uw_Sqlcache_pushUnlock(ctx, l, 1);
}

static char **uw_Sqlcache_copyKeys(char **keys, size_t numKeys) {
  char **copy = malloc(sizeof(char *) * numKeys);
  while (numKeys-- > 0) {
    char *k = keys[numKeys];
    copy[numKeys] = k ? strdup(k) : NULL;
  }
  return copy;
}

void uw_Sqlcache_store(uw_context ctx, uw_Sqlcache_Cache *cache, char **keys, uw_Sqlcache_Value *value) {
  uw_Sqlcache_Update *update = malloc(sizeof(uw_Sqlcache_Update));
  update->cache = cache;
  update->keys = uw_Sqlcache_copyKeys(keys, cache->numKeys);
  update->value = value;
  update->next = NULL;
  // Can't use [uw_Sqlcache_getTimeNow] because it modifies state and we don't have the lock.
  pthread_rwlock_rdlock(&cache->lockIn);
  value->timeValid = cache->timeNow;
  pthread_rwlock_unlock(&cache->lockIn);
  if (ctx->cacheUpdateTail) {
    ctx->cacheUpdateTail->next = update;
  } else {
    ctx->cacheUpdate = update;
  }
  ctx->cacheUpdateTail = update;
}

void uw_Sqlcache_flush(uw_context ctx, uw_Sqlcache_Cache *cache, char **keys) {
  (void)ctx;
  // A flush has to happen immediately so that subsequent stores in the same transaction fail.
  // This is safe to do because we will always call [uw_Sqlcache_wlock] earlier.
  // If the transaction fails, the only harm done is a few extra cache misses.
  pthread_rwlock_wrlock(&cache->lockIn);
  size_t numKeys = cache->numKeys;
  if (numKeys == 0) {
    uw_Sqlcache_Entry *entry = cache->table;
    if (entry) {
      uw_Sqlcache_freeValue(entry->value);
      entry->value = NULL;
    }
  } else {
    char *key = uw_Sqlcache_allocKeyBuffer(keys, numKeys);
    char *buf = key;
    time_t timeNow = uw_Sqlcache_getTimeNow(cache);
    while (numKeys-- > 0) {
      char *k = keys[numKeys];
      if (!k) {
        size_t len = buf - key;
        if (len == 0) {
          // The first key was null.
          cache->timeInvalid = timeNow;
        } else {
          uw_Sqlcache_Entry *entry = uw_Sqlcache_find(cache, key, len, 0);
          if (entry) {
            entry->timeInvalid = timeNow;
          }
        }
        free(key);
        pthread_rwlock_unlock(&cache->lockIn);
        return;
      }
      buf = uw_Sqlcache_keyCopy(buf, k);
    }
    // All the keys were non-null, so we delete the pointed-to entry.
    size_t len = buf - key;
    uw_Sqlcache_Entry *entry = uw_Sqlcache_find(cache, key, len, 0);
    free(key);
    uw_Sqlcache_delete(cache, entry);
  }
  pthread_rwlock_unlock(&cache->lockIn);
}

int strcmp_nullsafe(const char *str1, const char *str2) {
  if (str1)
    return strcmp(str1, str2);
  else
    return 1;
}

static int is_valid_hash(uw_Basis_string hash) {
  for (; *hash; ++hash)
    if (!U8_IS_SINGLE(*hash) || !isxdigit(*hash))
      return 0;

  return 1;
}

uw_unit uw_Basis_cache_file(uw_context ctx, uw_Basis_blob contents) {
  char *dir = ctx->app->file_cache, path[1024], tempfile[1024];
  unsigned char *res, *hash;
  char *hash_encoded;
  int fd, len, i;
  ssize_t written_so_far = 0;

  if (!dir)
    return uw_unit_v;

  hash = uw_malloc(ctx, SHA512_DIGEST_LENGTH);
  res = SHA512((unsigned char *)contents.data, contents.size, hash);
  if (!res)
    uw_error(ctx, FATAL, "Can't hash file contents");

  hash_encoded = uw_malloc(ctx, SHA512_DIGEST_LENGTH * 2 + 1);
  for (i = 0; i < SHA512_DIGEST_LENGTH; ++i)
    sprintf(hash_encoded + 2 * i, "%02x", (int)hash[i]);
  hash_encoded[SHA512_DIGEST_LENGTH * 2] = 0;

  len = snprintf(tempfile, sizeof tempfile, "%s/tmpXXXXXX", dir);
  if (len < 0 || len >= sizeof tempfile)
    uw_error(ctx, FATAL, "Error assembling file path for cache (temporary)");

  fd = mkstemp(tempfile);
  if (fd < 0)
    uw_error(ctx, FATAL, "Error creating temporary file %s for cache", tempfile);

  while (written_so_far < contents.size) {
    ssize_t written_just_now = write(fd, contents.data + written_so_far, contents.size - written_so_far);
    if (written_just_now <= 0) {
      close(fd);
      uw_error(ctx, FATAL, "Error writing all bytes to cached file");
    }
    written_so_far += written_just_now;
  }

  close(fd);

  len = snprintf(path, sizeof path, "%s/%s", dir, hash_encoded);
  if (len < 0 || len >= sizeof path)
    uw_error(ctx, FATAL, "Error assembling file path for cache");

  if (rename(tempfile, path))
    uw_error(ctx, FATAL, "Error renaming temporary file into cache");

  return uw_unit_v;
}

uw_Basis_blob uw_Basis_check_filecache(uw_context ctx, uw_Basis_string hash) {
  char path[1024], *dir = ctx->app->file_cache, *filedata;
  int len;
  long size, read_so_far = 0;
  FILE *fp;
  uw_Basis_blob res;

  // Hashes come formatted for printing by Postgres, which means they start with
  // two extra characters.  Let's remove them.
  if (hash[0] == '\\' && hash[1] == 'x')
    hash += 2;

  if (!dir)
    uw_error(ctx, FATAL, "Checking file cache when no directory is set");

  if (!is_valid_hash(hash))
    uw_error(ctx, FATAL, "Checking file cache with invalid hash %s", hash);

  len = snprintf(path, sizeof path, "%s/%s", dir, hash);
  if (len < 0 || len >= sizeof path)
    uw_error(ctx, FATAL, "Error assembling file path for cache");

  fp = fopen(path, "r");
  if (!fp) {
    ctx->file_cache_missed = 1;
    uw_error(ctx, UNLIMITED_RETRY, "Missed in the file cache for hash %s", hash);
  }
  uw_push_cleanup(ctx, (void (*)(void *))fclose, fp);

  if (fseek(fp, 0L, SEEK_END))
    uw_error(ctx, FATAL, "Error seeking to end of cached file");

  size = ftell(fp);
  if (size < 0)
    uw_error(ctx, FATAL, "Error getting size of cached file");

  rewind(fp);
  filedata = uw_malloc(ctx, size);

  while (read_so_far < size) {
    size_t just_read = fread(filedata + read_so_far, 1, size - read_so_far, fp);
    if (just_read <= 0)
      uw_error(ctx, FATAL, "Error reading all bytes of cached file");
    read_so_far += just_read;
  }

  uw_pop_cleanup(ctx);

  res.size = size;
  res.data = filedata;
  return res;
}

uw_Basis_bool uw_Basis_filecache_missed(uw_context ctx) {
  return !!(ctx->file_cache_missed);
}
