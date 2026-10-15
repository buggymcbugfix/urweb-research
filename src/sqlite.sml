 (* Copyright (c) 2009-2010, Adam Chlipala
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - The names of contributors may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *)

structure SQLite :> SQLITE = struct

open Settings
open Print.PD
open Print

fun p_sql_type t =
    case t of
        Int => "integer"
      | Float => "real"
      | String => "text"
      | Char => "text"
      | Bool => "integer"
      | Time => "text"
      | Blob => "blob"
      | Channel => "integer"
      | Client => "integer"
      | Nullable t => p_sql_type t

val ident = String.translate (fn #"'" => "PRIME"
                               | ch => str ch)

(* How long a connection waits for a lock that another connection holds, in
 * milliseconds: SQLite's busy_timeout, set on every connection. *)
val busyTimeout = "10000"

(* What a statement, a BEGIN IMMEDIATE or a draw from a sequence does when
 * SQLite has found the database busy: the request is run again, with no
 * pause.  A transaction that may write has held the write lock since its
 * BEGIN, so a statement that is busy all the same has already waited for
 * busy_timeout inside SQLite, and a pause here would keep the transaction,
 * and its locks, open for nothing: uw_error is what unwinds to the
 * ROLLBACK. *)
val busyRetry =
    box [string "uw_error(ctx, UNLIMITED_RETRY, \"Database is busy\");",
         newline]

fun checkRel (table, checkNullable) (s, xts) =
    let
        (* The catalog stores the bare name, without the quotes an identifier
         * may carry in generated SQL (and quotes would not survive inside the
         * C string literals below). *)
        val s = Settings.unquoteSql s
        val q = "SELECT COUNT(*) FROM sqlite_schema WHERE name = '" ^ s ^ "' COLLATE NOCASE"
    in
        box [string "if (sqlite3_prepare_v2(conn->conn, \"",
             string q,
             string "\", -1, &stmt, NULL) != SQLITE_OK) {",
             newline,
             box [string "sqlite3_close(conn->conn);",
                  newline,
                  string "uw_error(ctx, FATAL, \"Query preparation failed:<br />",
                  string q,
                  string "\");",
                  newline],
             string "}",
             newline,
             newline,

             (* busy_timeout has had sqlite3_step wait for the lock
              * already.  The check runs on the first connection only,
              * before any thread starts (see uw_db_validate), so failing
              * here ends the start-up, and the caller sees it in the exit
              * status; waiting on had no bound and said nothing. *)
             string "res = sqlite3_step(stmt);",
             newline,
             newline,
             string "if (res == SQLITE_BUSY) {",
             newline,
             box [string "sqlite3_finalize(stmt);",
                  newline,
                  string "sqlite3_close(conn->conn);",
                  newline,
                  string "uw_error(ctx, FATAL, \"Database is busy; can't validate:<br />",
                  string q,
                  string "\");",
                  newline],
             string "}",
             newline,
             newline,
             string "if (res == SQLITE_DONE) {",
             newline,
             box [string "sqlite3_finalize(stmt);",
                  newline,
                  string "sqlite3_close(conn->conn);",
                  newline,
                  string "uw_error(ctx, FATAL, \"No row returned:<br />",
                  string q,
                  string "\");",
                  newline],
             string "}",
             newline,
             newline,
             string "if (res != SQLITE_ROW) {",
             newline,
             box [string "sqlite3_finalize(stmt);",
                  newline,
                  string "sqlite3_close(conn->conn);",
                  newline,
                  string "uw_error(ctx, FATAL, \"Error getting row:<br />",
                  string q,
                  string "\");",
                  newline],
             string "}",
             newline,
             newline,

             string "if (sqlite3_column_count(stmt) != 1) {",
             newline,
             box [string "sqlite3_finalize(stmt);",
                  newline,
                  string "sqlite3_close(conn->conn);",
                  newline,
                  string "uw_error(ctx, FATAL, \"Bad column count:<br />",
                  string q,
                  string "\");",
                  newline],
             string "}",
             newline,
             newline,

             string "if (sqlite3_column_int(stmt, 0) != 1) {",
             newline,
             box [string "sqlite3_finalize(stmt);",
                  newline,
                  string "sqlite3_close(conn->conn);",
                  newline,
                  string "uw_error(ctx, FATAL, \"Table '",
                  string s,
                  string "' does not exist.\");",
                  newline],
             string "}",
             newline,
             newline,
             string "sqlite3_finalize(stmt);",
             newline]
    end

fun init {dbstring, prepared = ss, tables, views, sequences} =
    let
        val db = ref dbstring
    in
        app (fn s =>
                case String.fields (fn ch => ch = #"=") s of
                    [name, value] =>
                    (case name of
                         "dbname" => db := value
                       | _ => ())
                  | _ => ()) (String.tokens Char.isSpace dbstring);

        box [string "typedef struct {",
             newline,
             box [string "sqlite3 *conn;",
                  newline,
                  p_list_sepi (box [])
                              (fn i => fn _ =>
                                          box [string "sqlite3_stmt *p",
                                               string (Int.toString i),
                                               string ";",
                                               newline])
                              ss],
             string "} uw_conn;",
             newline,
             newline,

             (* SQLite asks its VFS for the time, which is where
              * CURRENT_TIMESTAMP comes from, and for the seed of its own
              * random number generator, which is where RANDOM comes from.
              * When the runtime's clock or its random numbers are pinned
              * (reproducible.c), a copy of the default VFS that reads the
              * pinned clock, or the pinned numbers, takes its place, so
              * that what is stored agrees with [now] and the order of
              * RANDOM is that of [rand].  The time is in milliseconds
              * since noon on 24 November 4714 BC, or days as a double for
              * an old VFS.
              *
              * SQLite seeds its generator once and then runs on; reset,
              * it seeds again on the next draw, from the VFS, and so from
              * the numbers of the attempt that draws rather than from
              * every RANDOM drawn since the server started, or from what
              * an extension drew when the connection was opened.  The
              * runtime resets it at the start of every attempt, through
              * the hook. *)
             string "static sqlite3_vfs uw_pinned_vfs;",
             newline,
             newline,
             string "static int uw_pinned_clock_int64(sqlite3_vfs *vfs, sqlite3_int64 *out) {",
             newline,
             box [string "int64_t t = 0;",
                  newline,
                  string "(void)vfs;",
                  newline,
                  string "uw_reproducible_epoch(&t);",
                  newline,
                  string "*out = t * 1000 + 210866760000000LL;",
                  newline,
                  string "return SQLITE_OK;",
                  newline],
             string "}",
             newline,
             newline,
             string "static int uw_pinned_clock(sqlite3_vfs *vfs, double *out) {",
             newline,
             box [string "sqlite3_int64 t;",
                  newline,
                  string "uw_pinned_clock_int64(vfs, &t);",
                  newline,
                  string "*out = t / 86400000.0;",
                  newline,
                  string "return SQLITE_OK;",
                  newline],
             string "}",
             newline,
             newline,
             string "static int uw_pinned_randomness(sqlite3_vfs *vfs, int n, char *out) {",
             newline,
             box [string "uint64_t r;",
                  newline,
                  string "int i;",
                  newline,
                  string "(void)vfs;",
                  newline,
                  string "for (i = 0; i < n; i += sizeof r) {",
                  newline,
                  box [string "uw_reproducible_rand(&r);",
                       newline,
                       string "memcpy(out + i, &r, n - i < (int)sizeof r ? n - i : (int)sizeof r);",
                       newline],
                  string "}",
                  newline,
                  string "return n;",
                  newline],
             string "}",
             newline,
             newline,
             string "static void uw_forget_seed(void) {",
             newline,
             box [string "sqlite3_randomness(0, NULL);",
                  newline],
             string "}",
             newline,
             newline,

             string "static void uw_client_init(void) {",
             newline,
             box [string "int64_t t;",
                  newline,
                  string "int pin_clock = uw_reproducible_epoch(&t), pin_rand = uw_reproducible_rand_pinned();",
                  newline,
                  string "if (pin_clock || pin_rand) {",
                  newline,
                  box [string "uw_pinned_vfs = *sqlite3_vfs_find(NULL);",
                       newline,
                       string "uw_pinned_vfs.zName = \"urweb-pinned\";",
                       newline,
                       string "if (pin_clock) {",
                       newline,
                       box [string "uw_pinned_vfs.xCurrentTime = uw_pinned_clock;",
                            newline,
                            string "if (uw_pinned_vfs.iVersion >= 2) { uw_pinned_vfs.xCurrentTimeInt64 = uw_pinned_clock_int64; }",
                            newline],
                       string "}",
                       newline,
                       string "if (pin_rand) {",
                       newline,
                       box [string "uw_pinned_vfs.xRandomness = uw_pinned_randomness;",
                            newline,
                            string "uw_reproducible_on_attempt(uw_forget_seed);",
                            newline],
                       string "}",
                       newline,
                       string "sqlite3_vfs_register(&uw_pinned_vfs, 1);",
                       newline],
                  string "}",
                  newline,
                  string "uw_sqlfmtInt = \"%lld%n\";",
                  newline,
                  string "uw_sqlfmtFloat = \"%.16g%n\";",
                  newline,
                  string "uw_Estrings = 0;",
                  newline,
                  string "uw_sql_type_annotations = 0;",
                  newline,
                  string "uw_sqlsuffixString = \"\";",
                  newline,
                  string "uw_sqlsuffixChar = \"\";",
                  newline,
                  string "uw_sqlsuffixBlob = \"\";",
                  newline,
                  string "uw_sqlfmtUint4 = \"%u%n\";",
                  newline],
             string "}",
             newline,
             newline,

             if #persistent (currentProtocol ()) then
                 (* The schema is checked on the process's first connection
                  * only, the one uw_request_init makes before any thread
                  * starts (and before the -d fork), so that a failure ends
                  * the start-up with the parent's exit status.  The
                  * threads' connections, and a reconnection, skip the
                  * check: they would find what the first one found.  The
                  * flag is set before any thread exists, so the threads
                  * read it without a lock. *)
                 box [string "static void uw_db_validate(uw_context ctx) {",
                      newline,
                      string "static int validated;",
                      newline,
                      string "uw_conn *conn = uw_get_db(ctx);",
                      newline,
                      string "sqlite3_stmt *stmt;",
                      newline,
                      string "int res;",
                      newline,
                      newline,
                      string "if (validated)",
                      newline,
                      box [string "return;",
                           newline],
                      newline,
                      p_list_sep newline (checkRel ("table", true)) tables,
                      p_list_sep newline (fn name => checkRel ("table", true)
                                                              (name, [("id", Settings.Client)])) sequences,
                      p_list_sep newline (checkRel ("view", false)) views,
                      newline,
                      string "validated = 1;",
                      newline,
                      string "}",
                      newline,
                      newline,

                      string "static void uw_db_prepare(uw_context ctx) {",
                      newline,
                      string "uw_conn *conn = uw_get_db(ctx);",
                      newline,
                      newline,

                      p_list_sepi newline (fn i => fn (s, _) =>
                                                      let
                                                          fun uhoh this s args =
                                                              box [p_list_sepi (box [])
                                                                               (fn j => fn () =>
                                                                                           box [string
                                                                                                    "sqlite3_finalize(conn->p",
                                                                                                string (Int.toString j),
                                                                                                string ");",
                                                                                                newline])
                                                                               (List.tabulate (i, fn _ => ())),
                                                                   box (if this then
                                                                            [string
                                                                                 "sqlite3_finalize(conn->p",
                                                                             string (Int.toString i),
                                                                             string ");",
                                                                             newline]
                                                                        else
                                                                            []),
                                                                   string "sqlite3_close(conn->conn);",
                                                                   newline,
                                                                   string "uw_error(ctx, FATAL, \"",
                                                                   string s,
                                                                   string "\"",
                                                                   p_list_sep (box []) (fn s => box [string ", ",
                                                                                                     string s]) args,
                                                                   string ");",
                                                                   newline]
                                                      in
                                                          box [string "if (sqlite3_prepare_v2(conn->conn, \"",
                                                               string (Prim.toCString s),
                                                               string "\", -1, &conn->p",
                                                               string (Int.toString i),
                                                               string ", NULL) != SQLITE_OK) {",
                                                               newline,
                                                               box [string "char msg[1024];",
                                                                    newline,
                                                                    string "strncpy(msg, sqlite3_errmsg(conn->conn), 1024);",
                                                                    newline,
                                                                    string "msg[1023] = 0;",
                                                                    newline,
                                                                    uhoh false ("Error preparing statement: "
                                                                                ^ Prim.toCString s ^ "<br />%s") ["msg"]],
                                                               string "}",
                                                               newline]
                                                      end)
                                  ss,

                      string "}"]
             else
                 box [string "static void uw_db_prepare(uw_context ctx) { }",
                      newline,
                      string "static void uw_db_validate(uw_context ctx) { }"],
             newline,
             newline,

             string "static void uw_db_init(uw_context ctx) {",
             newline,
             string "sqlite3 *sqlite;",
             newline,
             string "sqlite3_stmt *stmt;",
             newline,
             string "uw_conn *conn;",
             newline,
             newline,
             string "char *env_db_str = getenv(\"URWEB_SQLITE_DB_PATH\");",
             newline,
             string "const char *sqlite_db_path = env_db_str == NULL ? \"",
             string (Prim.toCString (!db)),
             string "\" : env_db_str;",
             newline,
             string "if (sqlite3_open_v2(sqlite_db_path, &sqlite, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) uw_error(ctx, FATAL, ",
             string "\"Can't open SQLite database: %s\", sqlite_db_path);",
             newline,
             newline,
             (* busy_timeout first: PRAGMA synchronous loads the schema,
              * and so needs a read lock, which another process may hold
              * for a while. *)
             string ("if (sqlite3_exec(sqlite, \"PRAGMA busy_timeout = " ^ busyTimeout ^ "\", NULL, NULL, NULL) != SQLITE_OK)"),
             newline,
             box [string "uw_error(ctx, FATAL, \"Can't set `PRAGMA busy_timeout` for SQLite database: %s\", sqlite3_errmsg(sqlite));",
                  newline],
             newline,
             string "if (sqlite3_exec(sqlite, \"PRAGMA foreign_keys = ON\", NULL, NULL, NULL) != SQLITE_OK)",
             newline,
             box [string "uw_error(ctx, FATAL, \"Can't enable foreign_keys for SQLite database: %s\", sqlite3_errmsg(sqlite));",
                  newline],
             newline,
             string "if (sqlite3_exec(sqlite, \"PRAGMA synchronous = FULL\", NULL, NULL, NULL) != SQLITE_OK)",
             newline,
             box [string "uw_error(ctx, FATAL, \"Can't enable `PRAGMA synchronous = FULL` for SQLite database: %s\", sqlite3_errmsg(sqlite));",
                  newline],
             newline,
             (* https://oldmoe.blog/2024/02/03/turn-on-mmap-support-for-your-sqlite-connections/ *)
             string "if (sqlite3_exec(sqlite, \"PRAGMA mmap_size = 268435456\", NULL, NULL, NULL) != SQLITE_OK)",
             newline,
             box [string "uw_error(ctx, FATAL, \"Can't set `PRAGMA mmap_size` for SQLite database: %s\", sqlite3_errmsg(sqlite));",
                  newline],
             newline,
             string "if (uw_database_max < SIZE_MAX) {",
             newline,
             box [string "char buf[100];",
                  newline,
                  newline,

                  string "sprintf(buf, \"PRAGMA max_page_count = %llu\", (unsigned long long)(uw_database_max / 1024));",
                  newline,
                  newline,

                  string "if (sqlite3_prepare_v2(sqlite, buf, -1, &stmt, NULL) != SQLITE_OK) {",
                  newline,
                  box [string "sqlite3_close(sqlite);",
                       newline,
                       string "uw_error(ctx, FATAL, \"Can't prepare max_page_count query for SQLite database\");",
                       newline],
                  string "}",
                  newline,
                  newline,

                  string "if (sqlite3_step(stmt) != SQLITE_ROW) {",
                  newline,
                  box [string "sqlite3_finalize(stmt);",
                       newline,
                       string "sqlite3_close(sqlite);",
                       newline,
                       string "uw_error(ctx, FATAL, \"Can't set max_page_count parameter for SQLite database\");",
                       newline],
                  string "}",
                  newline,
                  newline,

                  string "sqlite3_finalize(stmt);",
                  newline],
             string "}",
             newline,
             newline,

             string "conn = calloc(1, sizeof(uw_conn));",
             newline,
             string "conn->conn = sqlite;",
             newline,
             string "uw_set_db(ctx, conn);",
             newline,
             string "uw_db_validate(ctx);",
             newline,
             string "uw_db_prepare(ctx);",
             newline,
             string "}",
             newline,
             newline,

             string "static void uw_db_close(uw_context ctx) {",
             newline,
             string "uw_conn *conn = uw_get_db(ctx);",
             newline,
             p_list_sepi (box [])
                         (fn i => fn _ =>
                                     box [string "if (conn->p",
                                          string (Int.toString i),
                                          string ") sqlite3_finalize(conn->p",
                                          string (Int.toString i),
                                          string ");",
                                          newline])
                         ss,
             string "sqlite3_close(conn->conn);",
             newline,
             string "}",
             newline,
             newline,

             (* A transaction that may write takes the write lock at its
              * BEGIN (BEGIN IMMEDIATE).  After a plain BEGIN, the first
              * read takes a read lock and the first write asks to upgrade
              * it.  SQLite does not wait for an upgrade, whatever
              * busy_timeout says: under a rollback journal two connections
              * upgrading would wait for each other, and in WAL mode the
              * read snapshot is out of date once the other writer commits.
              * So the write fails with SQLITE_BUSY at once, and the
              * transaction has to be run again from the start.  The write
              * lock asked for before any read is waited for, up to
              * busy_timeout, and writers take turns instead.  A
              * transaction that cannot write keeps the plain BEGIN: it
              * never upgrades, and in WAL mode it does not wait for
              * writers.  could_write says whether the transaction may
              * write the database (see cjr_writes.sml); on Postgres it
              * decides READ ONLY.
              *
              * A transaction that may not wait for a lock (uw_db_may_wait)
              * asks for the write lock once: busy_timeout is off for its
              * BEGIN, which fails at once while another connection holds
              * the lock.  That is the expunger's, while the client pruner
              * keeps every request of the process out.
              *
              * A BEGIN that finds the database busy, after busy_timeout or
              * at once, asks for the transaction to be run again, as a busy
              * statement does.  A nonzero result would be taken for a lost
              * connection by uw_ensure_transaction, which reconnects and
              * then fails with FATAL.  Nothing needs rolling back: the
              * runtime sets transaction_started only after a successful
              * BEGIN. *)
             string "static int uw_db_begin(uw_context ctx, int could_write) {",
             newline,
             string "uw_conn *conn = uw_get_db(ctx);",
             newline,
             string "int may_wait = uw_db_may_wait(ctx);",
             newline,
             string "int r;",
             newline,
             newline,
             string "if (!may_wait) sqlite3_busy_timeout(conn->conn, 0);",
             newline,
             string "r = sqlite3_exec(conn->conn, could_write ? \"BEGIN IMMEDIATE\" : \"BEGIN\", NULL, NULL, NULL);",
             newline,
             string ("if (!may_wait) sqlite3_busy_timeout(conn->conn, " ^ busyTimeout ^ ");"),
             newline,
             newline,
             string "if (r == SQLITE_OK)",
             newline,
             box [string "return 0;",
                  newline],
             string "else if (r == SQLITE_BUSY) {",
             newline,
             busyRetry,
             string "} else {",
             newline,
             box [string "fprintf(stderr, \"Begin error: %s<br />\", sqlite3_errmsg(conn->conn));",
                  newline,
                  string "return 1;",
                  newline],
             string "}",
             newline,
             string "}",
             newline,
             string "static int uw_db_commit(uw_context ctx) {",
             newline,
             string "uw_conn *conn = uw_get_db(ctx);",
             newline,
             string "if (sqlite3_exec(conn->conn, \"COMMIT\", NULL, NULL, NULL) == SQLITE_OK)",
             newline,
             box [string "return 0;",
                  newline],
             string "else {",
             newline,
             box [string "fprintf(stderr, \"Commit error: %s<br />\", sqlite3_errmsg(conn->conn));",
                  newline,
                  string "return 1;",
                  newline],
             string "}",
             newline,
             string "}",
             newline,
             newline,

             string "static int uw_db_rollback(uw_context ctx) {",
             newline,
             string "uw_conn *conn = uw_get_db(ctx);",
             newline,
             string "if (sqlite3_exec(conn->conn, \"ROLLBACK\", NULL, NULL, NULL) == SQLITE_OK)",
             newline,
             box [string "return 0;",
                  newline],
             string "else {",
             newline,
             box [string "fprintf(stderr, \"Rollback error: %s<br />\", sqlite3_errmsg(conn->conn));",
                  newline,
                  string "return 1;",
                  newline],
             string "}",
             newline,
             string "}",
             newline,
             newline]
    end

val fmt = "\"%Y-%m-%d %H:%M:%S\""

fun p_getcol {loc, wontLeakStrings, col = i, typ = t} =
    let
        fun p_unsql t =
            case t of
                Int => box [string "sqlite3_column_int64(stmt, ", string (Int.toString i), string ")"]
              | Float => box [string "sqlite3_column_double(stmt, ", string (Int.toString i), string ")"]
              | String =>
                if wontLeakStrings then
                    box [string "(uw_Basis_string)sqlite3_column_text(stmt, ", string (Int.toString i), string ")"]
                else
                    box [string "uw_strdup(ctx, (uw_Basis_string)sqlite3_column_text(stmt, ", string (Int.toString i), string "))"]
              | Char => box [string "sqlite3_column_text(stmt, ", string (Int.toString i), string ")[0]"]
              | Bool => box [string "(uw_Basis_bool)sqlite3_column_int(stmt, ", string (Int.toString i), string ")"]
              | Time => box [string "uw_Basis_stringToTimef_error(ctx, ",
                             string fmt,
                             string ", (uw_Basis_string)sqlite3_column_text(stmt, ",
                             string (Int.toString i),
                             string "))"]
              | Blob => box [string "({",
                             newline,
                             string "char *data = (char *)sqlite3_column_blob(stmt, ",
                             string (Int.toString i),
                             string ");",
                             newline,
                             string "int len = sqlite3_column_bytes(stmt, ",
                             string (Int.toString i),
                             string ");",
                             newline,
                             string "uw_Basis_blob b = {len, uw_memdup(ctx, data, len)};",
                             newline,
                             string "b;",
                             newline,
                             string "})"]
              | Channel => box [string "({",
                                newline,
                                string "sqlite3_int64 n = sqlite3_column_int64(stmt, ",
                                string (Int.toString i),
                                string ");",
                                newline,
                                string "uw_Basis_channel ch = {n >> 32, n & 0xFFFFFFFF};",
                                newline,
                                string "ch;",
                                newline,
                                string "})"]
              | Client => box [string "sqlite3_column_int(stmt, ", string (Int.toString i), string ")"]

              | Nullable _ => raise Fail "Postgres: Recursive Nullable"

        fun getter t =
            case t of
                Nullable t =>
                box [string "(sqlite3_column_type(stmt, ",
                     string (Int.toString i),
                     string ") == SQLITE_NULL ? NULL : ",
                     case t of
                         String => getter t
                       | _ => box [string "({",
                                   newline,
                                   string (p_sql_ctype t),
                                   space,
                                   string "*tmp = uw_malloc(ctx, sizeof(",
                                   string (p_sql_ctype t),
                                   string "));",
                                   newline,
                                   string "*tmp = ",
                                   getter t,
                                   string ";",
                                   newline,
                                   string "tmp;",
                                   newline,
                                   string "})"],
                     string ")"]
              | _ =>
                box [string "(sqlite3_column_type(stmt, ",
                     string (Int.toString i),
                     string ") == SQLITE_NULL ? ",
                     box [string "({",
                          string (p_sql_ctype t),
                          space,
                          string "tmp;",
                          newline,
                          string "uw_error(ctx, FATAL, \"",
                          string (ErrorMsg.spanToString loc),
                          string ": Unexpectedly NULL field #",
                          string (Int.toString i),
                          string "\");",
                          newline,
                          string "tmp;",
                          newline,
                          string "})"],
                     string " : ",
                     p_unsql t,
                     string ")"]
    in
        getter t
    end

fun queryCommon {loc, query, cols, doCols} =
    box [string "int r;",
         newline,

         string "sqlite3_reset(stmt);",
         newline,

         string "uw_end_region(ctx);",
         newline,
         string "while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {",
         newline,
         doCols p_getcol,
         string "}",
         newline,
         newline,

         string "if (r == SQLITE_BUSY) {",
         newline,
         busyRetry,
         string "}",
         newline,
         newline,

         string "if (r != SQLITE_DONE) uw_error(ctx, FATAL, \"",
         string (ErrorMsg.spanToString loc),
         string ": query step failed: %s<br />%s\", ",
         query,
         string ", sqlite3_errmsg(conn->conn));",
         newline,
         newline]

fun query {loc, cols, doCols} =
    box [string "uw_conn *conn = uw_get_db(ctx);",
         newline,
         string "sqlite3_stmt *stmt;",
         newline,
         newline,
         string "if (sqlite3_prepare_v2(conn->conn, query, -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, \"Error preparing statement: %s<br />%s\", sqlite3_errmsg(conn->conn), query);",
         newline,
         newline,
         string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_finalize, stmt);",
         newline,
         newline,

         queryCommon {loc = loc, cols = cols, doCols = doCols, query = string "query"},

         string "uw_pop_cleanup(ctx);",
         newline]

val p_pre_inputs =
    p_list_sepi (box [])
                (fn i => fn t =>
                            case t of
                                Char => box [string "char arg",
                                             string (Int.toString (i + 1)),
                                             string "s = {arg",
                                             string (Int.toString (i + 1)),
                                             string ", 0};",
                                             newline]
                              | _ => box [])

fun p_inputs loc =
    p_list_sepi (box [])
                (fn i => fn t =>
                            let
                                fun bind (t, arg) =
                                    case t of
                                        Int => box [string "sqlite3_bind_int64(stmt, ",
                                                    string (Int.toString (i + 1)),
                                                    string ", ",
                                                    arg,
                                                    string ")"]
                                      | Float => box [string "sqlite3_bind_double(stmt, ",
                                                      string (Int.toString (i + 1)),
                                                      string ", ",
                                                      arg,
                                                      string ")"]
                                      | String => box [string "sqlite3_bind_text(stmt, ",
                                                       string (Int.toString (i + 1)),
                                                       string ", ",
                                                       arg,
                                                       string ", -1, SQLITE_TRANSIENT)"]
                                      | Char => box [string "sqlite3_bind_text(stmt, ",
                                                     string (Int.toString (i + 1)),
                                                     string ", ",
                                                     arg,
                                                     string "s, -1, SQLITE_TRANSIENT)"]
                                      | Bool => box [string "sqlite3_bind_int(stmt, ",
                                                     string (Int.toString (i + 1)),
                                                     string ", ",
                                                     arg,
                                                     string ")"]
                                      | Time => box [string "sqlite3_bind_text(stmt, ",
                                                     string (Int.toString (i + 1)),
                                                     string ", uw_Basis_timef(ctx, ",
                                                     string fmt,
                                                     string ", ",
                                                     arg,
                                                     string "), -1, SQLITE_TRANSIENT)"]
                                      | Blob => box [string "sqlite3_bind_blob(stmt, ",
                                                     string (Int.toString (i + 1)),
                                                     string ", ",
                                                     arg,
                                                     string ".data, ",
                                                     arg,
                                                     string ".size, SQLITE_TRANSIENT)"]
                                      | Channel => box [string "sqlite3_bind_int64(stmt, ",
                                                        string (Int.toString (i + 1)),
                                                        string ", ((sqlite3_int64)",
                                                        arg,
                                                        string ".cli << 32) | ",
                                                        arg,
                                                        string ".chn)"]
                                      | Client => box [string "sqlite3_bind_int(stmt, ",
                                                       string (Int.toString (i + 1)),
                                                       string ", ",
                                                       arg,
                                                       string ")"]
                                      | Nullable t => box [string "(",
                                                           arg,
                                                           string " == NULL ? sqlite3_bind_null(stmt, ",
                                                           string (Int.toString (i + 1)),
                                                           string ") : ",
                                                           bind (t, case t of
                                                                        String => arg
                                                                      | _ => box [string "(*", arg, string ")"]),
                                                           string ")"]
                            in
                                box [string "if (",
                                     bind (t, box [string "arg", string (Int.toString (i + 1))]),
                                     string " != SQLITE_OK) uw_error(ctx, FATAL, \"",
                                     string (ErrorMsg.spanToString loc),
                                     string ": Error binding parameter #",
                                     string (Int.toString (i + 1)),
                                     string ": %s\", sqlite3_errmsg(conn->conn));",
                                     newline]
                            end)

fun queryPrepared {loc, id, query, inputs, cols, doCols, nested} =
    box [string "uw_conn *conn = uw_get_db(ctx);",
         newline,
         p_pre_inputs inputs,
         if nested then
             box [string "sqlite3_stmt *stmt;",
                  newline]
         else
             box [string "sqlite3_stmt *stmt = conn->p",
                  string (Int.toString id),
                  string ";",
                  newline,
                  newline,

                  string "if (stmt == NULL) {",
                  newline],

         string "if (sqlite3_prepare_v2(conn->conn, \"",
         string (Prim.toCString query),
         string "\", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, \"Error preparing statement: ",
         string (Prim.toCString query),
         string "<br />%s\", sqlite3_errmsg(conn->conn));",
         newline,
         if nested then
             box [string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_finalize, stmt);",
                  newline]
         else
             box [string "conn->p",
                  string (Int.toString id),
                  string " = stmt;",
                  newline,
                  string "}",
                  newline,
                  newline,
                  string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);",
                  newline,
                  string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);",
                  newline],
         newline,

         p_inputs loc inputs,
         newline,

         queryCommon {loc = loc, cols = cols, doCols = doCols, query = box [string "\"",
                                                                            string (Prim.toCString query),
                                                                            string "\""]},

         string "uw_pop_cleanup(ctx);",
         newline,
         if nested then
             box []
         else
             box [string "uw_pop_cleanup(ctx);",
                  newline]]

