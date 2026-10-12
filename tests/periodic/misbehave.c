#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <urweb.h>
#include "misbehave.h"

uw_Basis_int uw_Misbehave_step(uw_context ctx) {
  static uw_Basis_int n = 0;
  (void)ctx;
  return ++n;
}

typedef struct { uw_context ctx; char *msg; } job;

static void commit(void *data) {
  job *j = data;
  uw_set_error_message(j->ctx, "%s", j->msg);
}

static void free_job(void *data, int will_retry) {
  job *j = data;
  (void)will_retry;
  free(j->msg);
  free(j);
}

uw_unit uw_Misbehave_failInCommit(uw_context ctx, uw_Basis_string msg) {
  job *j = malloc(sizeof(job));
  j->ctx = ctx;
  j->msg = strdup(msg);
  uw_register_transactional(ctx, j, commit, NULL, free_job);
  return uw_unit_v;
}

uw_unit uw_Misbehave_boundedRetry(uw_context ctx, uw_Basis_string msg) {
  uw_error(ctx, BOUNDED_RETRY, "%s", msg);
}

uw_unit uw_Misbehave_fatalWithoutTransaction(uw_context ctx, uw_Basis_string msg) {
  /* The sqlite driver's connection record starts with the sqlite3 handle. */
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  sqlite3_exec(db, "COMMIT", NULL, NULL, NULL);
  uw_error(ctx, FATAL, "%s", msg);
}
