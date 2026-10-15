/* Probes for the io tests: see probe.urs. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sqlite3.h>
#include <urweb.h>
#include "probe.h"

static void say(uw_context ctx, const char *what, const char *tag) {
  uw_loggers *ls = uw_get_loggers(ctx);
  ls->log_debug(ls->logger_data, "%s%s%s\n", what, tag ? " " : "", tag ? tag : "");
}

static uw_Basis_int counter = 0;
uw_Basis_int uw_Probe_step(uw_context ctx) { (void)ctx; return ++counter; }
uw_Basis_int uw_Probe_stepT(uw_context ctx) { (void)ctx; return ++counter; }

/* The SQLite driver's connection record starts with the sqlite3 handle, and
   autocommit is off exactly while a transaction is open. */
static uw_Basis_bool in_transaction(uw_context ctx) {
  void *conn = uw_get_db(ctx);
  if (!conn)
    return uw_Basis_False;
  return sqlite3_get_autocommit(*(sqlite3 **)conn) ? uw_Basis_False : uw_Basis_True;
}
uw_Basis_bool uw_Probe_inTransaction(uw_context ctx) { return in_transaction(ctx); }
uw_Basis_bool uw_Probe_inTransactionT(uw_context ctx) { return in_transaction(ctx); }

uw_unit uw_Probe_effect(uw_context ctx, uw_Basis_string msg) { say(ctx, "effect:", msg); return uw_unit_v; }
uw_unit uw_Probe_effectT(uw_context ctx, uw_Basis_string msg) { say(ctx, "effect:", msg); return uw_unit_v; }

/* The keys seen so far, for what happens once per key. */
static char *seen[64];
static int nseen;
static pthread_mutex_t seen_lock = PTHREAD_MUTEX_INITIALIZER;

static int first_time(const char *key) {
  int i, found = 0;
  pthread_mutex_lock(&seen_lock);
  for (i = 0; i < nseen; ++i)
    if (!strcmp(seen[i], key))
      found = 1;
  if (!found && nseen < 64)
    seen[nseen++] = strdup(key);
  pthread_mutex_unlock(&seen_lock);
  return !found;
}

uw_unit uw_Probe_failOnce(uw_context ctx, uw_Basis_string key) {
  if (first_time(key))
    uw_error(ctx, BOUNDED_RETRY, "first try of %s", key);
  return uw_unit_v;
}

uw_unit uw_Probe_boundedRetry(uw_context ctx, uw_Basis_string msg) {
  uw_error(ctx, BOUNDED_RETRY, "%s", msg);
}

uw_unit uw_Probe_fatalIo(uw_context ctx, uw_Basis_string msg) {
  uw_error(ctx, FATAL, "%s", msg);
}

uw_unit uw_Probe_retryIo(uw_context ctx, uw_Basis_string msg) {
  uw_error(ctx, BOUNDED_RETRY, "%s", msg);
}

static int (*real_commit)(uw_context);

static int failing_commit(uw_context ctx) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  uw_get_app(ctx)->db_commit = real_commit;
  sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
  say(ctx, "db_commit: a serialization conflict", NULL);
  return -1;
}

uw_unit uw_Probe_armConflict(uw_context ctx, uw_Basis_string key) {
  uw_app *a = uw_get_app(ctx);
  if (first_time(key)) {
    real_commit = a->db_commit;
    a->db_commit = failing_commit;
  }
  return uw_unit_v;
}

/* A transactional that prints what the runtime does with it and frees its
   data, so that freeing it twice would show under a sanitizer. */
typedef struct {
  uw_context ctx;
  char *tag;
  char *fail;      /* set: the commit callback sets this error message */
  int raise;       /* set: the commit callback calls uw_error */
} logged;

static void tsay(logged *l, const char *what) {
  uw_loggers *ls = uw_get_loggers(l->ctx);
  ls->log_debug(ls->logger_data, "transactional %s: %s\n", l->tag, what);
}

static void tx_commit(void *d) {
  logged *l = d;
  tsay(l, "commit");
  if (l->fail)
    uw_set_error_message(l->ctx, "%s", l->fail);
  if (l->raise)
    uw_error(l->ctx, BOUNDED_RETRY, "raised in the commit callback of %s", l->tag);
}
static void tx_rollback(void *d) { tsay(d, "rollback"); }
static void tx_free(void *d, int will_retry) {
  logged *l = d;
  tsay(l, will_retry ? "free, will retry" : "free");
  free(l->tag);
  free(l->fail);
  free(l);
}

