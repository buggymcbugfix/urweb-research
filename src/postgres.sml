(* Copyright (c) 2008-2010, Adam Chlipala
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

structure Postgres :> POSTGRES = struct

open Settings
open Print.PD
open Print

val ident = String.translate (fn #"'" => "PRIME"
                               | ch => str ch)

fun p_sql_type t =
    case t of
        Int => "int8"
      | Float => "float8"
      | String => "text"
      | Char => "char"
      | Bool => "bool"
      | Time => "timestamp"
      | Blob => "bytea"
      | Channel => "int8"
      | Client => "int4"
      | Nullable t => p_sql_type t

fun p_sql_type_base t =
    case t of
        Int => "bigint"
      | Float => "double precision"
      | String => "text"
      | Char => "character"
      | Bool => "boolean"
      | Time => "timestamp without time zone"
      | Blob => "bytea"
      | Channel => "bigint"
      | Client => "integer"
      | Nullable t => p_sql_type_base t

(* One query of the schema check: run q, which gives one row holding a
 * count, and report wrong unless the count is n.  What is wrong, the
 * failure of the query included, becomes the context's error message, and
 * the check returns 1. *)
fun checkCount (q, n, wrong) =
    box [string "res = PQexec(conn, \"",
         string q,
         string "\");",
         newline,
         newline,
         string "if (res == NULL) {",
         newline,
         box [string "uw_set_error_message(ctx, \"Out of memory allocating query result.\");",
              newline,
              string "return 1;",
              newline],
         string "}",
         newline,
         newline,
         string "if (PQresultStatus(res) != PGRES_TUPLES_OK) {",
         newline,
         box [string "uw_set_error_message(ctx, \"Query failed:\\n%s\\n%s\", \"",
              string q,
              string "\", PQerrorMessage(conn));",
              newline,
              string "PQclear(res);",
              newline,
              string "return 1;",
              newline],
         string "}",
         newline,
         newline,
         string "if (strcmp(PQgetvalue(res, 0, 0), \"",
         string n,
         string "\")) {",
         newline,
         box [string "PQclear(res);",
              newline,
              string "uw_set_error_message(ctx, \"%s\", \"",
              string wrong,
              string "\");",
              newline,
              string "return 1;",
              newline],
         string "}",
         newline,
         newline,
         string "PQclear(res);",
         newline]

fun checkRel (table, checkNullable) (s, xts) =
    let
        (* The catalog stores the bare name, without the quotes an identifier
         * may carry in generated SQL. *)
        val sl = CharVector.map Char.toLower (Settings.unquoteSql s)

        val q = "SELECT COUNT(*) FROM information_schema." ^ table ^ " WHERE table_name = '"
                ^ sl ^ "'"

        val q' = String.concat ["SELECT COUNT(*) FROM information_schema.columns WHERE table_name = '",
                                sl,
                                "' AND (",
                                case String.concatWith " OR "
                                                       (map (fn (x, t) =>
                                                                String.concat ["(LOWER(column_name) = '",
                                                                               Settings.mangleSqlCatalog
                                                                                   (CharVector.map
                                                                                        Char.toLower (ident x)),
                                                                               (case p_sql_type_base t of
                                                                                    "bigint" =>
                                                                                    "' AND data_type IN ('bigint', 'numeric', 'integer')"
                                                                                  | "text" =>
                                                                                    "' AND data_type IN ('text', 'character varying')"                                                                          
                                                                                  | t =>
                                                                                    String.concat ["' AND data_type = '",
                                                                                                   t,
                                                                                                   "'"]),
                                                                               if checkNullable then
                                                                                   (" AND is_nullable = '"
                                                                                    ^ (if isNotNull t then
                                                                                           "NO"
                                                                                       else
                                                                                           "YES")
                                                                                    ^ "'")
                                                                               else
                                                                                   "",
                                                                               ")"]) xts) of
                                    "" => "FALSE"
                                  | s => s,
                                ")"]

        val q'' = String.concat ["SELECT COUNT(*) FROM information_schema.columns WHERE table_name = '",
                                 sl,
                                 "' AND LOWER(column_name) LIKE '", Settings.mangleSqlCatalog "%'"]
    in
        box [checkCount (q, "1", "Table '" ^ sl ^ "' does not exist."),
             checkCount (q', Int.toString (length xts), "Table '" ^ sl ^ "' has the wrong column types."),
             checkCount (q'', Int.toString (length xts), "Table '" ^ sl ^ "' has extra columns.")]
    end

fun init {dbstring, prepared = ss, tables, views, sequences} =
    box ([(* The schema check, for Basis.validateDb and for the start-up
          * (uw_db_validate): 0, or 1 with what is wrong as the context's
          * error message. *)
         string "static int uw_db_check(uw_context ctx) {",
         newline,
         string "PGconn *conn = uw_get_db(ctx);",
         newline,
         string "PGresult *res;",
         newline,
         newline,
         p_list_sep newline (checkRel ("tables", true)) tables,
         p_list_sep newline (checkRel ("views", false)) views,
         p_list_sep newline
                    (fn s =>
                        let
                            val sl = CharVector.map Char.toLower s
                        in
                            checkCount ("SELECT COUNT(*) FROM pg_class WHERE relname = '"
                                        ^ sl ^ "' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)",
                                        "1",
                                        "Sequence '" ^ s ^ "' does not exist.")
                        end) sequences,
         newline,
         string "return 0;",
         newline,
         string "}",
         newline,
         newline,

         if #persistent (currentProtocol ()) then
             (* A check on every connection would cost three
              * information_schema queries a table, on every reconnection
              * during an outage too. *)
             box (DbInit.validateOnce "uw_db_check(ctx)"
                  @ [string "static int uw_db_prepare(uw_context ctx) {",
                  newline,
                  string "PGconn *conn = uw_get_db(ctx);",
                  newline,
                  string "PGresult *res;",
                  newline,
                  newline,

                  p_list_sepi newline (fn i => fn (s, _) =>
                                                  box [string "res = PQprepare(conn, \"uw",
                                                       string (Int.toString i),
                                                       string "\", \"",
                                                       string (Prim.toCString s),
                                                       string "\", 0, NULL);",
                                                       newline,
                                                       string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
                                                       newline,
                                                       box [string "uw_set_error_message(ctx, \"Unable to create prepared statement:\\n%s\\n%s\", \"",
                                                            string (Prim.toCString s),
                                                            string "\", PQerrorMessage(conn));",
                                                            newline,
                                                            string "PQclear(res);",
                                                            newline,
                                                            string "return 1;",
                                                            newline],
                                                       string "}",
                                                       newline,
                                                       string "PQclear(res);",
                                                       newline])
                              ss,

                  newline,
                  string "return 0;",
                  newline,
                  string "}",
                  newline,
                  newline])
         else
             box [string "static int uw_db_validate(uw_context ctx) { return 0; }",
                  newline,
                  string "static int uw_db_prepare(uw_context ctx) { return 0; }"],

         string "static void uw_client_init(void) {",
         newline,
         box [string "uw_sqlfmtInt = \"%lld::int8%n\";",
              newline,
              string "uw_sqlfmtFloat = \"%.16g::float8%n\";",
              newline,
              string "uw_Estrings = 1;",
              newline,
              string "uw_sql_type_annotations = 1;",
              newline,
              string "uw_sqlsuffixString = \"::text\";",
              newline,
              string "uw_sqlsuffixChar = \"::char\";",
              newline,
              string "uw_sqlsuffixBlob = \"::bytea\";",
              newline,
              string "uw_sqlfmtUint4 = \"%u::int4%n\";",
              newline],
         string "}",
         newline,
         newline,

         string "static void uw_db_close(uw_context ctx) {",
         newline,
         string "PQfinish(uw_get_db(ctx));",
         newline,
         string "}",
         newline,
         newline,

         string "static int uw_db_begin(uw_context ctx, int could_write) {",
         newline,
         string "PGconn *conn = uw_get_db(ctx);",
         newline,
         string "PGresult *res = PQexec(conn, could_write ? \"BEGIN ISOLATION LEVEL SERIALIZABLE\" : \"BEGIN ISOLATION LEVEL SERIALIZABLE, READ ONLY\");",
         newline,
         newline,
         string "if (res == NULL) return 1;",
         newline,
         newline,
         string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
         box [string "PQclear(res);",
              newline,
              string "return 1;",
              newline],
         string "}",
         newline,
         string "PQclear(res);",
         newline,
         string "return 0;",
         newline,
         string "}",
         newline,
         newline,

         string "static int uw_db_commit(uw_context ctx) {",
         newline,
         string "PGconn *conn = uw_get_db(ctx);",
         newline,
         string "PGresult *res = PQexec(conn, \"COMMIT\");",
         newline,
         newline,
         string "if (res == NULL) return 1;",
         newline,
         newline,
         string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
         box [string "if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), \"40001\")) {",
              box [newline,
                   string "PQclear(res);",
                   newline,
                   string "return -1;",
                   newline],
              string "}",
              newline,
              string "if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), \"40P01\")) {",
              box [newline,
                   string "PQclear(res);",
                   newline,
                   string "return -1;",
                   newline],
              string "}",
              newline,
	      string "PQclear(res);",
              newline,
              string "return 1;",
              newline],
         string "}",
         newline,
         string "PQclear(res);",
         newline,
         string "return 0;",
         newline,
         string "}",
         newline,
         newline,

         string "static int uw_db_rollback(uw_context ctx) {",
         newline,
         string "PGconn *conn = uw_get_db(ctx);",
         newline,
         string "PGresult *res = PQexec(conn, \"ROLLBACK\");",
         newline,
         newline,
         string "if (res == NULL) return 1;",
         newline,
         newline,
         string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
         box [string "PQclear(res);",
              newline,
              string "return 1;",
              newline],
         string "}",
         newline,
         string "PQclear(res);",
         newline,
         string "return 0;",
         newline,
         string "}",

         newline,
         newline,

         string "static void uw_db_init(uw_context ctx) {",
         newline,
         string "char *env_db_str = getenv(\"URWEB_PQ_CON\");",
	 newline,
         string "PGconn *conn = PQconnectdb(env_db_str == NULL ? \"",
         string (Prim.toCString dbstring),
         string "\" : env_db_str);",
         newline,
         string "if (conn == NULL) uw_error(ctx, FATAL, ",
         string "\"libpq can't allocate a connection.\");",
         newline,
         string "if (PQstatus(conn) != CONNECTION_OK) {",
         newline,
         box [string "char msg[1024];",
              newline,
              string "strncpy(msg, PQerrorMessage(conn), 1024);",
              newline,
              string "msg[1023] = 0;",
              newline,
              string "PQfinish(conn);",
              newline,
              string "uw_error(ctx, BOUNDED_RETRY, ",
              string "\"Connection to Postgres server failed: %s\", msg);"],
         newline,
         string "}",
         newline,
         string "uw_set_db(ctx, conn);",
         newline,
         newline]
         @ DbInit.checkAndPrepare [string "PQfinish(conn);",
                                   newline]
         @ [string "}"])

fun p_getcol {loc, wontLeakStrings, col = i, typ = t} =
    let
        fun p_unsql t e eLen =
            case t of
                Int => box [string "uw_Basis_stringToInt_error(ctx, ", e, string ")"]
              | Float => box [string "uw_Basis_stringToFloat_error(ctx, ", e, string ")"]
              | String =>
                if wontLeakStrings then
                    e
                else
                    box [string "uw_strdup(ctx, ", e, string ")"]
              | Char => box [string "uw_Basis_unsqlChar(ctx, ", e, string ")"]
              | Bool => box [string "uw_Basis_stringToBool_error(ctx, ", e, string ")"]
              | Time => box [string "uw_Basis_unsqlTime(ctx, ", e, string ")"]
              | Blob => box [string "uw_Basis_stringToBlob_error(ctx, ",
                             e,
                             string ", ",
                             eLen,
                             string ")"]
              | Channel => box [string "uw_Basis_stringToChannel_error(ctx, ", e, string ")"]
              | Client => box [string "uw_Basis_stringToClient_error(ctx, ", e, string ")"]

              | Nullable _ => raise Fail "Postgres: Recursive Nullable"

        fun getter t =
            case t of
                Nullable t =>
                box [string "(PQgetisnull(res, i, ",
                     string (Int.toString i),
                     string ") ? NULL : ",
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
                box [string "(PQgetisnull(res, i, ",
                     string (Int.toString i),
                     string ") ? ",
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
                     p_unsql t
                             (box [string "PQgetvalue(res, i, ",
                                   string (Int.toString i),
                                   string ")"])
                             (box [string "PQgetlength(res, i, ",
                                   string (Int.toString i),
                                   string ")"]),
                     string ")"]
    in
        getter t
    end

(* We turn 0-output queries into 1-output queries to satisfy SQL.
 * This function adjusts our length expectations. *)
fun bumpedLength ls =
    case ls of
        [] => 1
      | _ => length ls

fun queryCommon {loc, query, cols, doCols} =
    box [string "int n, i;",
         newline,
         newline,

         string "if (res == NULL) {",
         box [newline,
              string "uw_try_reconnecting_and_restarting(ctx);",
              newline,
              string "uw_error(ctx, FATAL, \"Can't allocate query result; database server may be down.\");",
              newline],
         string "}",
         newline,
         newline,

         string "if (PQresultStatus(res) != PGRES_TUPLES_OK) {",
         newline,
         box [string "if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), \"40001\")) {",
              box [newline,
                   string "PQclear(res);",
                   newline,
                   string "uw_error(ctx, UNLIMITED_RETRY, \"Serialization failure\");",
                   newline],
              string "}",
              newline,
              string "if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), \"40P01\")) {",
              box [newline,
                   string "PQclear(res);",
                   newline,
                   string "uw_error(ctx, UNLIMITED_RETRY, \"Deadlock detected\");",
                   newline],
              string "}",
              newline,
              string "PQclear(res);",
              newline,
              string "uw_error(ctx, FATAL, \"",
              string (ErrorMsg.spanToString loc),
              string ": Query failed:\\n%s\\n%s\", ",
              query,
              string ", PQerrorMessage(conn));",
              newline],
         string "}",
         newline,
         newline,

         string "if (PQnfields(res) != ",
         string (Int.toString (bumpedLength cols)),
         string ") {",
         newline,
         box [string "int nf = PQnfields(res);",
              newline,
              string "PQclear(res);",
              newline,
              string "uw_error(ctx, FATAL, \"",
              string (ErrorMsg.spanToString loc),
              string ": Query returned %d columns instead of ",
              string (Int.toString (bumpedLength cols)),
              string ":\\n%s\\n%s\", nf, ",
              query,
              string ", PQerrorMessage(conn));",
              newline],
         string "}",
         newline,
         newline,

         string "uw_end_region(ctx);",
         newline,
         string "uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);",
         newline,
         string "n = PQntuples(res);",
         newline,
         string "for (i = 0; i < n; ++i) {",
         newline,
         doCols p_getcol,
         string "}",
         newline,
         newline,
         string "uw_pop_cleanup(ctx);",
         newline]

fun query {loc, cols, doCols} =
    box [string "PGconn *conn = uw_get_db(ctx);",
         newline,
         string "PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);",
         newline,
         newline,
         queryCommon {loc = loc, cols = cols, doCols = doCols, query = string "query"}]

fun p_ensql t e =
    case t of
        Int => box [string "uw_Basis_attrifyInt(ctx, ", e, string ")"]
      | Float => box [string "uw_Basis_attrifyFloat(ctx, ", e, string ")"]
      | String => e
      | Char => box [string "uw_Basis_attrifyChar(ctx, ", e, string ")"]
      | Bool => box [string "(", e, string " ? \"TRUE\" : \"FALSE\")"]
      | Time => box [string "uw_Basis_ensqlTime(ctx, ", e, string ")"]
      | Blob => box [e, string ".data"]
      | Channel => box [string "uw_Basis_attrifyChannel(ctx, ", e, string ")"]
      | Client => box [string "uw_Basis_attrifyClient(ctx, ", e, string ")"]
      | Nullable String => e
      | Nullable t => box [string "(",
                           e,
                           string " == NULL ? NULL : ",
                           p_ensql t (box [string "(*", e, string ")"]),
                           string ")"]

fun makeParams inputs =
    box [string "static const int paramFormats[] = { ",
         p_list_sep (box [string ",", space])
                    (fn t => if isBlob t then string "1" else string "0") inputs,
         string " };",
         newline,
         if List.exists isBlob inputs then
             box [string "int *paramLengths = uw_malloc(ctx, ",
                  string (Int.toString (length inputs)),
                  string " * sizeof(int));",
                  newline,
                  p_list_sepi (box [])
                              (fn i => fn t =>
                                          box [string "paramLengths[",
                                               string (Int.toString i),
                                               string "] = ",
                                               case t of
                                                   Blob => string ("arg" ^ Int.toString (i + 1) ^ ".size")
                                                 | Nullable Blob => string ("arg" ^ Int.toString (i + 1)
                                                                            ^ "?arg" ^ Int.toString (i + 1) ^ "->size:0")
                                                 | _ => string "0",
                                               string ";",
                                               newline]) inputs,
                  newline]
         else
             box [string "const int *paramLengths = paramFormats;",
                  newline],

         string "const char **paramValues = uw_malloc(ctx, ",
         string (Int.toString (length inputs)),
         string " * sizeof(char*));",
         newline,
         p_list_sepi (box [])
                     (fn i => fn t => box [string "paramValues[",
                                           string (Int.toString i),
                                           string "] = ",
                                           p_ensql t (box [string "arg",
                                                           string (Int.toString (i + 1))]),
                                           string ";",
                                           newline])
                     inputs,
         newline]

fun queryPrepared {loc, id, query, inputs, cols, doCols, nested = _} =
    box [string "PGconn *conn = uw_get_db(ctx);",
         newline,

         makeParams inputs,

         newline,
         string "PGresult *res = ",
         if #persistent (Settings.currentProtocol ()) then
             box [string "PQexecPrepared(conn, \"uw",
                  string (Int.toString id),
                  string "\", ",
                  string (Int.toString (length inputs)),
                  string ", paramValues, paramLengths, paramFormats, 0);"]
         else
             box [string "PQexecParams(conn, \"",
                  string (Prim.toCString query),
                  string "\", ",
                  string (Int.toString (length inputs)),
                  string ", NULL, paramValues, paramLengths, paramFormats, 0);"],
         newline,
         newline,
         queryCommon {loc = loc, cols = cols, doCols = doCols, query = box [string "\"",
                                                                            string (Prim.toCString query),
                                                                            string "\""]}]

fun dmlCommon {loc, dml, mode} =
    box [string "if (res == NULL) {",
         box [newline,
              string "uw_try_reconnecting_and_restarting(ctx);",
              newline,
              string "uw_error(ctx, FATAL, \"Can't allocate DML result; database server may be down.\");",
              newline],
         string "}",
         newline,
         newline,

         string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
         newline,
         box [string "if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), \"40001\")) {",
              box [newline,
                   string "PQclear(res);",
                   newline,
                   string "uw_error(ctx, UNLIMITED_RETRY, \"Serialization failure\");",
                   newline],
              string "}",
              newline,
              string "if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), \"40P01\")) {",
              box [newline,
                   string "PQclear(res);",
                   newline,
                   string "uw_error(ctx, UNLIMITED_RETRY, \"Deadlock detected\");",
                   newline],
              string "}",
              newline,
              case mode of
                  Settings.Error => box [string "PQclear(res);",
                                         newline,
                                         string "uw_error(ctx, FATAL, \"",
                                         string (ErrorMsg.spanToString loc),
                                         string ": DML failed:\\n%s\\n%s\", ",
                                         dml,
                                         string ", PQerrorMessage(conn));"]
                | Settings.None => box [string "uw_set_error_message(ctx, PQerrorMessage(conn));",
                                        newline,
                                        newline,

                                        string "res = PQexec(conn, \"ROLLBACK TO s\");",
                                        newline,
                                        string "if (res == NULL) {",
                                        box [newline,
                                             string "uw_try_reconnecting_and_restarting(ctx);",
                                             newline,
                                             string "uw_error(ctx, FATAL, \"Can't allocate DML ROLLBACK result; database server may be down.\");",
                                             newline],
                                        string "}",
                                        newline,
                                        newline,

                                        string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
                                        newline,
                                        box [string "PQclear(res);",
                                             newline,
                                             string "uw_error(ctx, FATAL, \"",
                                             string (ErrorMsg.spanToString loc),
                                             string ": ROLLBACK TO failed:\\n%s\\n%s\", ",
                                             dml,
                                             string ", PQerrorMessage(conn));",
                                             newline,
                                             string "}"],
                                        newline,

                                        string "PQclear(res);",
                                        newline],
              newline],
         string "}",

         case mode of
             Error => box [newline,
                           newline,
                           string "PQclear(res);",
                           newline]
           | None => box[string " else {",
                         newline,
                         box [string "PQclear(res);",
                              newline,
                              string "res = PQexec(conn, \"RELEASE s\");",
                              newline,
                              string "if (res == NULL) {",
                              box [newline,
                                   string "uw_try_reconnecting_and_restarting(ctx);",
                                   newline,
                                   string "uw_error(ctx, FATAL, \"Can't allocate DML RELEASE result; database server may be down.\");",
                                   newline],
                              string "}",
                              newline,
                              newline,

                              string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
                              newline,
                              box [string "PQclear(res);",
                                   newline,
                                   string "uw_error(ctx, FATAL, \"",
                                   string (ErrorMsg.spanToString loc),
                                   string ": RELEASE failed:\\n%s\\n%s\", ",
                                   dml,
                                   string ", PQerrorMessage(conn));",
                                   newline],
                              string "}",
                              newline,
                              string "PQclear(res);",
                              newline],
                         string "}",
                         newline]]

fun makeSavepoint mode =
    case mode of
        Error => box []
      | None => box [string "res = PQexec(conn, \"SAVEPOINT s\");",
                     newline,
                     string "if (res == NULL) {",
                     box [newline,
                          string "uw_try_reconnecting_and_restarting(ctx);",
                          newline,
                          string "uw_error(ctx, FATAL, \"Can't allocate DML SAVEPOINT result; database server may be down.\");",
                          newline],
                     string "}",
                     newline,
                     newline,
                     string "if (PQresultStatus(res) != PGRES_COMMAND_OK) {",
                     box [newline,
                          string "PQclear(res);",
                          newline,
                          string "uw_error(ctx, FATAL, \"Error creating SAVEPOINT\");",
                          newline],
                     string "}",
                     newline,
                     string "PQclear(res);",
                     newline,
                     newline]

fun dml (loc, mode) =
    box [string "PGconn *conn = uw_get_db(ctx);",
         newline,
         string "PGresult *res;",
         newline,

         makeSavepoint mode,

         string "res = PQexecParams(conn, dml, 0, NULL, NULL, NULL, NULL, 0);",
         newline,
         newline,
         dmlCommon {loc = loc, dml = string "dml", mode = mode}]

fun dmlPrepared {loc, id, dml, inputs, mode} =
    box [string "PGconn *conn = uw_get_db(ctx);",
         newline,

         makeParams inputs,

         newline,
         string "PGresult *res;",
         newline,
         newline,

         makeSavepoint mode,

         string "res = ",
         if #persistent (Settings.currentProtocol ()) then
             box [string "PQexecPrepared(conn, \"uw",
                  string (Int.toString id),
                  string "\", ",
                  string (Int.toString (length inputs)),
                  string ", paramValues, paramLengths, paramFormats, 0);"]
         else
             box [string "PQexecParams(conn, \"",
                  string (Prim.toCString dml),
                  string "\", ",
                  string (Int.toString (length inputs)),
                  string ", NULL, paramValues, paramLengths, paramFormats, 0);"],
         newline,
         newline,
         dmlCommon {loc = loc, dml = box [string "\"",
                                          string (Prim.toCString dml),
                                          string "\""], mode = mode}]

fun nextvalCommon {loc, query} =
    box [string "if (res == NULL) {",
         box [newline,
              string "uw_try_reconnecting_and_restarting(ctx);",
              newline,
              string "uw_error(ctx, FATAL, \"Can't allocate NEXTVAL result; database server may be down.\");",
              newline],
         string "}",
         newline,
         newline,

         string "if (PQresultStatus(res) != PGRES_TUPLES_OK) {",
         newline,
         box [string "PQclear(res);",
              newline,
              string "uw_error(ctx, FATAL, \"",
              string (ErrorMsg.spanToString loc),
              string ": Query failed:\\n%s\\n%s\", ",
              query,
              string ", PQerrorMessage(conn));",
              newline],
         string "}",
         newline,
         newline,

         string "n = PQntuples(res);",
         newline,
         string "if (n != 1) {",
         newline,
         box [string "PQclear(res);",
              newline,
              string "uw_error(ctx, FATAL, \"",
              string (ErrorMsg.spanToString loc),
              string ": Wrong number of result rows:\\n%s\\n%s\", ",
              query,
              string ", PQerrorMessage(conn));",
              newline],
         string "}",
         newline,
         newline,

         string "n = uw_Basis_stringToInt_error(ctx, PQgetvalue(res, 0, 0));",
         newline,
         string "PQclear(res);",
         newline]

open Cjr

fun nextval {loc, seqE, seqName} =
    let
        val query = case seqName of
                        SOME s =>
                        string ("\"SELECT NEXTVAL('" ^ s ^ "')\"")
                      | _ => box [string "uw_Basis_strcat(ctx, \"SELECT NEXTVAL('\", uw_Basis_strcat(ctx, ",
                                  seqE,
                                  string ", \"')\"))"]
    in
        box [string "char *query = ",
             query,
             string ";",
             newline,
             string "PGconn *conn = uw_get_db(ctx);",
             newline,
             string "PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);",
             newline,
             newline,
             nextvalCommon {loc = loc, query = string "query"}]
    end

fun nextvalPrepared {loc, id, query} =
    box [string "PGconn *conn = uw_get_db(ctx);",
         newline,
         newline,
         string "PGresult *res = ",
         if #persistent (Settings.currentProtocol ()) then
             box [string "PQexecPrepared(conn, \"uw",
                  string (Int.toString id),
                  string "\", 0, NULL, NULL, NULL, 0);"]
         else
             box [string "PQexecParams(conn, \"",
                  string (Prim.toCString query),
                  string "\", 0, NULL, NULL, NULL, NULL, 0);"],
         newline,
         newline,
         nextvalCommon {loc = loc, query = box [string "\"",
                                                string (Prim.toCString query),
                                                string "\""]}]

fun setvalCommon {loc, query} =
    box [string "if (res == NULL) {",
         box [newline,
              string "uw_try_reconnecting_and_restarting(ctx);",
              newline,
              string "uw_error(ctx, FATAL, \"Can't allocate SETVAL result; database server may be down.\");",
              newline],
         string "}",
         newline,
         newline,

         string "if (PQresultStatus(res) != PGRES_TUPLES_OK) {",
         newline,
         box [string "PQclear(res);",
              newline,
              string "uw_error(ctx, FATAL, \"",
              string (ErrorMsg.spanToString loc),
              string ": Query failed:\\n%s\\n%s\", ",
              query,
              string ", PQerrorMessage(conn));",
              newline],
         string "}",
         newline,
         newline,

         string "PQclear(res);",
         newline]

fun setval {loc, seqE, count} =
    let
        val query = box [string "uw_Basis_strcat(ctx, \"SELECT SETVAL('\", uw_Basis_strcat(ctx, ",
                         seqE,
                         string ", uw_Basis_strcat(ctx, \"', \", uw_Basis_strcat(ctx, uw_Basis_sqlifyInt(ctx, ",
                         count,
                         string "), \")\"))))"]
    in
        box [string "char *query = ",
             query,
             string ";",
             newline,
             string "PGconn *conn = uw_get_db(ctx);",
             newline,
             string "PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);",
             newline,
             newline,
             setvalCommon {loc = loc, query = string "query"}]
    end

fun sqlifyString s = "E'" ^ String.translate (fn #"'" => "\\'"
                                               | #"\\" => "\\\\"
                                               | ch =>
                                                 if Char.isPrint ch then
                                                     str ch
                                                 else
                                                     "\\" ^ StringCvt.padLeft #"0" 3
                                                                              (Int.fmt StringCvt.OCT (ord ch)))
                                             (Prim.toCString s) ^ "'::text"

fun p_cast (s, t) = s ^ "::" ^ p_sql_type t

fun p_blank (n, t) = p_cast ("$" ^ Int.toString n, t)

(* The PostgreSQL keywords that cannot be used as table or column names
 * without quoting: the "reserved" and "reserved (can be function or type)"
 * categories of Appendix C of the manual, taken from the RESERVED_KEYWORD and
 * TYPE_FUNC_NAME_KEYWORD entries of src/include/parser/kwlist.h. *)
val keywords = [
    "ALL", "ANALYSE", "ANALYZE", "AND", "ANY", "ARRAY", "AS", "ASC",
    "ASYMMETRIC", "AUTHORIZATION", "BINARY", "BOTH", "CASE", "CAST", "CHECK", "COLLATE",
    "COLLATION", "COLUMN", "CONCURRENTLY", "CONSTRAINT", "CREATE", "CROSS", "CURRENT_CATALOG", "CURRENT_DATE",
    "CURRENT_ROLE", "CURRENT_SCHEMA", "CURRENT_TIME", "CURRENT_TIMESTAMP", "CURRENT_USER", "DEFAULT", "DEFERRABLE", "DESC",
    "DISTINCT", "DO", "ELSE", "END", "EXCEPT", "FALSE", "FETCH", "FOR",
    "FOREIGN", "FREEZE", "FROM", "FULL", "GRANT", "GROUP", "HAVING", "ILIKE",
    "IN", "INITIALLY", "INNER", "INTERSECT", "INTO", "IS", "ISNULL", "JOIN",
    "LATERAL", "LEADING", "LEFT", "LIKE", "LIMIT", "LOCALTIME", "LOCALTIMESTAMP", "NATURAL",
    "NOT", "NOTNULL", "NULL", "OFFSET", "ON", "ONLY", "OR", "ORDER",
    "OUTER", "OVERLAPS", "PLACING", "PRIMARY", "REFERENCES", "RETURNING", "RIGHT", "SELECT",
    "SESSION_USER", "SIMILAR", "SOME", "SYMMETRIC", "SYSTEM_USER", "TABLE", "TABLESAMPLE", "THEN",
    "TO", "TRAILING", "TRUE", "UNION", "UNIQUE", "USER", "USING", "VARIADIC",
    "VERBOSE", "WHEN", "WHERE", "WINDOW", "WITH"
   ]

val () = addDbms {name = "postgres",
                  randomFunction = "RANDOM",
                  header = Config.pgheader,
                  link = "-lpq",
                  p_sql_type = p_sql_type,
                  init = init,
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
                  supportsDeleteAs = true,
                  supportsUpdateAs = true,
                  createSequence = fn s => "CREATE SEQUENCE " ^ s,
                  textKeysNeedLengths = false,
                  supportsNextval = true,
                  supportsNestedPrepared = true,
                  sqlPrefix = "",
                  supportsOctetLength = true,
                  trueString = "TRUE",
                  falseString = "FALSE",
                  onlyUnion = false,
                  nestedRelops = true,
                  windowFunctions = true,
                  requiresTimestampDefaults = false,
                  supportsIsDistinctFrom = true,
                  supportsSHA512 = SOME {InitializeDb = "CREATE EXTENSION IF NOT EXISTS pgcrypto;",
                                         GenerateHash = fn name => "DIGEST(" ^ name ^ ", 'sha512')"},
                  supportsSimilar = SOME {InitializeDb = "CREATE EXTENSION IF NOT EXISTS pg_trgm;"},
                  keywords = keywords,
                  identifierQuote = "\""}

val () = setDbms "postgres"

end
