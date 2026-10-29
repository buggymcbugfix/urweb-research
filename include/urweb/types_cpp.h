#ifndef URWEB_TYPES_CPP_H
#define URWEB_TYPES_CPP_H

#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <setjmp.h>
#include <unicode/utypes.h>

typedef long long uw_Basis_int;
typedef double uw_Basis_float;
typedef char* uw_Basis_string;
typedef UChar32 uw_Basis_char;
typedef struct {
  time_t seconds;
  unsigned microseconds;
} uw_Basis_time;
typedef struct {
  size_t size;
  char *data;
} uw_Basis_blob;

typedef int uw_unit;
typedef uw_unit uw_Basis_unit;

typedef enum uw_Basis_bool { uw_Basis_False, uw_Basis_True } uw_Basis_bool;

typedef uw_Basis_string uw_Basis_xhtml;
typedef uw_Basis_string uw_Basis_page;
typedef uw_Basis_string uw_Basis_xbody;
typedef uw_Basis_string uw_Basis_css_class;

typedef unsigned uw_Basis_client;
typedef struct {
  unsigned cli, chn;
} uw_Basis_channel;

typedef struct {
  int context;
  unsigned long long source;
} uw_Basis_source;

typedef struct uw_Basis_file {
  uw_Basis_string name, type;
  uw_Basis_blob data;
} uw_Basis_file;

typedef struct uw_Basis_postBody {
  uw_Basis_string type, data;
  size_t len;
} uw_Basis_postBody;

typedef uw_Basis_string uw_Basis_queryString;

typedef struct {
  uw_Basis_string name, value, remaining;
} uw_Basis_postField;

typedef enum { SUCCESS, FATAL, BOUNDED_RETRY, UNLIMITED_RETRY, RETURN_INDIRECTLY } failure_kind;

// How many times a transaction is run when its failures ask for a bounded
// retry (BOUNDED_RETRY): the first attempt and, after each such failure,
// another, this many in all.  One number for every loop that runs a
// transaction again: a request, a periodic task, a context's
// initialization, and a transaction of io code; see uw_retry_again.
#define UW_RETRY_ATTEMPTS 5

typedef enum { SERVED, KEEP_OPEN, FAILED } request_result;

#define INTS_MAX 50
#define FLOATS_MAX 100
#define TIMES_MAX 100

typedef void (*uw_callback)(void *);
typedef void (*uw_callback_with_retry)(void *, int will_retry);
typedef void (*uw_logger)(void*, const char *fmt, ...);

// The levels of the runtime's log lines, numbered as in syslog: a line that
// starts with <3> reaches the journal at priority err, and so on.
typedef enum {
  UW_LOG_ERR = 3,     // something failed that needs a person to look at it
  UW_LOG_WARNING = 4, // something unusual that the program handled
  UW_LOG_NOTICE = 5,  // a request that could not be served as it was sent:
                      // a page that is none, an input that is none
  UW_LOG_INFO = 6,    // what happens routinely: the line for each request
  UW_LOG_DEBUG = 7    // what the runtime does, step by step
} uw_log_level;

struct uw_context;

typedef struct {
  void (*callback)(struct uw_context *);
  unsigned int period;
  int io; // the body is an io computation, run outside any transaction
  const char *where; // where the task is declared, for its error lines
} uw_periodic;

// The state of one transaction run inline in an io computation, by the code
// the compiler generates for Basis.runTransaction; see uw_io_transaction_*.
typedef struct {
  jmp_buf outer;    // the jump target to restore afterwards
  int attempts_left; // for uw_retry_again
  int failed;       // the transaction gave up: rolled back, error message kept
  int committing;   // inside uw_commit: a longjmp from there is a callback's
  // The context as it was when the transaction began, for an attempt that
  // does not commit to go back to
  size_t headers_used, script_used;
  char *heap_front;
  void *regions;
  size_t transactionals_base, cleanup_base;
  int num_recording;       // the page recordings the SQL cache has open
} uw_io_transaction;

// The state of an io function run by an RPC handler (Basis.io_rpc); see
// uw_io_request_begin and uw_io_request_end.
typedef struct {
  jmp_buf outer;    // the request's jump target, restored afterwards
} uw_io_request;

// A file of a `file` directive, baked into the executable: served at its URI
// by uw_handle, and read by Basis functions.  The bytes are a literal in the
// generated C.
typedef struct {
  const char *uri;
  const char *type;   // its MIME type, or NULL when none is known
  size_t size;
  const char *data;
} uw_served_file;

typedef struct {
  int inputs_len, timeout;
  char *url_prefix;

  void (*client_init)();
  void (*initializer)(struct uw_context *);
  void (*expunger)(struct uw_context *, uw_Basis_client);

  void (*db_init)(struct uw_context *);
  int (*db_begin)(struct uw_context *, int could_write);
  int (*db_commit)(struct uw_context *);
  int (*db_rollback)(struct uw_context *);
  void (*db_close)(struct uw_context *);
  // 0 when the database is as the program expects; otherwise 1, with what is
  // wrong as the context's error message (Basis.validateDb).
  int (*db_check)(struct uw_context *);

  void (*handle)(struct uw_context *, char *);

  int (*input_num)(const char*);
  uw_Basis_string (*cookie_sig)(struct uw_context *);
  int (*check_url)(const char *);
  int (*check_mime)(const char *);
  int (*check_requestHeader)(const char *);
  int (*check_responseHeader)(const char *);
  int (*check_envVar)(const char *);
  int (*check_meta)(const char *);

  void (*on_error)(struct uw_context *, char *);

  uw_periodic *periodics; // 0-terminated array

  uw_Basis_string time_format;

  int is_html5;
  char *file_cache;

  uw_served_file *served_files; // ended by an entry whose uri is NULL

  int io_handlers; // a request may run io code (an io RPC): the contexts
                   // that serve requests are marked for it (uw_io_prepare)
} uw_app;

// A protocol's loggers, which a context logs through: log_error at err,
// log_debug at info, both through uw_vlog.
typedef struct {
  /* uw_app *app; */
  void *logger_data;
  uw_logger log_error, log_debug;
} uw_loggers;

#define ERROR_BUF_LEN 10240

typedef struct {
  size_t max;
  char *start, *front, *back;
} uw_buffer;

// Caching

#include <pthread.h>
#include "uthash.h"

typedef struct uw_Sqlcache_Value {
  char *result;
  char *output;
  char *scriptOutput;
  unsigned long timeValid;
} uw_Sqlcache_Value;

typedef struct uw_Sqlcache_Entry {
  char *key;
  uw_Sqlcache_Value *value;
  unsigned long timeInvalid;
  UT_hash_handle hh;
} uw_Sqlcache_Entry;

// A cache's outer lock, held for a whole transaction: to read by one that
// reads the cached table, to write by one that writes it.  Written out,
// rather than a pthread_rwlock_t, so that readers wait behind a waiting
// writer on every platform; glibc's rwlock let readers in, and readers that
// kept coming and overlapping kept a writer out for good.
typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t changed;
  unsigned readers, writers_waiting;
  int writer;
} uw_Sqlcache_Lock;

#define UW_SQLCACHE_LOCK_INITIALIZER \
  { PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, 0, 0, 0 }

typedef struct uw_Sqlcache_Cache {
  uw_Sqlcache_Lock lockOut;
  pthread_rwlock_t lockIn;
  uw_Sqlcache_Entry *table;
  unsigned long timeInvalid;
  unsigned long timeNow;
  size_t numKeys;
  UT_hash_handle hh;
} uw_Sqlcache_Cache;

#endif
