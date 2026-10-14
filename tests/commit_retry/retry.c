#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <urweb.h>
#include "retry.h"

/* A counter in C: a rolled-back transaction does not reset it. */
uw_Basis_int uw_Retry_step(uw_context ctx) { static uw_Basis_int n = 0; (void)ctx; return ++n; }

static int conflicts(void) {
  const char *s = getenv("COMMIT_RETRY_CONFLICTS");
  return s ? atoi(s) : 1;
}

/* How many times a handler or task runs before its COMMIT goes through. */
uw_Basis_int uw_Retry_attempts(uw_context ctx) { (void)ctx; return conflicts() + 1; }

static int (*real_commit)(uw_context);
static int left;

/* What the Postgres driver does at COMMIT on SQLSTATE 40001: the server has
   already ended the transaction, and the driver returns -1. */
static int failing_commit(uw_context ctx) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  if (--left <= 0)
    uw_get_app(ctx)->db_commit = real_commit;
  sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
  return -1;
}

/* The next COMMITs fail, COMMIT_RETRY_CONFLICTS of them (one if unset). */
uw_unit uw_Retry_arm(uw_context ctx) {
  uw_app *a = uw_get_app(ctx);
  if (left <= 0) {
    left = conflicts();
    real_commit = a->db_commit;
    a->db_commit = failing_commit;
  }
  return uw_unit_v;
}

uw_Basis_bool uw_Retry_inTxn(uw_context ctx) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  return !sqlite3_get_autocommit(db);
}

/* Transactionals that say on stdout what is done to them. */
static void on_commit(void *data) { printf("%s: commit\n", (char *)data); fflush(stdout); }
static void on_rollback(void *data) { printf("%s: rollback\n", (char *)data); fflush(stdout); }
static void on_free(void *data, int will_retry) {
  printf("%s: free, will retry: %d\n", (char *)data, will_retry);
  fflush(stdout);
  free(data);
}

uw_unit uw_Retry_watch(uw_context ctx, uw_Basis_string name) {
  uw_register_transactional(ctx, strdup(name), on_commit, on_rollback, on_free);
  return uw_unit_v;
}

uw_unit uw_Retry_watchAfter(uw_context ctx, uw_Basis_string name) {
  uw_register_transactional(ctx, strdup(name), on_commit, NULL, on_free);
  return uw_unit_v;
}
