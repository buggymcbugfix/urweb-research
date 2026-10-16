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
   static int uw_db_check(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_sql_char_t'");
     
     if (res == NULL) {
     uw_set_error_message(ctx, "Out of memory allocating query result.");
      return 1;
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_sql_char_t'", PQerrorMessage(conn));
      PQclear(res);
      return 1;
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      uw_set_error_message(ctx, "%s", "Table 'uw_sql_char_t' does not exist.");
      return 1;
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_char_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_c' AND data_type = 'character' AND is_nullable = 'NO'))");
      
      if (res == NULL) {
      uw_set_error_message(ctx, "Out of memory allocating query result.");
       return 1;
       }
      
      if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_char_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_c' AND data_type = 'character' AND is_nullable = 'NO'))", PQerrorMessage(conn));
       PQclear(res);
       return 1;
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "2")) {
      PQclear(res);
       uw_set_error_message(ctx, "%s", "Table 'uw_sql_char_t' has the wrong column types.");
       return 1;
       }
      
      PQclear(res);
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_char_t' AND LOWER(column_name) LIKE 'uw_%'");
       
       if (res == NULL) {
       uw_set_error_message(ctx, "Out of memory allocating query result.");
        return 1;
        }
       
       if (PQresultStatus(res) != PGRES_TUPLES_OK) {
       uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_sql_char_t' AND LOWER(column_name) LIKE 'uw_%'", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       
       if (strcmp(PQgetvalue(res, 0, 0), "2")) {
       PQclear(res);
        uw_set_error_message(ctx, "%s", "Table 'uw_sql_char_t' has extra columns.");
        return 1;
        }
       
       PQclear(res);
       
    return 0;
    }
    
    static int uw_db_validate(uw_context ctx) {
     static int validated;
     
     if (validated)
     return 0;
      if (uw_db_check(ctx))
     return 1;
      validated = 1;
     return 0;
     }
     
     static int uw_db_prepare(uw_context ctx) {
     PGconn *conn = uw_get_db(ctx);
     PGresult *res;
     
     res = PQprepare(conn, "uw0", "SELECT COUNT(*) AS uw_1 FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_C = E'\\\\351'::text)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      uw_set_error_message(ctx, "Unable to create prepared statement:\n%s\n%s", "SELECT COUNT(*) AS uw_1 FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_C = E'\\\\351'::text)", PQerrorMessage(conn));
       PQclear(res);
       return 1;
       }
      PQclear(res);
      
      
      res = PQprepare(conn, "uw1", "SELECT E'\\\\351'::text AS uw_1 FROM uw_Sql_char_t AS T_T", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       uw_set_error_message(ctx, "Unable to create prepared statement:\n%s\n%s", "SELECT E'\\\\351'::text AS uw_1 FROM uw_Sql_char_t AS T_T", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       PQclear(res);
       
      
      res = PQprepare(conn, "uw2", "DELETE FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       uw_set_error_message(ctx, "Unable to create prepared statement:\n%s\n%s", "DELETE FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       PQclear(res);
       
      
      res = PQprepare(conn, "uw3", "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T ORDER BY T_T.uw_Id", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       uw_set_error_message(ctx, "Unable to create prepared statement:\n%s\n%s", "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T ORDER BY T_T.uw_Id", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       PQclear(res);
       
      
      res = PQprepare(conn, "uw4", "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       uw_set_error_message(ctx, "Unable to create prepared statement:\n%s\n%s", "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       PQclear(res);
       
     return 0;
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
    PGconn *conn = PQconnectdb(env_db_str == NULL ? "dbname=sql_char.db" : env_db_str);
    if (conn == NULL) uw_error(ctx, FATAL, "libpq can't allocate a connection.");
    if (PQstatus(conn) != CONNECTION_OK) {
    char msg[1024];
     strncpy(msg, PQerrorMessage(conn), 1024);
     msg[1023] = 0;
     PQfinish(conn);
     uw_error(ctx, BOUNDED_RETRY, "Connection to Postgres server failed: %s", msg);
    }
    uw_set_db(ctx, conn);
    
    if (uw_db_validate(ctx) || uw_db_prepare(ctx)) {
    PQfinish(conn);
     uw_set_db(ctx, NULL);
     uw_reraise(ctx, FATAL);
     }
    }
 
 /* No global setup for LRU cache. */
  
 
  
  struct __uws_1 {
   uw_Basis_int __uwf_1;
    };
  struct __uws_2 {
   uw_Basis_char __uwf_1;
    };
  struct __uws_3 {
   uw_Basis_char __uwf_C;
    uw_Basis_int __uwf_Id;
     };
  struct __uws_4 {
   struct __uws_3 __uwf_T;
    };
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Sql_char_t constraints   */
   
  
  static uw_unit
   __uwn_wrap_matchComputed_3(uw_context ctx, uw_unit __uwr_x0_0, 
                               uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_n_2 =
           (uw_begin_region(ctx), ({
                                   struct __uws_1* disc =
                                   ({
                                    struct __uws_1* acc =
                                    NULL;
                                    int dummy = (uw_begin_region(ctx), 0);
                                    uw_ensure_transaction(ctx);
                                    char *query = ({
                                                   uw_Basis_string arg0 =
                                                    "SELECT COUNT(*) AS uw_1 FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_C = ";
                                                    
                                                    uw_Basis_string arg1 =
                                                     uw_Basis_sqlifyChar(ctx,
                                                      uw_Basis_chr(ctx, 233LL));
                                                     
                                                    uw_Basis_string arg2 = ")";
                                                     uw_Basis_mstrcat(ctx, arg0,
                                                                               
                                                                            arg1
                                                                            , 
                                                                            arg2
                                                   , NULL);
                                                   });
                                     
                                     PGconn *conn = uw_get_db(ctx);
                                      PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);
                                      
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
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query failed:\n%s\n%s", query, PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, query, PQerrorMessage(conn));
                                        }
                                       
                                       uw_end_region(ctx);
                                       uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                       n = PQntuples(res);
                                       for (i = 0; i < n; ++i) {
                                       struct __uws_1 __uwr_r_2;
                                        struct __uws_1* __uwr_acc_3 =
                                        acc;
                                        
                                        __uwr_r_2.__uwf_1 =
                                         (PQgetisnull(res, i, 0) ? ({uw_Basis_int
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                    tmp;
                                                                    }) : uw_Basis_stringToInt_error(ctx, 
                                                                          PQgetvalue(res, i, 0)));
                                         
                                        
                                        acc =
                                        ({
                                         struct __uws_1 *tmp =
                                         uw_malloc(ctx, sizeof(struct __uws_1));
                                         *tmp = __uwr_r_2;
                                         tmp;
                                         });
                                        }
                                       
                                       uw_pop_cleanup(ctx);
                                       
                                    acc;
                                    });
                                   
                                   disc == NULL ?
                                    ({
                                     uw_Basis_int
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", 
                                     "Query returned no rows");
                                     tmp;
                                     })
                                     :
                                    disc != NULL && 1 ?
                                     ({struct __uws_1 __uwr_r_2 = (*disc);
                                        __uwr_r_2.__uwf_1;
                                      })
                                      :
                                     ({
                                      uw_Basis_int
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:42:4-43:54: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_n_2
                                                                               ),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              " match chr 233</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_matchLiteral_4(uw_context ctx, uw_unit __uwr_x0_0, 
                              uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_n_2 =
           (uw_begin_region(ctx), ({
                                   struct __uws_1* disc =
                                   ({
                                    struct __uws_1* acc =
                                    NULL;
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
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query failed:\n%s\n%s", 
                                        "SELECT COUNT(*) AS uw_1 FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_C = E'\\\\351'::text)", PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                        "SELECT COUNT(*) AS uw_1 FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_C = E'\\\\351'::text)", PQerrorMessage(conn));
                                        }
                                       
                                       uw_end_region(ctx);
                                       uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                       n = PQntuples(res);
                                       for (i = 0; i < n; ++i) {
                                       struct __uws_1 __uwr_r_2;
                                        struct __uws_1* __uwr_acc_3 =
                                        acc;
                                        
                                        __uwr_r_2.__uwf_1 =
                                         (PQgetisnull(res, i, 0) ? ({uw_Basis_int
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                    tmp;
                                                                    }) : uw_Basis_stringToInt_error(ctx, 
                                                                          PQgetvalue(res, i, 0)));
                                         
                                        
                                        acc =
                                        ({
                                         struct __uws_1 *tmp =
                                         uw_malloc(ctx, sizeof(struct __uws_1));
                                         *tmp = __uwr_r_2;
                                         tmp;
                                         });
                                        }
                                       
                                       uw_pop_cleanup(ctx);
                                       
                                    acc;
                                    });
                                   
                                   disc == NULL ?
                                    ({
                                     uw_Basis_int
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", 
                                     "Query returned no rows");
                                     tmp;
                                     })
                                     :
                                    disc != NULL && 1 ?
                                     ({struct __uws_1 __uwr_r_2 = (*disc);
                                        __uwr_r_2.__uwf_1;
                                      })
                                      :
                                     ({
                                      uw_Basis_int
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:38:4-39:58: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_n_2
                                                                               ),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              " match the literal</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_echoComputed_5(uw_context ctx, uw_unit __uwr_x0_0, 
                              uw_unit __uwr___1)
   {
   return(({
           uw_Basis_char __uwr_c_2 =
           (uw_begin_region(ctx), ({
                                   struct __uws_2* disc =
                                   ({
                                    struct __uws_2* acc =
                                    NULL;
                                    int dummy = (uw_begin_region(ctx), 0);
                                    uw_ensure_transaction(ctx);
                                    char *query = ({
                                                   uw_Basis_string arg0 =
                                                    "SELECT ";
                                                    
                                                    uw_Basis_string arg1 =
                                                     uw_Basis_sqlifyChar(ctx,
                                                      uw_Basis_chr(ctx, 233LL));
                                                     
                                                    uw_Basis_string arg2 =
                                                     " AS uw_1 FROM uw_Sql_char_t AS T_T";
                                                     uw_Basis_mstrcat(ctx, arg0,
                                                                               
                                                                            arg1
                                                                            , 
                                                                            arg2
                                                   , NULL);
                                                   });
                                     
                                     PGconn *conn = uw_get_db(ctx);
                                      PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);
                                      
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
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query failed:\n%s\n%s", query, PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, query, PQerrorMessage(conn));
                                        }
                                       
                                       uw_end_region(ctx);
                                       uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                       n = PQntuples(res);
                                       for (i = 0; i < n; ++i) {
                                       struct __uws_2 __uwr_r_2;
                                        struct __uws_2* __uwr_acc_3 =
                                        acc;
                                        
                                        __uwr_r_2.__uwf_1 =
                                         (PQgetisnull(res, i, 0) ? ({uw_Basis_char
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                    tmp;
                                                                    }) : PQgetvalue(res, i, 0)[0]);
                                         
                                        
                                        acc =
                                        ({
                                         struct __uws_2 *tmp =
                                         uw_malloc(ctx, sizeof(struct __uws_2));
                                         *tmp = __uwr_r_2;
                                         tmp;
                                         });
                                        }
                                       
                                       uw_pop_cleanup(ctx);
                                       
                                    acc;
                                    });
                                   
                                   disc == NULL ?
                                    ({
                                     uw_Basis_char
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", 
                                     "Query returned no rows");
                                     tmp;
                                     })
                                     :
                                    disc != NULL && 1 ?
                                     ({struct __uws_2 __uwr_r_2 = (*disc);
                                        __uwr_r_2.__uwf_1;
                                      })
                                      :
                                     ({
                                      uw_Basis_char
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:33:4-34:44: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               uw_Basis_ord(ctx,
                                                                               __uwr_c_2
                                                                               )),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_echoLiteral_6(uw_context ctx, uw_unit __uwr_x0_0, 
                             uw_unit __uwr___1)
   {
   return(({
           uw_Basis_char __uwr_c_2 =
           (uw_begin_region(ctx), ({
                                   struct __uws_2* disc =
                                   ({
                                    struct __uws_2* acc =
                                    NULL;
                                    int dummy = (uw_begin_region(ctx), 0);
                                    uw_ensure_transaction(ctx);
                                    
                                     
                                     PGconn *conn = uw_get_db(ctx);
                                      static const int paramFormats[] = {  };
                                       const int *paramLengths = paramFormats;
                                        const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
                                       
                                       
                                      PGresult *res = PQexecPrepared(conn, "uw1", 0, paramValues, paramLengths, paramFormats, 0);
                                      
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
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query failed:\n%s\n%s", 
                                        "SELECT E'\\\\351'::text AS uw_1 FROM uw_Sql_char_t AS T_T", PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                        "SELECT E'\\\\351'::text AS uw_1 FROM uw_Sql_char_t AS T_T", PQerrorMessage(conn));
                                        }
                                       
                                       uw_end_region(ctx);
                                       uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                       n = PQntuples(res);
                                       for (i = 0; i < n; ++i) {
                                       struct __uws_2 __uwr_r_2;
                                        struct __uws_2* __uwr_acc_3 =
                                        acc;
                                        
                                        __uwr_r_2.__uwf_1 =
                                         (PQgetisnull(res, i, 0) ? ({uw_Basis_char
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                    tmp;
                                                                    }) : PQgetvalue(res, i, 0)[0]);
                                         
                                        
                                        acc =
                                        ({
                                         struct __uws_2 *tmp =
                                         uw_malloc(ctx, sizeof(struct __uws_2));
                                         *tmp = __uwr_r_2;
                                         tmp;
                                         });
                                        }
                                       
                                       uw_pop_cleanup(ctx);
                                       
                                    acc;
                                    });
                                   
                                   disc == NULL ?
                                    ({
                                     uw_Basis_char
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", 
                                     "Query returned no rows");
                                     tmp;
                                     })
                                     :
                                    disc != NULL && 1 ?
                                     ({struct __uws_2 __uwr_r_2 = (*disc);
                                        __uwr_r_2.__uwf_1;
                                      })
                                      :
                                     ({
                                      uw_Basis_char
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:28:4-29:44: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               uw_Basis_ord(ctx,
                                                                               __uwr_c_2
                                                                               )),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_drop_7(uw_context ctx, uw_Basis_int __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_int arg1 = __uwr_x0_0;
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                      paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                        arg1);
                                       
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw2", 1, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:21:4-22:49: DML failed:\n%s\n%s", 
                                        "DELETE FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
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
                                   uw_end_region(ctx), ((uw_write(ctx, ">dropped "), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_x0_0
                                                                               ),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_list_8(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
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
                                                                               PQexecPrepared(conn, "uw3", 0, paramValues, paramLengths, paramFormats, 0);
                                                                               
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
                                                                               uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:18:16-18:22: Query failed:\n%s\n%s", 
                                                                               "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T ORDER BY T_T.uw_Id", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               if (PQnfields(res) != 2) {
                                                                               int nf = PQnfields(res);
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:18:16-18:22: Query returned %d columns instead of 2:\n%s\n%s", nf, 
                                                                               "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T ORDER BY T_T.uw_Id", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               uw_end_region(ctx);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                                                               n = PQntuples(res);
                                                                               for (i = 0; i < n; ++i) {
                                                                               struct __uws_4 __uwr_r_2;
                                                                               uw_unit
                                                                               __uwr_acc_3
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_C
                                                                               =
                                                                               (PQgetisnull(res, i, 0) ? 
                                                                               ({uw_Basis_char
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:18:16-18:22: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               PQgetvalue(res, i, 0)[0]);
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               =
                                                                               (PQgetisnull(res, i, 1) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:18:16-18:22: Unexpectedly NULL field #1");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 1)));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ": "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               uw_Basis_ord(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_C
                                                                               )),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "<br />"), 0))));
                                                                               }
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                              uw_end_region(ctx);
                                                                               acc;
                                                                              })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_get_9(uw_context ctx, uw_Basis_int __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_3 __uwr_r_2 =
           (uw_begin_region(ctx), ({
                                   struct __uws_4* disc =
                                   ({
                                    struct __uws_4* acc =
                                    NULL;
                                    int dummy = (uw_begin_region(ctx), 0);
                                    uw_ensure_transaction(ctx);
                                    uw_Basis_int arg1 = __uwr_x0_0;
                                     
                                     PGconn *conn = uw_get_db(ctx);
                                      static const int paramFormats[] = { 0 };
                                       const int *paramLengths = paramFormats;
                                        const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                       paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                         arg1);
                                        
                                       
                                      PGresult *res = PQexecPrepared(conn, "uw4", 1, paramValues, paramLengths, paramFormats, 0);
                                      
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
                                        uw_error(ctx, FATAL, "$/top.ur:431:17-431:18: Query failed:\n%s\n%s", 
                                        "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 2) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:431:17-431:18: Query returned %d columns instead of 2:\n%s\n%s", nf, 
                                        "SELECT T_T.uw_C, T_T.uw_Id FROM uw_Sql_char_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                        }
                                       
                                       uw_end_region(ctx);
                                       uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                       n = PQntuples(res);
                                       for (i = 0; i < n; ++i) {
                                       struct __uws_4 __uwr_r_2;
                                        struct __uws_4* __uwr_acc_3 =
                                        acc;
                                        
                                        __uwr_r_2.__uwf_T.__uwf_C =
                                         (PQgetisnull(res, i, 0) ? ({uw_Basis_char
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "$/top.ur:431:17-431:18: Unexpectedly NULL field #0");
                                                                    tmp;
                                                                    }) : PQgetvalue(res, i, 0)[0]);
                                         
                                         __uwr_r_2.__uwf_T.__uwf_Id =
                                          (PQgetisnull(res, i, 1) ? ({uw_Basis_int
                                                                     tmp;
                                                                     uw_error(ctx, FATAL, "$/top.ur:431:17-431:18: Unexpectedly NULL field #1");
                                                                     tmp;
                                                                     }) : uw_Basis_stringToInt_error(ctx, 
                                                                           PQgetvalue(res, i, 1)));
                                          
                                        
                                        acc =
                                        ({
                                         struct __uws_4 *tmp =
                                         uw_malloc(ctx, sizeof(struct __uws_4));
                                         *tmp = __uwr_r_2;
                                         tmp;
                                         });
                                        }
                                       
                                       uw_pop_cleanup(ctx);
                                       
                                    acc;
                                    });
                                   
                                   disc == NULL ?
                                    ({
                                     struct __uws_3
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:432:24-433:3: %s", 
                                     "Query returned no rows");
                                     tmp;
                                     })
                                     :
                                    disc != NULL && 1 ?
                                     ({struct __uws_4 __uwr_r_2 = (*disc);
                                        __uwr_r_2.__uwf_T;
                                      })
                                      :
                                     ({
                                      struct __uws_3
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:12:4-13:46: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               uw_Basis_ord(ctx,
                                                                               __uwr_r_2.__uwf_C
                                                                               )),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_store_10(uw_context ctx, uw_Basis_int __uwr_x0_0, 
                        uw_Basis_int __uwr_x1_1, uw_unit __uwr___2)
   {
   return(({
           uw_unit __uwr___3 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   char *dml = ({
                                                uw_Basis_string arg0 =
                                                 "INSERT INTO uw_Sql_char_t (uw_C, uw_Id) VALUES (";
                                                 
                                                 uw_Basis_string arg1 =
                                                  uw_Basis_sqlifyChar(ctx,
                                                   uw_Basis_chr(ctx,
                                                    __uwr_x1_1));
                                                  uw_Basis_string arg2 = ", ";
                                                   
                                                 uw_Basis_string arg3 =
                                                  uw_Basis_sqlifyInt(ctx,
                                                   __uwr_x0_0);
                                                  uw_Basis_string arg4 = ")";
                                                   uw_Basis_mstrcat(ctx, arg0, 
                                                                          arg1, 
                                                                          arg2, 
                                                                          arg3, 
                                                                          arg4,
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
                                        uw_error(ctx, FATAL, "tests/sql_char/sql_char.ur:8:4-9:47: DML failed:\n%s\n%s", dml, PQerrorMessage(conn));
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
                                   uw_end_region(ctx), ((uw_write(ctx, ">stored "), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_x1_1
                                                                               ),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
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
 
 if (!strncmp(request, "/Sql_char/store", 15) && (request[15] == 0 || request[15] == '/')) {
  request += 15;
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
    uw_Basis_int arg1 = uw_Basis_unurlifyInt(ctx, &request);
     __uwn_wrap_store_10(ctx, arg0, arg1, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Sql_char/get", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     __uwn_wrap_get_9(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql_char/list", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
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
     __uwn_wrap_list_8(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql_char/drop", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
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
     __uwn_wrap_drop_7(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql_char/echoLiteral", 21) && (request[21] == 0 || request[21] == '/')) {
   request += 21;
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
     __uwn_wrap_echoLiteral_6(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql_char/echoComputed", 22) && (request[22] == 0 || request[22] == '/')) {
   request += 22;
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
     __uwn_wrap_echoComputed_5(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql_char/matchLiteral", 22) && (request[22] == 0 || request[22] == '/')) {
   request += 22;
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
     __uwn_wrap_matchLiteral_4(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Sql_char/matchComputed", 23) && (request[23] == 0 || request[23] == '/')) {
   request += 23;
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
     __uwn_wrap_matchComputed_3(ctx, arg0, 0);
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
 uw_set_could_write_db(ctx, 0);
  uw_begin_initializing(ctx);
  uw_end_initializing(ctx);
  __uwn_initializer_1(ctx, 0);
   }
 uw_app uw_application = {1,
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
                                       uw_db_check,
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
 