fun dmlCommon {loc, dml, mode} =
    box [string "int r;",
         newline,

         string "if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {",
         newline,
         busyRetry,
         string "}",
         newline,
         newline,

         string "if (r != SQLITE_DONE) ",
         box [
             case mode of
                 Settings.Error => box [string "uw_error(ctx, FATAL, \"",
                                        string (ErrorMsg.spanToString loc),
                                        string ": DML step failed: %s<br />%s\", ",
                                        dml,
                                        string ", sqlite3_errmsg(conn->conn));"]
               | Settings.None => string "uw_set_error_message(ctx, sqlite3_errmsg(conn->conn));", (* tryDml *)
             newline
         ],
         string "else if (uw_dml_log_enabled()) {",
         newline,
         box [string "char *expanded = sqlite3_expanded_sql(stmt);",
              newline,
              string "uw_log_dml(ctx, \"",
              string (ErrorMsg.spanToString loc),
              string "\", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));",
              newline,
              string "sqlite3_free(expanded);",
              newline],
         string "}",
         newline]

fun dml (loc, mode) =
    box [string "uw_conn *conn = uw_get_db(ctx);",
         newline,
         string "sqlite3_stmt *stmt;",
         newline,
         newline,
         string "if (sqlite3_prepare_v2(conn->conn, dml, -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, \"Error preparing statement: %s<br />%s\", dml, sqlite3_errmsg(conn->conn));",
         newline,
         newline,
         string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_finalize, stmt);",
         newline,
         newline,

         dmlCommon {loc = loc, dml = string "dml", mode = mode},

         string "uw_pop_cleanup(ctx);",
         newline]

