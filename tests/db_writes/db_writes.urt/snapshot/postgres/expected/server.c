#include "include/urweb/config.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <time.h>
 #include <libpq-fe.h>
  #include "tests/ioprobe/probe.h"
   #include "include/urweb/urweb.h"
 
 static void uw_setup_limits() {
  uw_min_heap = 65536;
   
   }
  
  void uw_global_custom() {
   uw_setup_limits();
   }
   static int uw_db_check(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_db_writes_t'");
     
     if (res == NULL) {
     uw_set_error_message(ctx, "Out of memory allocating query result.");
      return 1;
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_db_writes_t'", PQerrorMessage(conn));
      PQclear(res);
      return 1;
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      uw_set_error_message(ctx, "%s", "Table 'uw_db_writes_t' does not exist.");
      return 1;
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_db_writes_t' AND ((LOWER(column_name) = 'uw_n' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))");
      
      if (res == NULL) {
      uw_set_error_message(ctx, "Out of memory allocating query result.");
       return 1;
       }
      
      if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_db_writes_t' AND ((LOWER(column_name) = 'uw_n' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))", PQerrorMessage(conn));
       PQclear(res);
       return 1;
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "1")) {
      PQclear(res);
       uw_set_error_message(ctx, "%s", "Table 'uw_db_writes_t' has the wrong column types.");
       return 1;
       }
      
      PQclear(res);
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_db_writes_t' AND LOWER(column_name) LIKE 'uw_%'");
       
       if (res == NULL) {
       uw_set_error_message(ctx, "Out of memory allocating query result.");
        return 1;
        }
       
       if (PQresultStatus(res) != PGRES_TUPLES_OK) {
       uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_db_writes_t' AND LOWER(column_name) LIKE 'uw_%'", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       
       if (strcmp(PQgetvalue(res, 0, 0), "1")) {
       PQclear(res);
        uw_set_error_message(ctx, "%s", "Table 'uw_db_writes_t' has extra columns.");
        return 1;
        }
       
       PQclear(res);
       res = PQexec(conn, "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_db_writes_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)");
        
        if (res == NULL) {
        uw_set_error_message(ctx, "Out of memory allocating query result.");
         return 1;
         }
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_db_writes_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)", PQerrorMessage(conn));
         PQclear(res);
         return 1;
         }
        
        if (strcmp(PQgetvalue(res, 0, 0), "1")) {
        PQclear(res);
         uw_set_error_message(ctx, "%s", "Sequence 'uw_Db_writes_s' does not exist.");
         return 1;
         }
        
        PQclear(res);
        
    return 0;
    }
    
    static void uw_db_validate(uw_context ctx) {
     static int validated;
     char msg[ERROR_BUF_LEN];
     
     if (validated)
     return;
      
     if (uw_db_check(ctx)) {
     strcpy(msg, uw_error_message(ctx));
      uw_error(ctx, FATAL, "%s", msg);
      }
     
     validated = 1;
     }
     
     static void uw_db_prepare(uw_context ctx) {
     PGconn *conn = uw_get_db(ctx);
     PGresult *res;
     
     res = PQprepare(conn, "uw0", "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T\n%s", msg);
       }
      PQclear(res);
      
      
      res = PQprepare(conn, "uw1", "INSERT INTO uw_Db_writes_t (uw_N) VALUES ($1::int8)", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       char msg[1024];
        strncpy(msg, PQerrorMessage(conn), 1024);
        msg[1023] = 0;
        PQclear(res);
        PQfinish(conn);
        uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Db_writes_t (uw_N) VALUES ($1::int8)\n%s", msg);
        }
       PQclear(res);
       
      
      res = PQprepare(conn, "uw2", "DELETE FROM uw_Db_writes_t AS T_T WHERE (T_T.uw_N < 0::int8)", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       char msg[1024];
        strncpy(msg, PQerrorMessage(conn), 1024);
        msg[1023] = 0;
        PQclear(res);
        PQfinish(conn);
        uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Db_writes_t AS T_T WHERE (T_T.uw_N < 0::int8)\n%s", msg);
        }
       PQclear(res);
       
      
      res = PQprepare(conn, "uw3", "SELECT NEXTVAL('uw_Db_writes_s')", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       char msg[1024];
        strncpy(msg, PQerrorMessage(conn), 1024);
        msg[1023] = 0;
        PQclear(res);
        PQfinish(conn);
        uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT NEXTVAL('uw_Db_writes_s')\n%s", msg);
        }
       PQclear(res);
       
      
      res = PQprepare(conn, "uw4", "INSERT INTO uw_Db_writes_t (uw_N) VALUES (1::int8)", 0, NULL);
       if (PQresultStatus(res) != PGRES_COMMAND_OK) {
       char msg[1024];
        strncpy(msg, PQerrorMessage(conn), 1024);
        msg[1023] = 0;
        PQclear(res);
        PQfinish(conn);
        uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Db_writes_t (uw_N) VALUES (1::int8)\n%s", msg);
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
    PGconn *conn = PQconnectdb(env_db_str == NULL ? "dbname=db_writes.db" : env_db_str);
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
   uw_Basis_int __uwf_1;
    };
  
  static char jslib[] = "*runtime elided*";
   static char jsapp[] = "*script elided*";
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Db_writes_t constraints   */
   
  /* SQL sequence uw_Db_writes_s */
   
  
  static uw_Basis_int
   __uwn_readThenWrite_3(uw_context ctx, uw_Basis_int __uwr_n_0, 
                          uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_k_2 =
           (uw_begin_region(ctx), ({
                                   uw_io_transaction __uwt;
                                   uw_Basis_int
                                   __uwr;
                                   uw_set_could_write_db(ctx, 0);
                                   uw_io_transaction_begin(ctx, &__uwt);
                                   while (1) {
                                   int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                    if (__uwfk == 0) {
                                    __uwr =
                                     ({
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
                                           "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
                                           }
                                          
                                          if (PQnfields(res) != 1) {
                                          int nf = PQnfields(res);
                                           PQclear(res);
                                           uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                           "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
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
                                            uw_malloc(ctx, sizeof(struct
                                                                   __uws_1));
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
                                         uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:43:9-43:61: pattern match failure");
                                         tmp;
                                         });
                                      });
                                     uw_io_transaction_commit(ctx, &__uwt);
                                     break;
                                     } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                                    break;
                                     }
                                    }
                                   uw_io_transaction_end(ctx, &__uwt, 1);
                                   __uwr;
                                   }));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), ({
                                     uw_io_transaction __uwt;
                                     uw_unit
                                     __uwr;
                                     uw_set_could_write_db(ctx, 1);
                                     uw_io_transaction_begin(ctx, &__uwt);
                                     while (1) {
                                     int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                      if (__uwfk == 0) {
                                      __uwr =
                                       (uw_begin_region(ctx), ({
                                        uw_Basis_int arg1 = __uwr_n_0;
                                         
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
                                             uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:44:4-44:59: DML failed:\n%s\n%s", 
                                             "INSERT INTO uw_Db_writes_t (uw_N) VALUES ($1::int8)", PQerrorMessage(conn));
                                            }
                                              
                                              PQclear(res);
                                              
                                        
                                        uw_end_region(ctx);
                                        0;
                                        }));
                                       uw_io_transaction_commit(ctx, &__uwt);
                                       break;
                                       } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                                      break;
                                       }
                                      }
                                     uw_io_transaction_end(ctx, &__uwt, 1);
                                     __uwr;
                                     }));
             uw_end_region(ctx);
              __uwr_k_2;
             });
           }));
   }
  
  
  
  
  
  static uw_unit
   __uwn_wrap_writesThroughFfi_4(uw_context ctx, uw_unit __uwr_x0_0, 
                                  uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), uw_Probe_pendingWrite(ctx, "uw_Db_writes_t"));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), uw_Probe_finishPendingWrite(ctx));
             uw_end_region(ctx);
              ((uw_write(ctx, "<body"), 0),
               (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                      uw_Basis_get_settings(ctx,
                                                       0))), 0),
                uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                           "")), 0),
                                     uw_end_region(ctx), (uw_write(ctx, ">written</body>"), 0))));
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_writesThroughCall_5(uw_context ctx, uw_unit __uwr_x0_0, 
                                   uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
            
             
             uw_ensure_transaction(ctx);
             
             PGconn *conn = uw_get_db(ctx);
              static const int paramFormats[] = {  };
               const int *paramLengths = paramFormats;
                const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
               
               
              PGresult *res;
              
              res = PQexecPrepared(conn, "uw2", 0, paramValues, paramLengths, paramFormats, 0);
              
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
                 uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:32:4-35:2: DML failed:\n%s\n%s", 
                 "DELETE FROM uw_Db_writes_t AS T_T WHERE (T_T.uw_N < 0::int8)", PQerrorMessage(conn));
                }
                  
                  PQclear(res);
                  
            
            uw_end_region(ctx);
            0;
            }));
           ((uw_write(ctx, "<body"), 0),
            (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                   uw_Basis_get_settings(ctx,
                                                    0))), 0),
             uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                        "")), 0),
                                  uw_end_region(ctx), (uw_write(ctx, ">cleared</body>"), 0))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_draws_6(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                              ({
                                                                               uw_Basis_int n;
                                                                               uw_ensure_transaction(ctx);
                                                                               PGconn *conn = uw_get_db(ctx);
                                                                               
                                                                               PGresult *res = 
                                                                               PQexecPrepared(conn, "uw3", 0, NULL, NULL, NULL, 0);
                                                                               
                                                                               if (res == NULL) {
                                                                               
                                                                               uw_try_reconnecting_and_restarting(ctx);
                                                                               uw_error(ctx, FATAL, "Can't allocate NEXTVAL result; database server may be down.");
                                                                               }
                                                                               
                                                                               if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:26:24-26:25: Query failed:\n%s\n%s", 
                                                                               "SELECT NEXTVAL('uw_Db_writes_s')", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               n = PQntuples(res);
                                                                               if (n != 1) {
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:26:24-26:25: Wrong number of result rows:\n%s\n%s", 
                                                                               "SELECT NEXTVAL('uw_Db_writes_s')", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               n = uw_Basis_stringToInt_error(ctx, PQgetvalue(res, 0, 0));
                                                                               PQclear(res);
                                                                               
                                                                               
                                                                               n;
                                                                               })
                                                                              ),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_writes_7(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
            
             
             uw_ensure_transaction(ctx);
             
             PGconn *conn = uw_get_db(ctx);
              static const int paramFormats[] = {  };
               const int *paramLengths = paramFormats;
                const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
               
               
              PGresult *res;
              
              res = PQexecPrepared(conn, "uw4", 0, paramValues, paramLengths, paramFormats, 0);
              
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
                 uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:21:4-24:2: DML failed:\n%s\n%s", 
                 "INSERT INTO uw_Db_writes_t (uw_N) VALUES (1::int8)", PQerrorMessage(conn));
                }
                  
                  PQclear(res);
                  
            
            uw_end_region(ctx);
            0;
            }));
           ((uw_write(ctx, "<body"), 0),
            (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                   uw_Basis_get_settings(ctx,
                                                    0))), 0),
             uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                        "")), 0),
                                  uw_end_region(ctx), (uw_write(ctx, ">written</body>"), 0))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_readsWithEffect_8(uw_context ctx, uw_unit __uwr_x0_0, 
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
                                        "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                        "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
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
                                      uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:16:4-20:2: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), uw_Probe_effectT(ctx, "an effect"));
             uw_end_region(ctx);
              ((uw_write(ctx, "<body"), 0),
               (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                      uw_Basis_get_settings(ctx,
                                                       0))), 0),
                uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                           "")), 0),
                                     uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                          (uw_begin_region(ctx),
                                                            uw_Basis_htmlifyInt_w(ctx,
                                                             __uwr_n_2),
                                                           uw_end_region(ctx), (uw_write(ctx, 
                                                                               "</body>"), 0))))));
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_reads_9(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
                                        "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                        "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
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
                                      uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:11:4-14:2: pattern match failure");
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
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_10(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_v_2 =
           uw_Basis_new_client_source(ctx,
            ({
             uw_Basis_string arg0 = "{c:\"c\",v:";
              uw_Basis_string arg1 = uw_Basis_htmlifyInt(ctx, 0LL);
               uw_Basis_string arg2 = "}";
                uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
             }));
           ((uw_write(ctx, "<body"), 0),
            (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                   uw_Basis_get_settings(ctx,
                                                    0))), 0),
             uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                        "")), 0),
                                  uw_end_region(ctx), ((uw_write(ctx, ">\n<a href=\"/Db_writes/reads\">reads</a>\n<a href=\"/Db_writes/readsWithEffect\">reads with an effect</a>\n<a href=\"/Db_writes/writes\">writes</a>\n<a href=\"/Db_writes/draws\">draws</a>\n<a href=\"/Db_writes/writesThroughCall\">writes through a call</a>\n<a href=\"/Db_writes/writesThroughFfi\">writes through an FFI function</a>\n<button onclick='uw_event=event;exec("), 0),
                                                       (uw_begin_region(ctx), ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_v_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                        uw_end_region(ctx), ((uw_write(ctx, 
                                                                              ")'>io</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                             (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_v_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                              uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))));
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static void uw_periodic0(uw_context ctx) {
  uw_set_could_write_db(ctx, 1);
   uw_unit __uwr_$x_0 = 0, __uwr_$y_1 = 0;
    (uw_begin_region(ctx), ({
     
      
      uw_ensure_transaction(ctx);
      
      PGconn *conn = uw_get_db(ctx);
       static const int paramFormats[] = {  };
        const int *paramLengths = paramFormats;
         const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
        
        
       PGresult *res;
       
       res = PQexecPrepared(conn, "uw2", 0, paramValues, paramLengths, paramFormats, 0);
       
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
          uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:51:0-51:38: DML failed:\n%s\n%s", 
          "DELETE FROM uw_Db_writes_t AS T_T WHERE (T_T.uw_N < 0::int8)", PQerrorMessage(conn));
         }
           
           PQclear(res);
           
     
     uw_end_region(ctx);
     0;
     }));
    }
  
  static void uw_periodic1(uw_context ctx) {
   uw_set_could_write_db(ctx, 0);
    uw_unit __uwr_$x_0 = 0, __uwr_$y_1 = 0;
     ({
      uw_Basis_int __uwr___2 =
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
                                   "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
                                   }
                                  
                                  if (PQnfields(res) != 1) {
                                  int nf = PQnfields(res);
                                   PQclear(res);
                                   uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                   "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
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
                                 uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:54:4-55:13: pattern match failure");
                                 tmp;
                                 });
                              }));
      uw_end_region(ctx);
       0;
      });
     }
   
   static void uw_periodic2(uw_context ctx) {
    uw_unit __uwr_$x_0 = 0, __uwr_$y_1 = 0;
     ({
      uw_Basis_int __uwr___2 =
      (uw_begin_region(ctx), ({
                              uw_io_transaction __uwt;
                              uw_Basis_int
                              __uwr;
                              uw_set_could_write_db(ctx, 0);
                              uw_io_transaction_begin(ctx, &__uwt);
                              while (1) {
                              int __uwfk = setjmp(*uw_jmp_buf(ctx));
                               if (__uwfk == 0) {
                               __uwr =
                                ({
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
                                      "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
                                      }
                                     
                                     if (PQnfields(res) != 1) {
                                     int nf = PQnfields(res);
                                      PQclear(res);
                                      uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                      "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
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
                                    uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:59:9-59:61: pattern match failure");
                                    tmp;
                                    });
                                 });
                                uw_io_transaction_commit(ctx, &__uwt);
                                break;
                                } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                               break;
                                }
                               }
                              uw_io_transaction_end(ctx, &__uwt, 1);
                              __uwr;
                              }));
      uw_end_region(ctx);
       ({
        uw_io_transaction __uwt;
        uw_unit
        __uwr;
        uw_set_could_write_db(ctx, 1);
        uw_io_transaction_begin(ctx, &__uwt);
        while (1) {
        int __uwfk = setjmp(*uw_jmp_buf(ctx));
         if (__uwfk == 0) {
         __uwr =
          (uw_begin_region(ctx), ({
           
            
            uw_ensure_transaction(ctx);
            
            PGconn *conn = uw_get_db(ctx);
             static const int paramFormats[] = {  };
              const int *paramLengths = paramFormats;
               const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
              
              
             PGresult *res;
             
             res = PQexecPrepared(conn, "uw2", 0, paramValues, paramLengths, paramFormats, 0);
             
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
                uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:60:4-60:29: DML failed:\n%s\n%s", 
                "DELETE FROM uw_Db_writes_t AS T_T WHERE (T_T.uw_N < 0::int8)", PQerrorMessage(conn));
               }
                 
                 PQclear(res);
                 
           
           uw_end_region(ctx);
           0;
           }));
          uw_io_transaction_commit(ctx, &__uwt);
          break;
          } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
         break;
          }
         }
        uw_io_transaction_end(ctx, &__uwt, 1);
        __uwr;
        });
      });
     }
    
    static uw_periodic my_periodics[] = {{uw_periodic0, 3600, 0},{uw_periodic1,
                                                                  7200, 0},{uw_periodic2,
                                                                            10800,
                                                                            1},{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Db_writes/main")) return 1;
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
 
 if (!strcmp(request, "/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js")) {
  uw_write_header(ctx, "Content-Type: text/javascript\r\n");
   uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
   uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
   uw_write(ctx, jslib);
   return;
   }
  
  
  if (!strcmp(request, "/app.42D1CE3E3AC93E84BBE7683157F375CDCC24EF35.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Db_writes/main", 15) && (request[15] == 0 || request[15] == '/')) {
  request += 15;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.42D1CE3E3AC93E84BBE7683157F375CDCC24EF35.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_10(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Db_writes/readThenWrite", 24) && (request[24] == 0 || request[24] == '/')) {
   request += 24;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 1);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_int it0 =
      __uwn_readThenWrite_3(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      uw_Basis_urlifyInt_w(ctx, it0);
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Db_writes/reads", 16) && (request[16] == 0 || request[16] == '/')) {
   request += 16;
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
     __uwn_wrap_reads_9(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Db_writes/readsWithEffect", 26) && (request[26] == 0 || request[26] == '/')) {
   request += 26;
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
     __uwn_wrap_readsWithEffect_8(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Db_writes/writes", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
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
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     __uwn_wrap_writes_7(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Db_writes/draws", 16) && (request[16] == 0 || request[16] == '/')) {
   request += 16;
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
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     __uwn_wrap_draws_6(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Db_writes/writesThroughCall", 28) && (request[28] == 0 || request[28] == '/')) {
   request += 28;
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
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     __uwn_wrap_writesThroughCall_5(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Db_writes/writesThroughFfi", 27) && (request[27] == 0 || request[27] == '/')) {
   request += 27;
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
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     __uwn_wrap_writesThroughFfi_4(ctx, arg0, 0);
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
  ({
   uw_unit __uwr_$x_0 = 0, __uwr_$y_1 = 0;
   ({
    uw_Basis_int __uwr___2 =
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
                                 "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
                                 }
                                
                                if (PQnfields(res) != 1) {
                                int nf = PQnfields(res);
                                 PQclear(res);
                                 uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                 "SELECT COUNT(*) AS uw_1 FROM uw_Db_writes_t AS T_T", PQerrorMessage(conn));
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
                               uw_error(ctx, FATAL, "tests/db_writes/db_writes.ur:48:4-49:13: pattern match failure");
                               tmp;
                               });
                            }));
    uw_end_region(ctx);
     0;
    });
   });
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
                                                       1};
 