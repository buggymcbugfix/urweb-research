#include <sqlite3.h>
#include <urweb.h>
#include "iocheck.h"

uw_Basis_int uw_Iocheck_step(uw_context ctx) {
  static uw_Basis_int n = 0;
  (void)ctx;
  return ++n;
}

static uw_Basis_bool in_transaction(uw_context ctx) {
  /* The sqlite driver's connection record starts with the sqlite3 handle;
     autocommit is off exactly while a transaction is open. */
  void *conn = uw_get_db(ctx);
  if (!conn)
    return uw_Basis_False;
  return sqlite3_get_autocommit(*(sqlite3 **)conn) ? uw_Basis_False : uw_Basis_True;
}

uw_Basis_bool uw_Iocheck_inTransaction(uw_context ctx) { return in_transaction(ctx); }
uw_Basis_bool uw_Iocheck_inTransactionT(uw_context ctx) { return in_transaction(ctx); }

uw_unit uw_Iocheck_boundedRetryIo(uw_context ctx, uw_Basis_string msg) {
  uw_error(ctx, BOUNDED_RETRY, "%s", msg);
}

uw_unit uw_Iocheck_effect(uw_context ctx, uw_Basis_string msg) {
  uw_loggers *ls = uw_get_loggers(ctx);
  ls->log_debug(ls->logger_data, "effect: %s\n", msg);
  return uw_unit_v;
}