fun dmlPrepared {loc, id, dml, inputs, mode = mode} =
    box [string "uw_conn *conn = uw_get_db(ctx);",
         newline,
         p_pre_inputs inputs,
         string "sqlite3_stmt *stmt = conn->p",
         string (Int.toString id),
         string ";",
         newline,
         newline,

         string "if (stmt == NULL) {",
         newline,
         box [string "if (sqlite3_prepare_v2(conn->conn, \"",
              string (Prim.toCString dml),
              string "\", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, \"Error preparing statement: ",
              string (Prim.toCString dml),
              string "<br />%s\", sqlite3_errmsg(conn->conn));",
              newline,
              string "conn->p",
              string (Int.toString id),
              string " = stmt;",
              newline],
         string "}",
         newline,

         string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);",
         newline,
         string "uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);",
         newline,

         p_inputs loc inputs,
         newline,

         dmlCommon {loc = loc, dml = box [string "\"",
                                          string (Prim.toCString dml),
                                          string "\""], mode = mode},

         string "uw_pop_cleanup(ctx);",
         newline,
         string "uw_pop_cleanup(ctx);",
         newline]

fun nextval {loc, seqE, seqName} =
    box [string "uw_conn *conn = uw_get_db(ctx);",
         newline,
         string "char *insert = ",
         case seqName of
             SOME s => string ("\"INSERT INTO " ^ s ^ " VALUES (NULL)\"")
           | NONE => box [string "uw_Basis_strcat(ctx, \"INSERT INTO \", uw_Basis_strcat(ctx, ",
                          seqE,
                          string ", \" VALUES (NULL)\"))"],
         string ";",
         newline,
         string "char *delete = ",
         case seqName of
             SOME s => string ("\"DELETE FROM " ^ s ^ "\"")
           | NONE => box [string "uw_Basis_strcat(ctx, \"DELETE FROM \", ",
                          seqE,
                          string ")"],
         string ";",
         newline,
         string "int r;",
         newline,
         newline,

         (* A busy database asks for the request to be run again, as it
          * does for a DML statement. *)
         string "if ((r = sqlite3_exec(conn->conn, insert, NULL, NULL, NULL)) == SQLITE_BUSY) {",
         newline,
         busyRetry,
         string "}",
         newline,
         string "if (r != SQLITE_OK) uw_error(ctx, FATAL, \"'nextval' INSERT failed: %s\", sqlite3_errmsg(conn->conn));",
         newline,
         string "n = sqlite3_last_insert_rowid(conn->conn);",
         newline,
         string "if ((r = sqlite3_exec(conn->conn, delete, NULL, NULL, NULL)) == SQLITE_BUSY) {",
         newline,
         busyRetry,
         string "}",
         newline,
         string "if (r != SQLITE_OK) uw_error(ctx, FATAL, \"'nextval' DELETE failed: %s\", sqlite3_errmsg(conn->conn));",
         newline,
         (* A sequence is a table here, and a number is drawn by adding a
          * row and deleting it again: both go to the log, as issued. *)
         string "if (uw_dml_log_enabled()) {",
         newline,
         box [string "uw_log_dml(ctx, \"nextval 1/2\", insert, -1);",
              newline,
              string "uw_log_dml(ctx, \"nextval 2/2\", delete, -1);",
              newline],
         string "}",
         newline]

