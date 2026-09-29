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
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_sql_t'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_sql_t'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_sql_t' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_t' AND ((LOWER(column_name) = 'uw_a' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_b' AND data_type = 'double precision' AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_c' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_d' AND data_type = 'boolean' AND is_nullable = 'NO'))");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_t' AND ((LOWER(column_name) = 'uw_a' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_b' AND data_type = 'double precision' AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_c' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_d' AND data_type = 'boolean' AND is_nullable = 'NO'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "4")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_sql_t' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_t' AND LOWER(column_name) LIKE 'uw_%'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "4")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_sql_t' has extra columns.");
      }
     
     PQclear(res);
     }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "SELECT T_T.uw_A, T_T.uw_B, T_T.uw_C, T_T.uw_D FROM uw_Sql_t AS T_T", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_A, T_T.uw_B, T_T.uw_C, T_T.uw_D FROM uw_Sql_t AS T_T\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "DELETE FROM uw_Sql_t AS T_T WHERE (T_T.uw_A = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Sql_t AS T_T WHERE (T_T.uw_A = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw2", "INSERT INTO uw_Sql_t (uw_A, uw_B, uw_C, uw_D) VALUES ($1::int8, $2::float8, $3::text, $4::bool)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Sql_t (uw_A, uw_B, uw_C, uw_D) VALUES ($1::int8, $2::float8, $3::text, $4::bool)\n%s", msg);
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
  
 
  
  
  struct __uws_1
   {
   uw_Basis_int __uwf_A;
    uw_Basis_float __uwf_B;
     uw_Basis_string __uwf_C;
      uw_Basis_bool __uwf_D;
       };
  struct __uws_2 {
   struct __uws_1 __uwf_T;
    };
  
  struct __uws_3
   {
   uw_Basis_string __uwf_A;
    uw_Basis_string __uwf_B;
     uw_Basis_string __uwf_C;
      uw_Basis_bool __uwf_D;
       };
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Sql_t keys uw_A constraints   */
   
  
  static uw_unit
   __uwn_list_3(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "\n<table border=\"1\">\n<tr> <th>A</th> <th>B</th> <th>C</th> <th>D</th> </tr>\n"), 0),
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_unit acc =
                                   0;
                                   int dummy = (uw_begin_region(ctx), 0);
                                   uw_ensure_transaction(ctx);
                                   
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = {  };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
                                      
                                      
                                     PGresult *res = PQexecPrepared(conn, "uw0", 0, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                       uw_error(ctx, FATAL, "demo/sql.ur:12:39-12:43: Query failed:\n%s\n%s", 
                                       "SELECT T_T.uw_A, T_T.uw_B, T_T.uw_C, T_T.uw_D FROM uw_Sql_t AS T_T", PQerrorMessage(conn));
                                       }
                                      
                                      if (PQnfields(res) != 4) {
                                      int nf = PQnfields(res);
                                       PQclear(res);
                                       uw_error(ctx, FATAL, "demo/sql.ur:12:39-12:43: Query returned %d columns instead of 4:\n%s\n%s", nf, 
                                       "SELECT T_T.uw_A, T_T.uw_B, T_T.uw_C, T_T.uw_D FROM uw_Sql_t AS T_T", PQerrorMessage(conn));
                                       }
                                      
                                      uw_end_region(ctx);
                                      uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                      n = PQntuples(res);
                                      for (i = 0; i < n; ++i) {
                                      struct __uws_2 __uwr_r_2;
                                       uw_unit __uwr_acc_3 =
                                       acc;
                                       
                                       __uwr_r_2.__uwf_T.__uwf_A =
                                        (PQgetisnull(res, i, 0) ? ({uw_Basis_int
                                                                   tmp;
                                                                   uw_error(ctx, FATAL, "demo/sql.ur:12:39-12:43: Unexpectedly NULL field #0");
                                                                   tmp;
                                                                   }) : uw_Basis_stringToInt_error(ctx, 
                                                                         PQgetvalue(res, i, 0)));
                                        
                                        __uwr_r_2.__uwf_T.__uwf_B =
                                         (PQgetisnull(res, i, 1) ? ({uw_Basis_float
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "demo/sql.ur:12:39-12:43: Unexpectedly NULL field #1");
                                                                    tmp;
                                                                    }) : uw_Basis_stringToFloat_error(ctx, 
                                                                          PQgetvalue(res, i, 1)));
                                         
                                        __uwr_r_2.__uwf_T.__uwf_C =
                                         (PQgetisnull(res, i, 2) ? ({uw_Basis_string
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "demo/sql.ur:12:39-12:43: Unexpectedly NULL field #2");
                                                                    tmp;
                                                                    }) : PQgetvalue(res, i, 2));
                                         
                                        __uwr_r_2.__uwf_T.__uwf_D =
                                         (PQgetisnull(res, i, 3) ? ({uw_Basis_bool
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "demo/sql.ur:12:39-12:43: Unexpectedly NULL field #3");
                                                                    tmp;
                                                                    }) : uw_Basis_stringToBool_error(ctx, 
                                                                          PQgetvalue(res, i, 3)));
                                         
                                       
                                       acc =
                                       ((uw_write(ctx, "<tr>\n<td>"), 0),
                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                __uwr_r_2.__uwf_T.__uwf_A
                                                                ),
                                         uw_end_region(ctx), ((uw_write(ctx, "</td> <td>"), 0),
                                                              (uw_begin_region(ctx),
                                                                uw_Basis_htmlifyFloat_w(ctx,
                                                                 __uwr_r_2.__uwf_T.__uwf_B
                                                                 ),
                                                               uw_end_region(ctx),
                                                                ((uw_write(ctx, 
                                                                  "</td> <td>"), 0),
                                                                 (uw_begin_region(ctx),
                                                                   uw_Basis_htmlifyString_w(ctx,
                                                                    __uwr_r_2.__uwf_T.__uwf_C
                                                                    ),
                                                                  uw_end_region(ctx),
                                                                   ((uw_write(ctx, 
                                                                     "</td> <td>"), 0),
                                                                    (uw_begin_region(ctx),
                                                                      uw_Basis_htmlifyBool_w(ctx,
                                                                       __uwr_r_2.__uwf_T.__uwf_D
                                                                       ),
                                                                     uw_end_region(ctx),
                                                                      ((uw_write(ctx, 
                                                                        "</td>\n<td><form method=\"post\" action=\"/Sql/delete/"), 0),
                                                                       (uw_begin_region(ctx),
                                                                         uw_Basis_urlifyInt_w(ctx,
                                                                          __uwr_r_2.__uwf_T.__uwf_A
                                                                          ),
                                                                        uw_end_region(ctx),
                                                                         (uw_write(ctx, 
                                                                          "\"><input type=\"submit\" value=\"Delete\" /></form></td>\n</tr>"), 0)))))))))));
                                       }
                                      
                                      uw_pop_cleanup(ctx);
                                      
                                   uw_end_region(ctx);
                                    acc;
                                   })),
            uw_end_region(ctx), (uw_write(ctx, "\n</table>\n<br /><hr /><br />\n<form method=\"post\" action=\"/Sql/add\">\n<table>\n<tr> <th>A:</th> <td><input type=\"text\" name=\"A\" /></td> </tr>\n<tr> <th>B:</th> <td><input type=\"text\" name=\"B\" /></td> </tr>\n<tr> <th>C:</th> <td><input type=\"text\" name=\"C\" /></td> </tr>\n<tr> <th>D:</th> <td><input type=\"checkbox\" name=\"D\" /></td> </tr>\n<tr> <th></th> <td><input type=\"submit\" value=\"Add Row\" /></td> </tr>\n</table>\n</form>\n"), 0))));
   }
  
  static uw_unit
   __uwn_wrap_delete_4(uw_context ctx, uw_Basis_int __uwr_x1_0, 
                        uw_unit __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           uw_unit __uwr___3 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_int arg1 = __uwr_x1_0;
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                      paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                        arg1);
                                       
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw1", 1, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/sql.ur:40:4-49:2: DML failed:\n%s\n%s", 
                                        "DELETE FROM uw_Sql_t AS T_T WHERE (T_T.uw_A = $1::int8)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">\n<p>Row deleted.</p>\n"), 0),
                                                        (uw_begin_region(ctx), ({
                                                                               uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                               __uwn_list_3(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "\n</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_add_5(uw_context ctx, struct __uws_3 __uwr_x0_0, 
                     uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_int arg1 =
                                    uw_Basis_stringToInt_error(ctx,
                                     __uwr_x0_0.__uwf_A);
                                    
                                    uw_Basis_float arg2 =
                                     uw_Basis_stringToFloat_error(ctx,
                                      __uwr_x0_0.__uwf_B);
                                    uw_Basis_string arg3 = __uwr_x0_0.__uwf_C;
                                    uw_Basis_bool arg4 = __uwr_x0_0.__uwf_D;
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0, 0, 
                                                                          0, 0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 4 * sizeof(char*));
                                      paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                        arg1);
                                       
                                       paramValues[1] = uw_Basis_attrifyFloat(ctx, 
                                                         arg2);
                                        paramValues[2] = arg3;
                                         
                                       paramValues[3] = (arg4 ? "TRUE" : "FALSE");
                                        
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw2", 4, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/sql.ur:30:4-39:2: DML failed:\n%s\n%s", 
                                        "INSERT INTO uw_Sql_t (uw_A, uw_B, uw_C, uw_D) VALUES ($1::int8, $2::float8, $3::text, $4::bool)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">\n<p>Row added.</p>\n"), 0),
                                                        (uw_begin_region(ctx), ({
                                                                               uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                               __uwn_list_3(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "\n</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_6(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n"), 0),
                                                      (uw_begin_region(ctx), ({
                                                                              uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                              __uwn_list_3(ctx,
                                                                              arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                              }),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\n</body>"), 0)))))));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'A':
   return 0;
   case 'B':
    return 1;
    case 'C':
     return 2;
     case 'D':
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
 
 if (!strncmp(request, "/Sql/main", 9) && (request[9] == 0 || request[9] == '/')) {
  request += 9;
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
  
  if (!strncmp(request, "/Sql/delete", 11) && (request[11] == 0 || request[11] == '/')) {
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     
      uw_unit uw_inputs;
      __uwn_wrap_delete_4(ctx, arg0, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql/add", 8) && (request[8] == 0 || request[8] == '/')) {
   request += 8;
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
    uw_Basis_string uw_input_A;
     uw_Basis_string uw_input_B;
      uw_Basis_string uw_input_C;
       uw_Basis_bool uw_input_D;
        
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input A");
      uw_input_A = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input B");
       uw_input_B = uw_Basis_unurlifyString_fromClient(ctx, &request);
       request = uw_get_input(ctx, 2);
        if (request == NULL)
        uw_error(ctx, FATAL, "Missing input C");
        uw_input_C = uw_Basis_unurlifyString_fromClient(ctx, &request);
        request = uw_get_optional_input(ctx, 3);
         if (request == NULL)
         uw_error(ctx, FATAL, "Missing input D");
         uw_input_D = uw_Basis_unurlifyBool(ctx, &request);
         struct __uws_3 uw_inputs
          = {
          uw_input_A,
           uw_input_B,
            uw_input_C,
             uw_input_D,
              };
     __uwn_wrap_add_5(ctx, uw_inputs, 0);
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
 