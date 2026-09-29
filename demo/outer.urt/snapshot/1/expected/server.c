#include "include/urweb/config.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <time.h>
 #include <libpq-fe.h>
  #include "include/urweb/urweb.h"
 
 static void uw_setup_limits() {
  }
  
  void uw_global_custom() {
   uw_setup_limits();
   }
   static void uw_db_validate(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_outer_u'");
     
     if (res == NULL) {
     PQfinish(conn);
      uw_error(ctx, FATAL, "Out of memory allocating query result.");
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_outer_u'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_outer_u' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_u' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_link' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_c' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_d' AND data_type = 'double precision' AND is_nullable = 'YES'))");
     
     if (res == NULL) {
     PQfinish(conn);
      uw_error(ctx, FATAL, "Out of memory allocating query result.");
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_u' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_link' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_c' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_d' AND data_type = 'double precision' AND is_nullable = 'YES'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "4")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_outer_u' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_u' AND LOWER(column_name) LIKE 'uw_%'");
     
     if (res == NULL) {
     PQfinish(conn);
      uw_error(ctx, FATAL, "Out of memory allocating query result.");
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_u' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "4")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_outer_u' has extra columns.");
      }
     
     PQclear(res);
     
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_outer_t'");
      
      if (res == NULL) {
      PQfinish(conn);
       uw_error(ctx, FATAL, "Out of memory allocating query result.");
       }
      
      if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_outer_t'\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "1")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Table 'uw_outer_t' does not exist.");
       }
      
      PQclear(res);
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_b' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))");
      
      if (res == NULL) {
      PQfinish(conn);
       uw_error(ctx, FATAL, "Out of memory allocating query result.");
       }
      
      if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_b' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "2")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Table 'uw_outer_t' has the wrong column types.");
       }
      
      PQclear(res);
      
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_t' AND LOWER(column_name) LIKE 'uw_%'");
      
      if (res == NULL) {
      PQfinish(conn);
       uw_error(ctx, FATAL, "Out of memory allocating query result.");
       }
      
      if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_outer_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "2")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Table 'uw_outer_t' has extra columns.");
       }
      
      PQclear(res);
      }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "SELECT T_T.uw_B, T_T.uw_Id, T_U.uw_C, T_U.uw_D, T_U.uw_Id FROM uw_Outer_t AS T_T LEFT JOIN uw_Outer_u AS T_U ON (T_T.uw_Id = T_U.uw_Link)", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_B, T_T.uw_Id, T_U.uw_C, T_U.uw_D, T_U.uw_Id FROM uw_Outer_t AS T_T LEFT JOIN uw_Outer_u AS T_U ON (T_T.uw_Id = T_U.uw_Link)\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "INSERT INTO uw_Outer_t (uw_B, uw_Id) VALUES ($1::text, $2::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Outer_t (uw_B, uw_Id) VALUES ($1::text, $2::int8)\n%s", msg);
       }
      PQclear(res);
      }
    
    static void uw_client_init(void) {
    uw_sqlfmtInt = "%lld::int8%n";
     uw_sqlfmtFloat = "%.16g::float8%n";
     uw_Estrings = 1;
     uw_sql_type_annotations = 1;
     uw_sqlsuffixString = "::text";
     uw_sqlsuffixChar = "::char";
     uw_sqlsuffixBlob = "::bytea";
     uw_sqlfmtUint4 = "%u::int4%n";
     }
    
    static void uw_db_close(uw_context ctx) {
    PQfinish(uw_get_db(ctx));
    }
    
    static int uw_db_begin(uw_context ctx, int could_write) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res = PQexec(conn, could_write ? "BEGIN ISOLATION LEVEL SERIALIZABLE" : "BEGIN ISOLATION LEVEL SERIALIZABLE, READ ONLY");
    
    if (res == NULL) return 1;
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {PQclear(res);
                                                   return 1;
                                                   }
    PQclear(res);
    return 0;
    }
    
    static int uw_db_commit(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res = PQexec(conn, "COMMIT");
    
    if (res == NULL) return 1;
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40001")) {
                                                   
                                                    PQclear(res);
                                                    return -1;
                                                    }
                                                   if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40P01")) {
                                                   
                                                    PQclear(res);
                                                    return -1;
                                                    }
                                                   PQclear(res);
                                                   return 1;
                                                   }
    PQclear(res);
    return 0;
    }
    
    static int uw_db_rollback(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res = PQexec(conn, "ROLLBACK");
    
    if (res == NULL) return 1;
    
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {PQclear(res);
                                                   return 1;
                                                   }
    PQclear(res);
    return 0;
    }
    
    static void uw_db_init(uw_context ctx) {
    char *env_db_str = getenv("URWEB_PQ_CON");
    PGconn *conn = PQconnectdb(env_db_str == NULL ? "dbname=test" : env_db_str);
    if (conn == NULL) uw_error(ctx, FATAL, "libpq can't allocate a connection.");
    if (PQstatus(conn) != CONNECTION_OK) {
    char msg[1024];
     strncpy(msg, PQerrorMessage(conn), 1024);
     msg[1023] = 0;
     PQfinish(conn);
     uw_error(ctx, BOUNDED_RETRY, "Connection to Postgres server failed: %s", msg);
    }
    uw_set_db(ctx, conn);
    uw_db_validate(ctx);
    uw_db_prepare(ctx);
    }
 
 /* No global setup for LRU cache. */
  
 
  
  struct __uws_1 {
   uw_Basis_string __uwf_B;
    uw_Basis_int __uwf_Id;
     };
  
  struct __uws_2
   {
   uw_Basis_string __uwf_C;
    uw_Basis_float* __uwf_D;
     uw_Basis_int* __uwf_Id;
      };
  struct __uws_3 {
   struct __uws_1 __uwf_T;
    struct __uws_2 __uwf_U;
     };
  
  struct __uws_4
   {
   uw_Basis_string __uwf_C;
    uw_Basis_string __uwf_D;
     uw_Basis_string __uwf_Id;
      uw_Basis_string __uwf_Link;
       };
  struct __uws_5 {
   uw_Basis_string __uwf_B;
    uw_Basis_string __uwf_Id;
     };
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Outer_t keys uw_Id constraints   */
   
  
  /* SQL table uw_Outer_u keys uw_Id constraints
   Link : FOREIGN KEY (uw_Link) REFERENCES uw_Outer_t (uw_Id)  */
   
  
  static uw_unit
   __uwn_main_3(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<table>"), 0),
                                                      (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                                              uw_unit
                                                                              acc
                                                                              =
                                                                              0;
                                                                              int dummy = (uw_begin_region(ctx), 0);
                                                                              uw_ensure_transaction(ctx);
                                                                              
                                                                               
                                                                               PGconn *conn = uw_get_db(ctx);
                                                                               static const int paramFormats[] = {  };
                                                                               const int *paramLengths = paramFormats;
                                                                               const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
                                                                               
                                                                               
                                                                               PGresult *res = 
                                                                               PQexecPrepared(conn, "uw0", 0, paramValues, paramLengths, paramFormats, 0);
                                                                               
                                                                               int n, i;
                                                                               
                                                                               if (res == NULL) {
                                                                               
                                                                               uw_try_reconnecting_and_restarting(ctx);
                                                                               uw_error(ctx, FATAL, "Can't allocate query result; database server may be down.");
                                                                               }
                                                                               
                                                                               if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                                                                               if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40001")) {
                                                                               
                                                                               PQclear(res);
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Serialization failure");
                                                                               }
                                                                               if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40P01")) {
                                                                               
                                                                               PQclear(res);
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Deadlock detected");
                                                                               }
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Query failed:\n%s\n%s", 
                                                                               "SELECT T_T.uw_B, T_T.uw_Id, T_U.uw_C, T_U.uw_D, T_U.uw_Id FROM uw_Outer_t AS T_T LEFT JOIN uw_Outer_u AS T_U ON (T_T.uw_Id = T_U.uw_Link)", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               if (PQnfields(res) != 5) {
                                                                               int nf = PQnfields(res);
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Query returned %d columns instead of 5:\n%s\n%s", nf, 
                                                                               "SELECT T_T.uw_B, T_T.uw_Id, T_U.uw_C, T_U.uw_D, T_U.uw_Id FROM uw_Outer_t AS T_T LEFT JOIN uw_Outer_u AS T_U ON (T_T.uw_Id = T_U.uw_Link)", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               uw_end_region(ctx);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                                                               n = PQntuples(res);
                                                                               for (i = 0; i < n; ++i) {
                                                                               struct __uws_3 __uwr_r_2;
                                                                               uw_unit
                                                                               __uwr_acc_3
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_B
                                                                               =
                                                                               (PQgetisnull(res, i, 0) ? 
                                                                               ({uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               PQgetvalue(res, i, 0));
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               =
                                                                               (PQgetisnull(res, i, 1) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Unexpectedly NULL field #1");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 1)));
                                                                               
                                                                               __uwr_r_2.__uwf_U.__uwf_C
                                                                               =
                                                                               (PQgetisnull(res, i, 2) ? NULL : 
                                                                               (PQgetisnull(res, i, 2) ? 
                                                                               ({uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Unexpectedly NULL field #2");
                                                                               tmp;
                                                                               }) : 
                                                                               PQgetvalue(res, i, 2)));
                                                                               
                                                                               __uwr_r_2.__uwf_U.__uwf_D
                                                                               =
                                                                               (PQgetisnull(res, i, 3) ? NULL : 
                                                                               ({
                                                                               uw_Basis_float
                                                                               *tmp = uw_malloc(ctx, sizeof(uw_Basis_float));
                                                                               *tmp = 
                                                                               (PQgetisnull(res, i, 3) ? 
                                                                               ({uw_Basis_float
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Unexpectedly NULL field #3");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToFloat_error(ctx, 
                                                                               PQgetvalue(res, i, 3)));
                                                                               tmp;
                                                                               }));
                                                                               
                                                                               __uwr_r_2.__uwf_U.__uwf_Id
                                                                               =
                                                                               (PQgetisnull(res, i, 4) ? NULL : 
                                                                               ({
                                                                               uw_Basis_int
                                                                               *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
                                                                               *tmp = 
                                                                               (PQgetisnull(res, i, 4) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/outer.ur:19:10-19:17: Unexpectedly NULL field #4");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 4)));
                                                                               tmp;
                                                                               }));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               ((uw_write(ctx, 
                                                                               "<tr>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_B
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_Basis_int*
                                                                               disc
                                                                               =
                                                                               __uwr_r_2.__uwf_U.__uwf_Id;
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               ""
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_int
                                                                               __uwr_x_4
                                                                               =
                                                                               (*disc);
                                                                               uw_Basis_intToString(ctx,
                                                                               __uwr_x_4
                                                                               );
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               })
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_Basis_string
                                                                               disc
                                                                               =
                                                                               __uwr_r_2.__uwf_U.__uwf_C;
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               ""
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_string
                                                                               __uwr_x_4
                                                                               =
                                                                               disc;
                                                                               __uwr_x_4;
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               })
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_Basis_float*
                                                                               disc
                                                                               =
                                                                               __uwr_r_2.__uwf_U.__uwf_D;
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               ""
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_float
                                                                               __uwr_x_4
                                                                               =
                                                                               (*disc);
                                                                               uw_Basis_floatToString(ctx,
                                                                               __uwr_x_4);
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               })
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</td>\n</tr>"), 0)))))))))));
                                                                               }
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                              uw_end_region(ctx);
                                                                               acc;
                                                                              })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</table>\n<form method=\"post\" action=\"/Outer/addT\">Insert into t: <input size=\"5\" type=\"text\" name=\"Id\" /> <input size=\"5\" type=\"text\" name=\"B\" />\n<input type=\"submit\" /></form>\n<form method=\"post\" action=\"/Outer/addU\">\nInsert into u: <input size=\"5\" type=\"text\" name=\"Id\" /> <input size=\"5\" type=\"text\" name=\"Link\" /> <input size=\"5\" type=\"text\" name=\"C\" />\n<input size=\"5\" type=\"text\" name=\"D\" /> <input type=\"submit\" />\n</form>\n</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_addU_4(uw_context ctx, struct __uws_4 __uwr_x0_0, 
                      uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   char *dml = ({
                                                uw_Basis_string arg0 =
                                                 "INSERT INTO uw_Outer_u (uw_C, uw_D, uw_Id, uw_Link) VALUES (";
                                                 
                                                 uw_Basis_string arg1 =
                                                  uw_Basis_sqlifyString(ctx,
                                                   __uwr_x0_0.__uwf_C);
                                                  uw_Basis_string arg2 = ", ";
                                                   
                                                 uw_Basis_string arg3 =
                                                  ({
                                                   uw_Basis_float* disc =
                                                   ({
                                                    uw_Basis_string disc =
                                                    __uwr_x0_0.__uwf_D;
                                                    
                                                    !strcmp(disc, "") ? NULL
                                                      :
                                                     1 ?
                                                      ({uw_Basis_string
                                                         __uwr___2 = disc;
                                                         ({
                                                          uw_Basis_float *tmp =
                                                          uw_malloc(ctx, sizeof(
                                                          uw_Basis_float));
                                                          *tmp =
                                                          uw_Basis_stringToFloat_error(ctx,
                                                           __uwr_x0_0.__uwf_D);
                                                          tmp;
                                                          });
                                                       })
                                                       :
                                                      ({
                                                       uw_Basis_float*
                                                       tmp;
                                                       uw_error(ctx, FATAL, "$/basis.urs:547:20-547:36: pattern match failure");
                                                       tmp;
                                                       });
                                                    });
                                                   
                                                   disc == NULL ?
                                                    "NULL::float8"
                                                     :
                                                    disc != NULL && 1 ?
                                                     ({uw_Basis_float __uwr_y_2
                                                        = (*disc);
                                                        uw_Basis_sqlifyFloat(ctx,
                                                         __uwr_y_2);
                                                      })
                                                      :
                                                     ({
                                                      uw_Basis_string
                                                      tmp;
                                                      uw_error(ctx, FATAL, "demo/outer.ur:34:8-34:118: pattern match failure");
                                                      tmp;
                                                      });
                                                   });
                                                  uw_Basis_string arg4 = ", ";
                                                   
                                                 uw_Basis_string arg5 =
                                                  uw_Basis_sqlifyInt(ctx,
                                                   uw_Basis_stringToInt_error(ctx,
                                                    __uwr_x0_0.__uwf_Id));
                                                  uw_Basis_string arg6 = ", ";
                                                   
                                                 uw_Basis_string arg7 =
                                                  uw_Basis_sqlifyInt(ctx,
                                                   uw_Basis_stringToInt_error(ctx,
                                                    __uwr_x0_0.__uwf_Link));
                                                  uw_Basis_string arg8 = ")";
                                                   uw_Basis_mstrcat(ctx, arg0, 
                                                                          arg1, 
                                                                          arg2, 
                                                                          arg3, 
                                                                          arg4, 
                                                                          arg5, 
                                                                          arg6, 
                                                                          arg7, 
                                                                          arg8,
                                                                               NULL);
                                                });
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     PGresult *res;
                                     res = PQexecParams(conn, dml, 0, NULL, NULL, NULL, NULL, 0);
                                     
                                     if (res == NULL) {
                                                        uw_try_reconnecting_and_restarting(ctx);
                                                        uw_error(ctx, FATAL, "Can't allocate DML result; database server may be down.");
                                                        }
                                      
                                      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                                      if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40001")) {
                                       
                                        PQclear(res);
                                        uw_error(ctx, UNLIMITED_RETRY, "Serialization failure");
                                        }
                                       if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40P01")) {
                                       
                                        PQclear(res);
                                        uw_error(ctx, UNLIMITED_RETRY, "Deadlock detected");
                                        }
                                       PQclear(res);
                                        uw_error(ctx, FATAL, "demo/outer.ur:34:4-35:11: DML failed:\n%s\n%s", dml, PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ({
             uw_unit arg0 = 0;
              uw_unit arg1 = 0;
             __uwn_main_3(ctx, arg0, arg1);
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_addT_5(uw_context ctx, struct __uws_5 __uwr_x0_0, 
                      uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_string arg1 = __uwr_x0_0.__uwf_B;
                                    
                                    uw_Basis_int arg2 =
                                     uw_Basis_stringToInt_error(ctx,
                                      __uwr_x0_0.__uwf_Id);
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0, 0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
                                      paramValues[0] = arg1;
                                       
                                       paramValues[1] = uw_Basis_attrifyInt(ctx, 
                                                         arg2);
                                        
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw1", 2, paramValues, paramLengths, paramFormats, 0);
                                     
                                     if (res == NULL) {
                                                        uw_try_reconnecting_and_restarting(ctx);
                                                        uw_error(ctx, FATAL, "Can't allocate DML result; database server may be down.");
                                                        }
                                      
                                      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                                      if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40001")) {
                                       
                                        PQclear(res);
                                        uw_error(ctx, UNLIMITED_RETRY, "Serialization failure");
                                        }
                                       if (!strcmp_nullsafe(PQresultErrorField(res, PG_DIAG_SQLSTATE), "40P01")) {
                                       
                                        PQclear(res);
                                        uw_error(ctx, UNLIMITED_RETRY, "Deadlock detected");
                                        }
                                       PQclear(res);
                                        uw_error(ctx, FATAL, "demo/outer.ur:30:4-31:11: DML failed:\n%s\n%s", 
                                        "INSERT INTO uw_Outer_t (uw_B, uw_Id) VALUES ($1::text, $2::int8)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ({
             uw_unit arg0 = 0;
              uw_unit arg1 = 0;
             __uwn_main_3(ctx, arg0, arg1);
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_6(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit arg0 = __uwr_x0_0;
            uw_unit arg1 = 0;
           __uwn_main_3(ctx, arg0, arg1);
           }));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'B':
   return 0;
   case 'C':
    return 0;
    case 'D':
     return 1;
     case 'I':
      return 2;
      case 'L':
       return 3;
       default:
  return -1;
  }}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   return 0;
   }
  
 static int uw_check_mime(const char *s) {
  return 0;
   }
  
 static int uw_check_requestHeader(const char *s) {
  return 0;
   }
  
 static int uw_check_responseHeader(const char *s) {
  return 0;
   }
  
 static int uw_check_envVar(const char *s) {
  return 0;
   }
  
 static int uw_check_meta(const char *s) {
  return 0;
   }
  
 extern void uw_sign(const char *in, char *out);
 extern int uw_hash_blocksize;
 static uw_Basis_string uw_cookie_sig(uw_context ctx) {
 uw_Basis_string r = uw_malloc(ctx, uw_hash_blocksize);
  uw_sign("", r);
  return uw_Basis_makeSigString(ctx, r);
  }
 
 static uw_served_file uw_served_files[] = {
 {NULL, NULL, 0, NULL}};
 
 static void uw_handle(uw_context ctx, char *request) {
 uw_Basis_string ims = uw_Basis_requestHeader(ctx, "If-modified-since");
 if (ims && !strcmp(ims, "Thu, 01 Jan 1970 00:00:00 GMT")) {
 uw_clear_headers(ctx);
  uw_write_header(ctx, uw_supports_direct_status ? "HTTP/1.1 304 Not Modified\r\n" : "Status: 304 Not Modified\r\n");
  return;
  }
 
 
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Outer/main", 11) && (request[11] == 0 || request[11] == '/')) {
  request += 11;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 1);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_6(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Outer/addT", 11) && (request[11] == 0 || request[11] == '/')) {
   request += 11;
   if (*request == '/') ++request;
   uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
    uw_write(ctx, uw_begin_html5);
    uw_mayReturnIndirectly(ctx);
    uw_set_script_header(ctx, "");
    uw_set_could_write_db(ctx, 1);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_Basis_string uw_input_B;
     uw_Basis_string uw_input_Id;
      
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input B");
      uw_input_B = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 2);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input Id");
       uw_input_Id = uw_Basis_unurlifyString_fromClient(ctx, &request);
       struct __uws_5 uw_inputs = {
        uw_input_B,
         uw_input_Id,
          };
     __uwn_wrap_addT_5(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Outer/addU", 11) && (request[11] == 0 || request[11] == '/')) {
   request += 11;
   if (*request == '/') ++request;
   uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
    uw_write(ctx, uw_begin_html5);
    uw_mayReturnIndirectly(ctx);
    uw_set_script_header(ctx, "");
    uw_set_could_write_db(ctx, 1);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_Basis_string uw_input_C;
     uw_Basis_string uw_input_D;
      uw_Basis_string uw_input_Id;
       uw_Basis_string uw_input_Link;
        
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input C");
      uw_input_C = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input D");
       uw_input_D = uw_Basis_unurlifyString_fromClient(ctx, &request);
       request = uw_get_input(ctx, 2);
        if (request == NULL)
        uw_error(ctx, FATAL, "Missing input Id");
        uw_input_Id = uw_Basis_unurlifyString_fromClient(ctx, &request);
        request = uw_get_input(ctx, 3);
         if (request == NULL)
         uw_error(ctx, FATAL, "Missing input Link");
         uw_input_Link = uw_Basis_unurlifyString_fromClient(ctx, &request);
         struct __uws_4 uw_inputs
          = {
          uw_input_C,
           uw_input_D,
            uw_input_Id,
             uw_input_Link,
              };
     __uwn_wrap_addU_4(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
 uw_clear_headers(ctx);
 uw_write_header(ctx, uw_supports_direct_status ? "HTTP/1.1 404 Not Found\r\n" : "Status: 404 Not Found\r\n");
 uw_write_header(ctx, "Content-type: text/plain\r\n");
 uw_write(ctx, "Not Found");
 }
 
 static void uw_expunger(uw_context ctx, uw_Basis_client cli) {
  __uwn_expunger_2(ctx, cli);
   }
 static void uw_initializer(uw_context ctx) {
 uw_begin_initializing(ctx);
  uw_end_initializing(ctx);
  __uwn_initializer_1(ctx, 0);
   }
 uw_app uw_application = {4,
                            60,
                               "/",
                                   uw_client_init,
                                                  uw_initializer,
                                                                 uw_expunger,
                                                                             
                           uw_db_init,
                                      uw_db_begin,
                                                  uw_db_commit,
                                                               uw_db_rollback,
                                                                              
                           uw_db_close,
                                       uw_handle,
                                                 uw_input_num,
                                                              uw_cookie_sig,
                                                                            
                           uw_check_url,
                                        uw_check_mime,
                                                      uw_check_requestHeader,
                                                                             
                           uw_check_responseHeader,
                                                   uw_check_envVar,
                                                                   
                           uw_check_meta,
                                         NULL,
                                              my_periodics,
                                                           "%c",
                                                                1,
                                                                  NULL,
                                                                       
                           uw_served_files,
                                           0};
 