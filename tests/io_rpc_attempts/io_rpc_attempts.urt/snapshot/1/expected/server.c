#include "include/urweb/config.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <time.h>
 #include <sqlite3.h>
  #include "tests/ioprobe/probe.h"
   #include "include/urweb/urweb.h"
 
 static void uw_setup_limits() {
  }
  
  void uw_global_custom() {
   uw_setup_limits();
   }
   typedef struct {
    sqlite3 *conn;
     sqlite3_stmt *p0;
      sqlite3_stmt *p1;
       } uw_conn;
    
    static sqlite3_vfs uw_pinned_vfs;
    
    static int uw_pinned_clock_int64(sqlite3_vfs *vfs, sqlite3_int64 *out) {
    int64_t t = 0;
     (void)vfs;
     uw_reproducible_epoch(&t);
     *out = t * 1000 + 210866760000000LL;
     return SQLITE_OK;
     }
    
    static int uw_pinned_clock(sqlite3_vfs *vfs, double *out) {
    sqlite3_int64 t;
     uw_pinned_clock_int64(vfs, &t);
     *out = t / 86400000.0;
     return SQLITE_OK;
     }
    
    static int uw_pinned_randomness(sqlite3_vfs *vfs, int n, char *out) {
    uint64_t r;
     int i;
     (void)vfs;
     for (i = 0; i < n; i += sizeof r) {
     uw_reproducible_rand(&r);
      memcpy(out + i, &r, n - i < (int)sizeof r ? n - i : (int)sizeof r);
      }
     return n;
     }
    
    static void uw_forget_seed(void) {
    sqlite3_randomness(0, NULL);
     }
    
    static void uw_client_init(void) {
    int64_t t;
     int pin_clock = uw_reproducible_epoch(&t), pin_rand = uw_reproducible_rand_pinned();
     if (pin_clock || pin_rand) {
     uw_pinned_vfs = *sqlite3_vfs_find(NULL);
      uw_pinned_vfs.zName = "urweb-pinned";
      if (pin_clock) {
      uw_pinned_vfs.xCurrentTime = uw_pinned_clock;
       if (uw_pinned_vfs.iVersion >= 2) { uw_pinned_vfs.xCurrentTimeInt64 = uw_pinned_clock_int64; }
       }
      if (pin_rand) {
      uw_pinned_vfs.xRandomness = uw_pinned_randomness;
       uw_reproducible_on_attempt(uw_forget_seed);
       }
      sqlite3_vfs_register(&uw_pinned_vfs, 1);
      }
     uw_sqlfmtInt = "%lld%n";
     uw_sqlfmtFloat = "%.16g%n";
     uw_Estrings = 0;
     uw_sql_type_annotations = 0;
     uw_sql_utc_times = 1;
     uw_sqlsuffixString = "";
     uw_sqlsuffixChar = "";
     uw_sqlsuffixBlob = "";
     uw_sqlfmtUint4 = "%u%n";
     }
    
    static int uw_db_check_with(uw_context ctx, failure_kind busy) {
     uw_conn *conn = uw_get_db(ctx);
     sqlite3_stmt *stmt;
     int res;
     
     if (sqlite3_prepare_v2(conn->conn, "PRAGMA journal_mode", -1, &stmt, NULL) != SQLITE_OK) {
      uw_set_error_message(ctx, "Query preparation failed: %s: %s", "PRAGMA journal_mode", sqlite3_errmsg(conn->conn));
       return 1;
       }
      
      res = sqlite3_step(stmt);
      
      if (res == SQLITE_BUSY) {
      sqlite3_finalize(stmt);
       uw_error(ctx, busy, "Database is busy; can't validate: %s", "PRAGMA journal_mode");
       }
      
      if (res != SQLITE_ROW) {
      uw_set_error_message(ctx, "Error getting row: %s: %s", "PRAGMA journal_mode", res == SQLITE_DONE ? "no row" : sqlite3_errmsg(conn->conn));
       sqlite3_finalize(stmt);
       return 1;
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       uw_set_error_message(ctx, "Bad column count: %s", "PRAGMA journal_mode");
       return 1;
       }
      
      if (sqlite3_stricmp((const char *)sqlite3_column_text(stmt, 0), "wal")) {
       uw_set_error_message(ctx, "The SQLite database is in journal mode %s, not WAL (set it with PRAGMA journal_mode = WAL)", (const char *)sqlite3_column_text(stmt, 0));
        sqlite3_finalize(stmt);
        return 1;
        }
       
      sqlite3_finalize(stmt);
      
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_rpc_attempts_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      uw_set_error_message(ctx, "Query preparation failed: %s: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_rpc_attempts_t' COLLATE NOCASE", sqlite3_errmsg(conn->conn));
       return 1;
       }
      
      res = sqlite3_step(stmt);
      
      if (res == SQLITE_BUSY) {
      sqlite3_finalize(stmt);
       uw_error(ctx, busy, "Database is busy; can't validate: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_rpc_attempts_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      uw_set_error_message(ctx, "Error getting row: %s: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_rpc_attempts_t' COLLATE NOCASE", res == SQLITE_DONE ? "no row" : sqlite3_errmsg(conn->conn));
       sqlite3_finalize(stmt);
       return 1;
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       uw_set_error_message(ctx, "Bad column count: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_rpc_attempts_t' COLLATE NOCASE");
       return 1;
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
       sqlite3_finalize(stmt);
        uw_set_error_message(ctx, "Table '%s' does not exist.", "uw_Io_rpc_attempts_t");
        return 1;
        }
       
      sqlite3_finalize(stmt);
      
     return 0;
     }
     
     static int uw_db_check(uw_context ctx) {
     return uw_db_check_with(ctx, UNLIMITED_RETRY);
      }
     
     static int uw_db_validate(uw_context ctx) {
      static int validated;
      
      if (validated)
      return 0;
       if (uw_db_check_with(ctx, FATAL))
      return 1;
       validated = 1;
      return 0;
      }
      
      static int uw_db_prepare(uw_context ctx) {
      uw_conn *conn = uw_get_db(ctx);
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &conn->p0, NULL) != SQLITE_OK) {
       uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
        return 1;
        }
       
       
       if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N FROM uw_Io_rpc_attempts_t AS T_T ORDER BY T_T.uw_N", -1, &conn->p1, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "SELECT T_T.uw_N FROM uw_Io_rpc_attempts_t AS T_T ORDER BY T_T.uw_N", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
      return 0;
      }
    
    static void uw_db_close(uw_context ctx);
    
    static void uw_db_init(uw_context ctx) {
    sqlite3 *sqlite;
    sqlite3_stmt *stmt;
    uw_conn *conn;
    
    char *env_db_str = getenv("URWEB_SQLITE_DB_PATH");
    const char *sqlite_db_path = env_db_str == NULL ? "io_rpc_attempts.db" : env_db_str;
    if (sqlite3_open_v2(sqlite_db_path, &sqlite, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Can't open SQLite database: %s", sqlite_db_path);
    
    if (sqlite3_exec(sqlite, "PRAGMA busy_timeout = 10000", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't set `PRAGMA busy_timeout` for SQLite database: %s", sqlite3_errmsg(sqlite));
     
    if (sqlite3_exec(sqlite, "PRAGMA foreign_keys = ON", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't enable foreign_keys for SQLite database: %s", sqlite3_errmsg(sqlite));
     
    if (sqlite3_exec(sqlite, "PRAGMA synchronous = FULL", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't enable `PRAGMA synchronous = FULL` for SQLite database: %s", sqlite3_errmsg(sqlite));
     
    if (sqlite3_exec(sqlite, "PRAGMA mmap_size = 268435456", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't set `PRAGMA mmap_size` for SQLite database: %s", sqlite3_errmsg(sqlite));
     
    if (uw_database_max < SIZE_MAX) {
    static int warned;
     char buf[100];
     sqlite3_int64 page_size, pages;
     
     if (sqlite3_prepare_v2(sqlite, "PRAGMA page_size", -1, &stmt, NULL) != SQLITE_OK
         || sqlite3_step(stmt) != SQLITE_ROW) {
     uw_set_error_message(ctx, "Can't read the page size of the SQLite database: %s", sqlite3_errmsg(sqlite));
      sqlite3_finalize(stmt);
      sqlite3_close(sqlite);
      uw_reraise(ctx, FATAL);
      }
     page_size = sqlite3_column_int64(stmt, 0);
     sqlite3_finalize(stmt);
     
     if (page_size <= 0 || uw_database_max < (size_t)page_size) {
     sqlite3_close(sqlite);
      uw_error(ctx, FATAL, "limit database is %zu bytes, less than one page of the SQLite database (%lld bytes)", uw_database_max, (long long)page_size);
      }
     
     sprintf(buf, "PRAGMA max_page_count = %llu", (unsigned long long)(uw_database_max / page_size));
     
     if (sqlite3_prepare_v2(sqlite, buf, -1, &stmt, NULL) != SQLITE_OK) {
     sqlite3_close(sqlite);
      uw_error(ctx, FATAL, "Can't prepare max_page_count query for SQLite database");
      }
     
     if (sqlite3_step(stmt) != SQLITE_ROW) {
     sqlite3_finalize(stmt);
      sqlite3_close(sqlite);
      uw_error(ctx, FATAL, "Can't set max_page_count parameter for SQLite database");
      }
     pages = sqlite3_column_int64(stmt, 0);
     sqlite3_finalize(stmt);
     
     if (!warned) {
     if (pages > (sqlite3_int64)(uw_database_max / page_size))
      fprintf(stderr, "The SQLite database is already larger than limit database (%zu bytes): it keeps its size of %lld pages of %lld bytes.\n", uw_database_max, (long long)pages, (long long)page_size);
       warned = 1;
      }
     }
    
    conn = calloc(1, sizeof(uw_conn));
    conn->conn = sqlite;
    uw_set_db(ctx, conn);
    
    if (uw_db_validate(ctx) || uw_db_prepare(ctx)) {
    uw_db_close(ctx);
     free(conn);
     uw_set_db(ctx, NULL);
     uw_reraise(ctx, FATAL);
     }
    }
    
    static void uw_db_close(uw_context ctx) {
    uw_conn *conn = uw_get_db(ctx);
    if (conn->p0) sqlite3_finalize(conn->p0);
     if (conn->p1) sqlite3_finalize(conn->p1);
      sqlite3_close(conn->conn);
    }
    
    static int uw_db_begin(uw_context ctx, int could_write) {
    uw_conn *conn = uw_get_db(ctx);
    int may_wait = uw_db_may_wait(ctx);
    int r;
    
    if (!may_wait) sqlite3_busy_timeout(conn->conn, 0);
    r = sqlite3_exec(conn->conn, could_write ? "BEGIN IMMEDIATE" : "BEGIN", NULL, NULL, NULL);
    if (!may_wait) sqlite3_busy_timeout(conn->conn, 10000);
    
    if (r == SQLITE_OK)
    return 0;
     else if (r == SQLITE_BUSY) {
    uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
     } else {
    fprintf(stderr, "Begin error: %s<br />", sqlite3_errmsg(conn->conn));
     return 1;
     }
    }
    static int uw_db_commit(uw_context ctx) {
    uw_conn *conn = uw_get_db(ctx);
    if (sqlite3_exec(conn->conn, "COMMIT", NULL, NULL, NULL) == SQLITE_OK)
    return 0;
     else {
    fprintf(stderr, "Commit error: %s<br />", sqlite3_errmsg(conn->conn));
     return 1;
     }
    }
    
    static int uw_db_rollback(uw_context ctx) {
    uw_conn *conn = uw_get_db(ctx);
    if (sqlite3_get_autocommit(conn->conn)) return 0;
    if (sqlite3_exec(conn->conn, "ROLLBACK", NULL, NULL, NULL) == SQLITE_OK)
    return 0;
     else {
    fprintf(stderr, "Rollback error: %s<br />", sqlite3_errmsg(conn->conn));
     return 1;
     }
    }
    
    
 
 /* No global setup for LRU cache. */
  
 
  
  
  
  enum __uwe_result_s_3 { __uwc_Success_4, __uwc_Failure_5 };
   
   struct __uwd_result_s_3 {
   enum __uwe_result_s_3
   tag;
   union {
    uw_unit uw_Success;
     uw_Basis_string uw_Failure;
    } data;
    };
  struct __uws_1 {
   uw_Basis_int __uwf_N;
    };
  struct __uws_2 {
   struct __uws_1 __uwf_1;
    struct __uws_2* __uwf_2;
     };
  struct __uws_3 {
   struct __uws_1 __uwf_T;
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
  
  /* cookie Io_rpc_attempts/c */
  /* SQL table uw_Io_rpc_attempts_t constraints   */
   
  
  static uw_Basis_int
   __uwn_handled_6(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           struct __uwd_result_s_3* __uwr_r_2 =
           ({
            uw_unit* disc =
            ({
             uw_io_transaction __uwt;
             uw_unit
             *__uwr = NULL;
             uw_set_could_write_db(ctx, 1);
             uw_io_transaction_begin(ctx, &__uwt);
             while (1) {
             int __uwfk = setjmp(*uw_jmp_buf(ctx));
              if (__uwfk == 0) {
              uw_unit __uwv =
               ({
                uw_unit __uwr___2 =
                (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                        uw_Basis_int arg1 = __uwr_n_0;
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p0;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p0 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:13:28-14:73: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                           
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:13:28-14:73: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:13:28-14:73", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                            sqlite3_free(expanded);
                                            }
                                           uw_pop_cleanup(ctx);
                                          uw_pop_cleanup(ctx);
                                          
                                        
                                        uw_end_region(ctx);
                                        0;
                                        })));
                uw_end_region(ctx);
                 ({
                  uw_unit
                  tmp;
                  uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:14:29-14:57: %s", 
                  "refused");
                  tmp;
                  });
                });
               __uwr = uw_malloc(ctx, sizeof(uw_unit));
               *__uwr = __uwv;
               uw_io_transaction_commit(ctx, &__uwt);
               break;
               } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
              __uwr = NULL;
               break;
               }
              }
             uw_io_transaction_end(ctx, &__uwt, 0);
             __uwr;
             });
            
            disc != NULL && 1 ?
             ({uw_unit __uwr_v_2 = (*disc);
                ({
                 struct __uwd_result_s_3 *tmp =
                 uw_malloc(ctx, sizeof(struct __uwd_result_s_3));
                 tmp->tag =
                 __uwc_Success_4;
                 tmp->data.uw_Success = __uwr_v_2;
                  tmp;
                 });
              })
              :
             disc == NULL ?
              ({
               struct __uwd_result_s_3 *tmp =
               uw_malloc(ctx, sizeof(struct __uwd_result_s_3));
               tmp->tag =
               __uwc_Failure_5;
               tmp->data.uw_Failure =
                uw_Basis_htmlifyString(ctx, uw_Basis_io_errorMessage(ctx));
                tmp;
               })
               :
              ({
               struct __uwd_result_s_3*
               tmp;
               uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:13:4-16:55: pattern match failure");
               tmp;
               });
            });
           ({
            uw_unit __uwr___3 =
            (uw_begin_region(ctx), uw_Basis_debug(ctx,
                                    "handled: the function returns"));
            uw_end_region(ctx);
             ({
              struct __uwd_result_s_3* disc =
              __uwr_r_2;
              
              disc->tag == __uwc_Success_4 && 1 ? 1LL
                :
               disc->tag == __uwc_Failure_5 && 1 ?
                ({uw_Basis_string __uwr___4 = disc->data.uw_Failure;
                   0LL;
                 })
                 :
                ({
                 uw_Basis_int
                 tmp;
                 uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:15:4-16:55: pattern match failure");
                 tmp;
                 });
              });
            });
           }));
   }
  
  static uw_Basis_string
   __uwn_rolledBack_7(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_string arg0 =
            ({
             uw_unit* disc =
             ({
              uw_io_transaction __uwt;
              uw_unit
              *__uwr = NULL;
              uw_set_could_write_db(ctx, 0);
              uw_io_transaction_begin(ctx, &__uwt);
              while (1) {
              int __uwfk = setjmp(*uw_jmp_buf(ctx));
               if (__uwfk == 0) {
               uw_unit __uwv =
                ({
                 uw_unit __uwr___2 =
                 (uw_begin_region(ctx), ({
                                         uw_Basis_string arg0 = "/";
                                          
                                          uw_Basis_string arg1 =
                                           "Io_rpc_attempts/c";
                                           
                                          uw_Basis_string arg2 =
                                           "set+in+a+rolled.2Dback+transaction";
                                           uw_Basis_time* arg3 = NULL;
                                            
                                          uw_Basis_bool arg4 = uw_Basis_False;
                                           uw_Basis_bool arg5 = uw_Basis_False;
                                            uw_Basis_set_cookie(ctx, arg0, 
                                                                      arg1, 
                                                                      arg2, 
                                                                      arg3, 
                                                                      arg4, 
                                                                      arg5);
                                         }));
                 uw_end_region(ctx);
                  ({
                   uw_unit __uwr___3 =
                   (uw_begin_region(ctx), ({
                                           uw_Basis_string arg0 = "X-Io";
                                            
                                            uw_Basis_string arg1 =
                                             "rolled back";
                                             uw_Basis_setHeader(ctx, arg0, arg1);
                                           }));
                   uw_end_region(ctx);
                    ({
                     uw_unit
                     tmp;
                     uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:24:30-24:58: %s", 
                     "refused");
                     tmp;
                     });
                   });
                 });
                __uwr = uw_malloc(ctx, sizeof(uw_unit));
                *__uwr = __uwv;
                uw_io_transaction_commit(ctx, &__uwt);
                break;
                } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
               __uwr = NULL;
                break;
                }
               }
              uw_io_transaction_end(ctx, &__uwt, 0);
              __uwr;
              });
             
             disc == NULL ? "None"
               :
              disc != NULL && 1 ? "Some"
                :
               ({
                uw_Basis_string
                tmp;
                uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:26:12-26:58: pattern match failure");
                tmp;
                });
             });
            uw_Basis_string arg1 = ", cookie seen afterwards: ";
             
            uw_Basis_string arg2 =
             ({
              uw_Basis_string disc =
              ({
               uw_io_transaction __uwt;
               uw_Basis_string
               __uwr;
               uw_set_could_write_db(ctx, 0);
               uw_io_transaction_begin(ctx, &__uwt);
               while (1) {
               int __uwfk = setjmp(*uw_jmp_buf(ctx));
                if (__uwfk == 0) {
                __uwr =
                 ({
                  uw_Basis_string request = uw_maybe_strdup(ctx, uw_Basis_get_cookie(ctx,
                                                                  "Io_rpc_attempts/c"
                                                                  ));
                  
                  (request ? uw_Basis_unurlifyString(ctx, &request) : NULL);
                  });
                 uw_io_transaction_commit(ctx, &__uwt);
                 break;
                 } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                break;
                 }
                }
               uw_io_transaction_end(ctx, &__uwt, 1);
               __uwr;
               });
              
              disc == NULL ? "none"
                :
               disc != NULL && 1 ?
                ({uw_Basis_string __uwr_v_2 = disc;
                   __uwr_v_2;
                 })
                 :
                ({
                 uw_Basis_string
                 tmp;
                 uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:27:14-27:54: pattern match failure");
                 tmp;
                 });
              });
             uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
           }));
   }
  
  static uw_Basis_int
   __uwn_retried_8(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_io_transaction __uwt;
           uw_Basis_int
           __uwr;
           uw_set_could_write_db(ctx, 1);
           uw_io_transaction_begin(ctx, &__uwt);
           while (1) {
           int __uwfk = setjmp(*uw_jmp_buf(ctx));
            if (__uwfk == 0) {
            __uwr =
             ({
              uw_unit __uwr___2 =
              (uw_begin_region(ctx), ({
                                      uw_Basis_string arg0 = "/";
                                       
                                       uw_Basis_string arg1 =
                                        "Io_rpc_attempts/c";
                                        
                                       uw_Basis_string arg2 =
                                        uw_Basis_urlifyString(ctx,
                                         ({
                                          uw_Basis_string arg0 =
                                           "set in attempt ";
                                           
                                           uw_Basis_string arg1 =
                                            uw_Basis_intToString(ctx,
                                             uw_Probe_stepT(ctx));
                                            uw_Basis_strcat(ctx, arg0, arg1);
                                          }));
                                        uw_Basis_time* arg3 = NULL;
                                         uw_Basis_bool arg4 = uw_Basis_False;
                                          uw_Basis_bool arg5 = uw_Basis_False;
                                           uw_Basis_set_cookie(ctx, arg0, arg1, 
                                                                     arg2, 
                                                                     arg3, 
                                                                     arg4, arg5);
                                      }));
              uw_end_region(ctx);
               ({
                uw_unit __uwr___3 =
                (uw_begin_region(ctx), uw_Probe_failOnce(ctx, "io"));
                uw_end_region(ctx);
                 ({
                  uw_unit __uwr___4 =
                  (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                          uw_Basis_int arg1 = __uwr_n_0;
                                           
                                           uw_ensure_transaction(ctx);
                                           
                                           uw_conn *conn = uw_get_db(ctx);
                                            sqlite3_stmt *stmt = conn->p0;
                                            
                                            if (stmt == NULL) {
                                            if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                             conn->p0 = stmt;
                                             }
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                            if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:36:20-37:28: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                             
                                            int r;
                                             if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                             uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                              }
                                             
                                             if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:36:20-37:28: DML step failed: %s<br />%s", 
                                                                    "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                    else if (uw_dml_log_enabled()) {
                                             char *expanded = sqlite3_expanded_sql(stmt);
                                              uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:36:20-37:28", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                              sqlite3_free(expanded);
                                              }
                                             uw_pop_cleanup(ctx);
                                            uw_pop_cleanup(ctx);
                                            
                                          
                                          uw_end_region(ctx);
                                          0;
                                          })));
                  uw_end_region(ctx);
                   __uwr_n_0;
                  });
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
   }
  
  static uw_Basis_int
   __uwn_retriedT_9(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 = "/";
                                    uw_Basis_string arg1 = "Io_rpc_attempts/c";
                                     
                                    uw_Basis_string arg2 =
                                     uw_Basis_urlifyString(ctx,
                                      ({
                                       uw_Basis_string arg0 =
                                        "set in attempt ";
                                        
                                        uw_Basis_string arg1 =
                                         uw_Basis_intToString(ctx,
                                          uw_Probe_stepT(ctx));
                                         uw_Basis_strcat(ctx, arg0, arg1);
                                       }));
                                     uw_Basis_time* arg3 = NULL;
                                      uw_Basis_bool arg4 = uw_Basis_False;
                                       uw_Basis_bool arg5 = uw_Basis_False;
                                        uw_Basis_set_cookie(ctx, arg0, arg1, 
                                                                  arg2, arg3, 
                                                                  arg4, arg5);
                                   }));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), uw_Probe_failOnce(ctx, "transaction"));
             uw_end_region(ctx);
              ({
               uw_unit __uwr___4 =
               (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                       uw_Basis_int arg1 = __uwr_n_0;
                                        
                                        uw_ensure_transaction(ctx);
                                        
                                        uw_conn *conn = uw_get_db(ctx);
                                         sqlite3_stmt *stmt = conn->p0;
                                         
                                         if (stmt == NULL) {
                                         if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                          conn->p0 = stmt;
                                          }
                                         uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                         uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                         if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:45:4-46:12: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                          
                                         int r;
                                          if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                          uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                           }
                                          
                                          if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:45:4-46:12: DML step failed: %s<br />%s", 
                                                                 "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                 else if (uw_dml_log_enabled()) {
                                          char *expanded = sqlite3_expanded_sql(stmt);
                                           uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:45:4-46:12", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                           sqlite3_free(expanded);
                                           }
                                          uw_pop_cleanup(ctx);
                                         uw_pop_cleanup(ctx);
                                         
                                       
                                       uw_end_region(ctx);
                                       0;
                                       })));
               uw_end_region(ctx);
                __uwr_n_0;
               });
             });
           }));
   }
  
  static uw_Basis_int
   __uwn_committedThenFatal_10(uw_context ctx, uw_Basis_int __uwr_n_0, 
                                uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
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
                                     ({
                                      uw_unit __uwr___2 =
                                      (uw_begin_region(ctx), ({
                                                              uw_Basis_string
                                                               arg0 = "/";
                                                               
                                                               uw_Basis_string
                                                                arg1 =
                                                                "Io_rpc_attempts/c";
                                                                
                                                               uw_Basis_string
                                                                arg2 =
                                                                "set+before+the+failure";
                                                                
                                                               uw_Basis_time*
                                                                arg3 = NULL;
                                                                
                                                               uw_Basis_bool
                                                                arg4 =
                                                                uw_Basis_False;
                                                                
                                                               uw_Basis_bool
                                                                arg5 =
                                                                uw_Basis_False;
                                                                uw_Basis_set_cookie(ctx, 
                                                              arg0, arg1, arg2, 
                                                               arg3, arg4, arg5);
                                                              }));
                                      uw_end_region(ctx);
                                       (uw_begin_region(ctx), ({
                                        uw_Basis_int arg1 = __uwr_n_0;
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p0;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p0 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:51:20-53:58: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                           
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:51:20-53:58: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:51:20-53:58", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                            sqlite3_free(expanded);
                                            }
                                           uw_pop_cleanup(ctx);
                                          uw_pop_cleanup(ctx);
                                          
                                        
                                        uw_end_region(ctx);
                                        0;
                                        }));
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
             uw_Basis_int
             __uwr;
             uw_set_could_write_db(ctx, 0);
             uw_io_transaction_begin(ctx, &__uwt);
             while (1) {
             int __uwfk = setjmp(*uw_jmp_buf(ctx));
              if (__uwfk == 0) {
              __uwr =
               ({
                uw_Basis_int
                tmp;
                uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:54:19-54:61: %s", 
                "failed after the cookie");
                tmp;
                });
               uw_io_transaction_commit(ctx, &__uwt);
               break;
               } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
              break;
               }
              }
             uw_io_transaction_end(ctx, &__uwt, 1);
             __uwr;
             });
           }));
   }
  
  static uw_Basis_string
   __uwn_sources_11(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit* __uwr___2 =
           ({
            uw_io_transaction __uwt;
            uw_unit
            *__uwr = NULL;
            uw_set_could_write_db(ctx, 0);
            uw_io_transaction_begin(ctx, &__uwt);
            while (1) {
            int __uwfk = setjmp(*uw_jmp_buf(ctx));
             if (__uwfk == 0) {
             uw_unit __uwv =
              ({
               uw_Basis_source __uwr_s_2 =
               uw_Basis_new_client_source(ctx,
                ({
                 uw_Basis_string arg0 = "{c:\"c\",v:";
                  uw_Basis_string arg1 = uw_Basis_htmlifyInt(ctx, __uwr_n_0);
                   uw_Basis_string arg2 = "}";
                    uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                 }));
               ({
                uw_unit
                tmp;
                uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:60:30-60:58: %s", 
                "refused");
                tmp;
                });
               });
              __uwr = uw_malloc(ctx, sizeof(uw_unit));
              *__uwr = __uwv;
              uw_io_transaction_commit(ctx, &__uwt);
              break;
              } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
             __uwr = NULL;
              break;
              }
             }
            uw_io_transaction_end(ctx, &__uwt, 0);
            __uwr;
            });
           ({
            uw_Basis_string arg0 =
             "<script type=\"text/javascript\">dyn(\"span\", execD({c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:";
             
             uw_Basis_string arg1 =
              uw_Basis_htmlifySource(ctx,
               ({
                uw_io_transaction __uwt;
                uw_Basis_source
                __uwr;
                uw_set_could_write_db(ctx, 0);
                uw_io_transaction_begin(ctx, &__uwt);
                while (1) {
                int __uwfk = setjmp(*uw_jmp_buf(ctx));
                 if (__uwfk == 0) {
                 __uwr =
                  uw_Basis_new_client_source(ctx,
                   ({
                    uw_Basis_string arg0 = "{c:\"c\",v:";
                     
                     uw_Basis_string arg1 =
                      uw_Basis_htmlifyInt(ctx, __uwr_n_0 + 1LL);
                      uw_Basis_string arg2 = "}";
                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
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
              uw_Basis_string arg2 = "}},x:{c:\"c\",v:null}}))</script>";
               uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
            });
           }));
   }
  
  static uw_Basis_int
   __uwn_outside_12(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), uw_Probe_registerIo(ctx, "outside"));
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
                                       ({
                                        uw_unit __uwr___3 =
                                        (uw_begin_region(ctx), uw_Probe_register(ctx,
                                                                "inside"));
                                        uw_end_region(ctx);
                                         (uw_begin_region(ctx), ({
                                          uw_Basis_int arg1 = __uwr_n_0;
                                           
                                           uw_ensure_transaction(ctx);
                                           
                                           uw_conn *conn = uw_get_db(ctx);
                                            sqlite3_stmt *stmt = conn->p0;
                                            
                                            if (stmt == NULL) {
                                            if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                             conn->p0 = stmt;
                                             }
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                            if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:68:20-68:83: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                             
                                            int r;
                                             if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                             uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                              }
                                             
                                             if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:68:20-68:83: DML step failed: %s<br />%s", 
                                                                    "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                    else if (uw_dml_log_enabled()) {
                                             char *expanded = sqlite3_expanded_sql(stmt);
                                              uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:68:20-68:83", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                              sqlite3_free(expanded);
                                              }
                                             uw_pop_cleanup(ctx);
                                            uw_pop_cleanup(ctx);
                                            
                                          
                                          uw_end_region(ctx);
                                          0;
                                          }));
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
              __uwr_n_0;
             });
           }));
   }
  
  static uw_Basis_int
   __uwn_pure_13(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), uw_Basis_debug(ctx, "pure: no transaction"));
           uw_end_region(ctx);
            __uwr_n_0 + 1LL;
           }));
   }
  
  static uw_Basis_int
   __uwn_fatalOutside_14(uw_context ctx, uw_Basis_int __uwr_n_0, 
                          uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
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
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p0;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p0 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:78:4-78:59: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:78:4-78:59: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:78:4-78:59", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                          sqlite3_free(expanded);
                                          }
                                         uw_pop_cleanup(ctx);
                                        uw_pop_cleanup(ctx);
                                        
                                      
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
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), uw_Probe_fatalIo(ctx,
                                     "fatal outside any transaction"));
             uw_end_region(ctx);
              __uwr_n_0;
             });
           }));
   }
  
  static uw_Basis_int
   __uwn_redirecting_15(uw_context ctx, uw_Basis_int __uwr_n_0, 
                         uw_unit __uwr___1)
   {
   return(({
           uw_io_transaction __uwt;
           uw_Basis_int
           __uwr;
           uw_set_could_write_db(ctx, 1);
           uw_io_transaction_begin(ctx, &__uwt);
           while (1) {
           int __uwfk = setjmp(*uw_jmp_buf(ctx));
            if (__uwfk == 0) {
            __uwr =
             ({
              uw_unit __uwr___2 =
              (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                      uw_Basis_int arg1 = __uwr_n_0;
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p0;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p0 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:85:20-86:60: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:85:20-86:60: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Io_rpc_attempts_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_rpc_attempts/io_rpc_attempts.ur:85:20-86:60", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                          sqlite3_free(expanded);
                                          }
                                         uw_pop_cleanup(ctx);
                                        uw_pop_cleanup(ctx);
                                        
                                      
                                      uw_end_region(ctx);
                                      0;
                                      })));
              uw_end_region(ctx);
               ({
                uw_Basis_int
                tmp;
                uw_redirect(ctx, "/Io_rpc_attempts/rows");
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
   }
  
  static uw_Basis_int
   __uwn_pageWritten_16(uw_context ctx, uw_Basis_int __uwr_n_0, 
                         uw_unit __uwr___1)
   {
   return(({
           uw_unit* __uwr___2 =
           ({
            uw_io_transaction __uwt;
            uw_unit
            *__uwr = NULL;
            uw_set_could_write_db(ctx, 0);
            uw_io_transaction_begin(ctx, &__uwt);
            while (1) {
            int __uwfk = setjmp(*uw_jmp_buf(ctx));
             if (__uwfk == 0) {
             uw_unit __uwv =
              ({
               uw_unit __uwr___2 =
               (uw_begin_region(ctx), uw_Probe_pageWriteT(ctx,
                                       "<written in a rolled-back attempt>"));
               uw_end_region(ctx);
                ({
                 uw_unit
                 tmp;
                 uw_error(ctx, FATAL, "tests/io_rpc_attempts/io_rpc_attempts.ur:93:30-93:58: %s", 
                 "refused");
                 tmp;
                 });
               });
              __uwr = uw_malloc(ctx, sizeof(uw_unit));
              *__uwr = __uwv;
              uw_io_transaction_commit(ctx, &__uwt);
              break;
              } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
             __uwr = NULL;
              break;
              }
             }
            uw_io_transaction_end(ctx, &__uwt, 0);
            __uwr;
            });
           __uwr_n_0;
           }));
   }
  
  static uw_Basis_int
   __uwn_leavesOpen_17(uw_context ctx, uw_Basis_int __uwr_n_0, 
                        uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), uw_Probe_beginIo(ctx));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), uw_Probe_armConflictIo(ctx, "leavesOpen"));
             uw_end_region(ctx);
              ({
               uw_Basis_int __uwr_k_4 =
               (uw_begin_region(ctx), uw_Probe_step(ctx));
               uw_end_region(ctx);
                ({
                 uw_unit __uwr___5 =
                 (uw_begin_region(ctx), uw_Basis_debug(ctx,
                                         ({
                                          uw_Basis_string arg0 =
                                           "leavesOpen runs, step ";
                                           
                                           uw_Basis_string arg1 =
                                            uw_Basis_intToString(ctx,
                                             __uwr_k_4);
                                            uw_Basis_strcat(ctx, arg0, arg1);
                                          })));
                 uw_end_region(ctx);
                  __uwr_k_4;
                 });
               });
             });
           }));
   }
  
  static struct __uws_2* __uwn_$revPRIME_unpoly_18(uw_context,
   struct __uws_2*, struct __uws_2*);
   static struct __uws_2*
    __uwn_$revPRIME_unpoly_18(uw_context ctx, struct __uws_2* __uwr_acc_0, 
                               struct __uws_2* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_1 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_2* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_2* rearg0 =
                    ({
                     struct __uws_2 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_2));
                     *tmp =
                     ({ struct __uws_2 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_2* rearg1 = __uwr_ls_3;
                   struct __uws_2*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_2*
               tmp;
               uw_error(ctx, FATAL, "$/top.ur:279:21-282:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_$mapXPRIME_unpoly_unpoly_19(uw_context,
   struct __uws_2*);
   static uw_unit
    __uwn_$mapXPRIME_unpoly_unpoly_19(uw_context ctx, 
                                       struct __uws_2* __uwr_ls_0)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_0;
            
            disc == NULL ? 0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_1 __uwr_x_1 = (*disc).__uwf_1;
                 struct __uws_2* __uwr_ls_2 = (*disc).__uwf_2;
                  ((uw_write(ctx, " "), 0),
                   (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                           __uwr_x_1.__uwf_N),
                    uw_end_region(ctx), ({
                                         struct __uws_2* rearg0 = __uwr_ls_2;
                                         uw_unit
                                         tmp;
                                         __uwr_ls_0 = rearg0;
                                         goto restart;
                                         tmp;
                                         })));
               })
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_wrap_rows_20(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">stored:"), 0),
                                                      (uw_begin_region(ctx), __uwn_$mapXPRIME_unpoly_unpoly_19(ctx,
                                                                              ({
                                                                               struct
                                                                               __uws_2*
                                                                               arg0
                                                                               =
                                                                               NULL;
                                                                               
                                                                               struct
                                                                               __uws_2*
                                                                               arg1
                                                                               =
                                                                               ({
                                                                               struct
                                                                               __uws_2*
                                                                               acc
                                                                               =
                                                                               NULL;
                                                                               int dummy = (uw_begin_region(ctx), 0);
                                                                               uw_ensure_transaction(ctx);
                                                                               
                                                                               
                                                                               uw_conn *conn = uw_get_db(ctx);
                                                                               sqlite3_stmt *stmt = conn->p1;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N FROM uw_Io_rpc_attempts_t AS T_T ORDER BY T_T.uw_N", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_N FROM uw_Io_rpc_attempts_t AS T_T ORDER BY T_T.uw_N<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p1 = stmt;
                                                                               }
                                                                               
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                               
                                                                               
                                                                               int r;
                                                                               sqlite3_reset(stmt);
                                                                               uw_end_region(ctx);
                                                                               while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                                                               struct __uws_3 __uwr_r_2;
                                                                               struct
                                                                               __uws_2*
                                                                               __uwr_acc_3
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_N
                                                                               =
                                                                               (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:292:16-292:18: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               sqlite3_column_int64(stmt, 0));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               ({
                                                                               struct
                                                                               __uws_2
                                                                               *tmp
                                                                               =
                                                                               uw_malloc(ctx, sizeof(
                                                                               struct
                                                                               __uws_2));
                                                                               *tmp
                                                                               =
                                                                               ({
                                                                               struct
                                                                               __uws_2
                                                                               tmp
                                                                               =
                                                                               {__uwr_r_2.__uwf_T
                                                                               ,
                                                                               
                                                                               __uwr_acc_3};
                                                                               tmp;
                                                                               });
                                                                               tmp;
                                                                               });
                                                                               }
                                                                               
                                                                               if (r == SQLITE_BUSY) {
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                               }
                                                                               
                                                                               if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:292:16-292:18: query step failed: %s<br />%s", 
                                                                               "SELECT T_T.uw_N FROM uw_Io_rpc_attempts_t AS T_T ORDER BY T_T.uw_N", sqlite3_errmsg(conn->conn));
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               acc;
                                                                               })
                                                                               ;
                                                                               __uwn_$revPRIME_unpoly_18(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_main_21(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_s_2 =
           uw_Basis_new_client_source(ctx,
            ({
             uw_Basis_string arg0 = "{c:\"c\",v:";
              uw_Basis_string arg1 = uw_Basis_htmlifyInt(ctx, 0LL);
               uw_Basis_string arg2 = "}";
                uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
             }));
           ({
            uw_Basis_source __uwr_x_3 =
            uw_Basis_new_client_source(ctx,
             ({
              uw_Basis_string arg0 = "{c:\"c\",v:";
               uw_Basis_string arg1 = uw_Basis_jsifyString(ctx, "");
                uw_Basis_string arg2 = "}";
                 uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
              }));
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">\n<button onclick='uw_event=event;exec("), 0),
                                                        (uw_begin_region(ctx), ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                         uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ")'>handled</button>\n<button onclick='uw_event=event;exec({c:\"a\",f:{c:\"n\",n:3},x:{c:\"c\",v:null}})'>rolledBack</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                              (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:4},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>retried</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:5},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>retriedT</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:6},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>committedThenFatal</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:7},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_x_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>sources</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:8},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>outside</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:9},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>pure</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:10},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>fatalOutside</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:11},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>redirecting</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:12},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>pageWritten</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:13},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>leavesOpen</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:14},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "))</script>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:15},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_x_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))))))))))))))))))))))));
            });
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Io_rpc_attempts/main")) return 1;
    if (!strcmp(s, "/Io_rpc_attempts/rows")) return 1;
     return 0;
   }
  
 static int uw_check_mime(const char *s) {
  return 0;
   }
  
 static int uw_check_requestHeader(const char *s) {
  return 0;
   }
  
 static int uw_check_responseHeader(const char *s) {
  if (!strcmp(s, "X-Io")) return 1;
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
  uw_sign(uw_unnull(uw_Basis_get_cookie(ctx, "Io_rpc_attempts/c")), r);
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
  
  
  if (!strcmp(request, "/app.195AB39FD9826FF7C8A09AA4871AEC31328E11B4.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Io_rpc_attempts/main", 21) && (request[21] == 0 || request[21] == '/')) {
  request += 21;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.195AB39FD9826FF7C8A09AA4871AEC31328E11B4.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_21(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    uw_finish_page(ctx);
    return;
   }
   }
  
  if (!strncmp(request, "/Io_rpc_attempts/rows", 21) && (request[21] == 0 || request[21] == '/')) {
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
     __uwn_wrap_rows_20(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     uw_finish_page(ctx);
     return;
    }
    }
  
  if (!strncmp(request, "/Io_rpc_attempts/handled", 24) && (request[24] == 0 || request[24] == '/')) {
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
      __uwn_handled_6(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/rolledBack", 27) && (request[27] == 0 || request[27] == '/')) {
   request += 27;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
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
     uw_Basis_string it0 =
      __uwn_rolledBack_7(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      uw_Basis_urlifyString_w(ctx, it0);
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_rpc_attempts/retried", 24) && (request[24] == 0 || request[24] == '/')) {
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
      __uwn_retried_8(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/retriedT", 25) && (request[25] == 0 || request[25] == '/')) {
   request += 25;
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
     uw_Basis_int it0 = __uwn_retriedT_9(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     uw_Basis_urlifyInt_w(ctx, it0);
      return;
    }
    }
  
  if (!strncmp(request, "/Io_rpc_attempts/committedThenFatal", 35) && (request[35] == 0 || request[35] == '/')) {
   request += 35;
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
      __uwn_committedThenFatal_10(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/sources", 24) && (request[24] == 0 || request[24] == '/')) {
   request += 24;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
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
     uw_Basis_string it0 =
      __uwn_sources_11(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      uw_Basis_urlifyString_w(ctx, it0);
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_rpc_attempts/outside", 24) && (request[24] == 0 || request[24] == '/')) {
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
      __uwn_outside_12(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/pure", 21) && (request[21] == 0 || request[21] == '/')) {
   request += 21;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
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
      __uwn_pure_13(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/fatalOutside", 29) && (request[29] == 0 || request[29] == '/')) {
   request += 29;
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
      __uwn_fatalOutside_14(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/redirecting", 28) && (request[28] == 0 || request[28] == '/')) {
   request += 28;
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
      __uwn_redirecting_15(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/pageWritten", 28) && (request[28] == 0 || request[28] == '/')) {
   request += 28;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
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
      __uwn_pageWritten_16(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_rpc_attempts/leavesOpen", 27) && (request[27] == 0 || request[27] == '/')) {
   request += 27;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
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
      __uwn_leavesOpen_17(ctx, arg0, 0);
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
                                                       1};
 