#include <sqlite3.h>

#include "insert.h"

uw_unit uw_Insert_row(uw_context ctx, uw_Basis_string table, uw_Basis_int n) {
  sqlite3 *db;
  char *sql;
  int r;

  uw_ensure_transaction(ctx);
  db = *(sqlite3 **)uw_get_db(ctx);
  sql = sqlite3_mprintf("INSERT INTO %s (uw_N) VALUES (%lld)", table, (long long)n);
  if (!sql)
    uw_error(ctx, FATAL, "Insert.row: out of memory");
  r = sqlite3_exec(db, sql, NULL, NULL, NULL);
  sqlite3_free(sql);
  if (r != SQLITE_OK)
    uw_error(ctx, FATAL, "Insert.row: %s", sqlite3_errmsg(db));
  return uw_unit_v;
}