fun nextvalPrepared _ = raise Fail "SQLite.nextvalPrepared called"
fun setval _ = raise Fail "SQLite.setval called"

fun sqlifyString s = "'" ^ String.translate (fn #"'" => "''"
                                              | #"\000" => ""
                                              | ch => str ch)
                                            s ^ "'"

fun p_cast (s, _) = s

fun p_blank _ = "?"

(* Every SQLite keyword, from https://sqlite.org/lang_keywords.html.  Some
 * of them can in fact be used unquoted as identifiers ("fallback" tokens),
 * but which ones is a parser detail, so all of them are quoted. *)
val keywords = [
    "ABORT", "ACTION", "ADD", "AFTER", "ALL", "ALTER", "ALWAYS", "ANALYZE",
    "AND", "AS", "ASC", "ATTACH", "AUTOINCREMENT", "BEFORE", "BEGIN", "BETWEEN",
    "BY", "CASCADE", "CASE", "CAST", "CHECK", "COLLATE", "COLUMN", "COMMIT",
    "CONFLICT", "CONSTRAINT", "CREATE", "CROSS", "CURRENT", "CURRENT_DATE", "CURRENT_TIME", "CURRENT_TIMESTAMP",
    "DATABASE", "DEFAULT", "DEFERRABLE", "DEFERRED", "DELETE", "DESC", "DETACH", "DISTINCT",
    "DO", "DROP", "EACH", "ELSE", "END", "ESCAPE", "EXCEPT", "EXCLUDE",
    "EXCLUSIVE", "EXISTS", "EXPLAIN", "FAIL", "FILTER", "FIRST", "FOLLOWING", "FOR",
    "FOREIGN", "FROM", "FULL", "GENERATED", "GLOB", "GROUP", "GROUPS", "HAVING",
    "IF", "IGNORE", "IMMEDIATE", "IN", "INDEX", "INDEXED", "INITIALLY", "INNER",
    "INSERT", "INSTEAD", "INTERSECT", "INTO", "IS", "ISNULL", "JOIN", "KEY",
    "LAST", "LEFT", "LIKE", "LIMIT", "MATCH", "MATERIALIZED", "NATURAL", "NO",
    "NOT", "NOTHING", "NOTNULL", "NULL", "NULLS", "OF", "OFFSET", "ON",
    "OR", "ORDER", "OTHERS", "OUTER", "OVER", "PARTITION", "PLAN", "PRAGMA",
    "PRECEDING", "PRIMARY", "QUERY", "RAISE", "RANGE", "RECURSIVE", "REFERENCES", "REGEXP",
    "REINDEX", "RELEASE", "RENAME", "REPLACE", "RESTRICT", "RETURNING", "RIGHT", "ROLLBACK",
    "ROW", "ROWS", "SAVEPOINT", "SELECT", "SET", "TABLE", "TEMP", "TEMPORARY",
    "THEN", "TIES", "TO", "TRANSACTION", "TRIGGER", "UNBOUNDED", "UNION", "UNIQUE",
    "UPDATE", "USING", "VACUUM", "VALUES", "VIEW", "VIRTUAL", "WHEN", "WHERE",
    "WINDOW", "WITH", "WITHOUT"
   ]