static uw_unit reg(uw_context ctx, uw_Basis_string tag, int with_rollback, const char *fail, int raise) {
  logged *l = malloc(sizeof *l);
  l->ctx = ctx;
  l->tag = strdup(tag);
  l->fail = fail ? strdup(fail) : NULL;
  l->raise = raise;
  if (uw_register_transactional(ctx, l, tx_commit, with_rollback ? tx_rollback : NULL, tx_free))
    uw_error(ctx, FATAL, "too many transactionals");
  return uw_unit_v;
}

uw_unit uw_Probe_register(uw_context ctx, uw_Basis_string tag) { return reg(ctx, tag, 1, NULL, 0); }
uw_unit uw_Probe_registerNoRollback(uw_context ctx, uw_Basis_string tag) { return reg(ctx, tag, 0, NULL, 0); }
uw_unit uw_Probe_registerIo(uw_context ctx, uw_Basis_string tag) { return reg(ctx, tag, 1, NULL, 0); }
uw_unit uw_Probe_failInCommitBeforeDb(uw_context ctx, uw_Basis_string tag) { return reg(ctx, tag, 1, tag, 0); }
uw_unit uw_Probe_failInCommit(uw_context ctx, uw_Basis_string tag) { return reg(ctx, tag, 0, tag, 0); }
uw_unit uw_Probe_raiseInCommit(uw_context ctx, uw_Basis_string tag) { return reg(ctx, tag, 0, NULL, 1); }

typedef struct { uw_context ctx; char *tag; } cleanup_tag;

static void run_cleanup(void *d) {
  cleanup_tag *c = d;
  say(c->ctx, "cleanup ran:", c->tag);
  free(c->tag);
  free(c);
}

static uw_unit push(uw_context ctx, uw_Basis_string tag) {
  cleanup_tag *c = malloc(sizeof *c);
  c->ctx = ctx;
  c->tag = strdup(tag);
  uw_push_cleanup(ctx, run_cleanup, c);
  return uw_unit_v;
}

uw_unit uw_Probe_pushCleanupT(uw_context ctx, uw_Basis_string tag) { return push(ctx, tag); }
uw_unit uw_Probe_pushCleanupIo(uw_context ctx, uw_Basis_string tag) { return push(ctx, tag); }
uw_unit uw_Probe_popCleanupT(uw_context ctx) { uw_pop_cleanup(ctx); return uw_unit_v; }
uw_unit uw_Probe_popCleanupIo(uw_context ctx) { uw_pop_cleanup(ctx); return uw_unit_v; }

/* One statement per process is enough for the tests. */
static sqlite3_stmt *pending = NULL;

uw_unit uw_Probe_pendingWrite(uw_context ctx, uw_Basis_string table) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  char sql[256];
  snprintf(sql, sizeof sql, "INSERT INTO %s (uw_N) VALUES (99) RETURNING uw_N", table);
  if (sqlite3_prepare_v2(db, sql, -1, &pending, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "pendingWrite: %s", sqlite3_errmsg(db));
  if (sqlite3_step(pending) != SQLITE_ROW)
    uw_error(ctx, FATAL, "pendingWrite: no row: %s", sqlite3_errmsg(db));
  return uw_unit_v;
}

static uw_unit finish(uw_context ctx) {
  (void)ctx;
  if (pending) {
    sqlite3_finalize(pending);
    pending = NULL;
  }
  return uw_unit_v;
}
uw_unit uw_Probe_finishPendingWrite(uw_context ctx) { return finish(ctx); }
uw_unit uw_Probe_finishPendingWriteIo(uw_context ctx) { return finish(ctx); }

uw_unit uw_Probe_sleepIo(uw_context ctx, uw_Basis_int ms) { (void)ctx; usleep(ms * 1000); return uw_unit_v; }
uw_unit uw_Probe_sleepT(uw_context ctx, uw_Basis_int ms) { (void)ctx; usleep(ms * 1000); return uw_unit_v; }

/* The statements SQLite runs, as it runs them: BEGIN and COMMIT as well as
   the program's own, with the parameters filled in. */
static int trace_statement(unsigned type, void *c, void *stmt, void *x) {
  (void)type;
  (void)x;
  char *sql = sqlite3_expanded_sql(stmt);
  say(c, "sql:", sql ? sql : "?");
  sqlite3_free(sql);
  return 0;
}

uw_unit uw_Probe_traceSqlIo(uw_context ctx) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  sqlite3_trace_v2(db, SQLITE_TRACE_STMT, trace_statement, ctx);
  return uw_unit_v;
}
