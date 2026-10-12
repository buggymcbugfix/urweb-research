#include <sqlite3.h>
#include <urweb.h>
#include "conflict.h"

/* A counter in C: a rolled-back transaction does not reset it. */
uw_Basis_int uw_Conflict_step(uw_context ctx) { static uw_Basis_int n = 0; (void)ctx; return ++n; }

static int (*real_commit)(uw_context);
static int armed;

/* What the Postgres driver does at COMMIT on SQLSTATE 40001: the server has
   already ended the transaction, and the driver returns -1. */
static int failing_commit(uw_context ctx) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  uw_get_app(ctx)->db_commit = real_commit;
  armed = 0;
  sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
  return -1;
}

uw_unit uw_Conflict_armOnce(uw_context ctx) {
  uw_app *a = uw_get_app(ctx);
  if (!armed) { real_commit = a->db_commit; a->db_commit = failing_commit; armed = 1; }
  return uw_unit_v;
}

uw_Basis_bool uw_Conflict_inTxn(uw_context ctx) {
  sqlite3 *db = *(sqlite3 **)uw_get_db(ctx);
  return !sqlite3_get_autocommit(db);
}