val () = addDbms {name = "sqlite",
                  randomFunction = "RANDOM",
                  header = Config.sqheader,
                  link = "-lsqlite3",
                  init = init,
                  p_sql_type = p_sql_type,
                  query = query,
                  queryPrepared = queryPrepared,
                  dml = dml,
                  dmlPrepared = dmlPrepared,
                  nextval = nextval,
                  nextvalPrepared = nextvalPrepared,
                  setval = setval,
                  sqlifyString = sqlifyString,
                  p_cast = p_cast,
                  p_blank = p_blank,
                  supportsDeleteAs = false,
                  supportsUpdateAs = false,
                  createSequence = fn s => "CREATE TABLE " ^ s ^ " (id INTEGER PRIMARY KEY AUTOINCREMENT)",
                  textKeysNeedLengths = false,
                  supportsNextval = false,
                  supportsNestedPrepared = false,
                  sqlPrefix = "PRAGMA foreign_keys = ON;\nPRAGMA journal_mode = WAL;\n\n",
                  supportsOctetLength = false,
                  trueString = "1",
                  falseString = "0",
                  onlyUnion = false,
                  nestedRelops = false,
                  windowFunctions = false,
                  requiresTimestampDefaults = false,
                  supportsIsDistinctFrom = true,
                  supportsSHA512 = NONE,
                  supportsSimilar = NONE,
                  keywords = keywords,
                  identifierQuote = "\""}

end
