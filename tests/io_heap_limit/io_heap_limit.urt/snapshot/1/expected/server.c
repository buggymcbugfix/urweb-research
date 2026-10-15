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
  uw_heap_max = 300000000;
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
     uw_sqlsuffixString = "";
     uw_sqlsuffixChar = "";
     uw_sqlsuffixBlob = "";
     uw_sqlfmtUint4 = "%u%n";
     }
    
    static void uw_db_validate(uw_context ctx) {
     uw_conn *conn = uw_get_db(ctx);
     sqlite3_stmt *stmt;
     int res;
     
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_heap_limit_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Query preparation failed:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_heap_limit_t' COLLATE NOCASE");
       }
      
      while ((res = sqlite3_step(stmt)) == SQLITE_BUSY)
      sleep(1);
       
      if (res == SQLITE_DONE) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "No row returned:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_heap_limit_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Error getting row:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_heap_limit_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Bad column count:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_heap_limit_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Table 'uw_Io_heap_limit_t' does not exist.");
       }
      
      sqlite3_finalize(stmt);
      }
     
     static void uw_db_prepare(uw_context ctx) {
     uw_conn *conn = uw_get_db(ctx);
     
     if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_heap_limit_t (uw_N) VALUES (\?)", -1, &conn->p0, NULL) != SQLITE_OK) {
      char msg[1024];
       strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
       msg[1023] = 0;
       sqlite3_close(conn->conn);
        uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_heap_limit_t (uw_N) VALUES (\?)<br />%s"
        , msg);
        }
      
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N FROM uw_Io_heap_limit_t AS T_T ORDER BY T_T.uw_N", -1, &conn->p1, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_N FROM uw_Io_heap_limit_t AS T_T ORDER BY T_T.uw_N<br />%s"
         , msg);
         }
       }
    
    static void uw_db_init(uw_context ctx) {
    sqlite3 *sqlite;
    sqlite3_stmt *stmt;
    uw_conn *conn;
    
    char *env_db_str = getenv("URWEB_SQLITE_DB_PATH");
    const char *sqlite_db_path = env_db_str == NULL ? "io_heap_limit.db" : env_db_str;
    if (sqlite3_open_v2(sqlite_db_path, &sqlite, SQLITE_OPEN_READWRITE, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Can't open SQLite database: %s", sqlite_db_path);
    
    if (sqlite3_exec(sqlite, "PRAGMA foreign_keys = ON", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't enable foreign_keys for SQLite database");
     
    if (sqlite3_exec(sqlite, "PRAGMA synchronous = FULL", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't enable `PRAGMA synchronous = FULL` for SQLite database");
     
    if (sqlite3_exec(sqlite, "PRAGMA mmap_size = 268435456", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't set `PRAGMA mmap_size` for SQLite database");
     
    if (sqlite3_exec(sqlite, "PRAGMA busy_timeout = 10000", NULL, NULL, NULL) != SQLITE_OK)
    uw_error(ctx, FATAL, "Can't set `PRAGMA busy_timeout` for SQLite database");
     
    if (uw_database_max < SIZE_MAX) {
    char buf[100];
     
     sprintf(buf, "PRAGMA max_page_count = %llu", (unsigned long long)(uw_database_max / 1024));
     
     if (sqlite3_prepare_v2(sqlite, buf, -1, &stmt, NULL) != SQLITE_OK) {
     sqlite3_close(sqlite);
      uw_error(ctx, FATAL, "Can't prepare max_page_count query for SQLite database");
      }
     
     if (sqlite3_step(stmt) != SQLITE_ROW) {
     sqlite3_finalize(stmt);
      sqlite3_close(sqlite);
      uw_error(ctx, FATAL, "Can't set max_page_count parameter for SQLite database");
      }
     
     sqlite3_finalize(stmt);
     }
    
    conn = calloc(1, sizeof(uw_conn));
    conn->conn = sqlite;
    uw_set_db(ctx, conn);
    uw_db_validate(ctx);
    uw_db_prepare(ctx);
    }
    
    static void uw_db_close(uw_context ctx) {
    uw_conn *conn = uw_get_db(ctx);
    if (conn->p0) sqlite3_finalize(conn->p0);
     if (conn->p1) sqlite3_finalize(conn->p1);
      sqlite3_close(conn->conn);
    }
    
    static int uw_db_begin(uw_context ctx, int could_write) {
    uw_conn *conn = uw_get_db(ctx);
    
    if (sqlite3_exec(conn->conn, "BEGIN", NULL, NULL, NULL) == SQLITE_OK)
    return 0;
     else {
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
    if (sqlite3_exec(conn->conn, "ROLLBACK", NULL, NULL, NULL) == SQLITE_OK)
    return 0;
     else {
    fprintf(stderr, "Rollback error: %s<br />", sqlite3_errmsg(conn->conn));
     return 1;
     }
    }
    
    
 
 /* No global setup for LRU cache. */
  
 
  
  struct __uws_1 {
   uw_Basis_int __uwf_N;
    };
  struct __uws_2 {
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
  
  /* SQL table uw_Io_heap_limit_t constraints   */
   
  
  static uw_Basis_string __uwn_grow_3(uw_context,
   uw_Basis_string, uw_Basis_int);
   static uw_Basis_string
    __uwn_grow_3(uw_context ctx, uw_Basis_string __uwr_s_0, 
                  uw_Basis_int __uwr_k_1)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            __uwr_k_1 == 0LL;
            
            disc == uw_Basis_True ? __uwr_s_0
              :
             disc == uw_Basis_False ?
              ({
               uw_Basis_string rearg0 =
                ({
                 uw_Basis_string arg0 = __uwr_s_0;
                  uw_Basis_string arg1 = __uwr_s_0;
                   uw_Basis_strcat(ctx, arg0, arg1);
                 });
                uw_Basis_int rearg1 = __uwr_k_1 - 1LL;
               uw_Basis_string
               tmp;
               __uwr_s_0 = rearg0;
                __uwr_k_1 = rearg1;
               goto restart;
               tmp;
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:14:43-14:84: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int
   __uwn_outside_4(uw_context ctx, uw_Basis_int __uwr_k_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_n_2 =
           (uw_begin_region(ctx), uw_Basis_strlen(ctx,
                                   ({
                                    uw_Basis_string arg0 = "x";
                                     uw_Basis_int arg1 = __uwr_k_0;
                                    __uwn_grow_3(ctx, arg0, arg1);
                                    })));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), uw_Basis_debug(ctx,
                                     ({
                                      uw_Basis_string arg0 = "outside: built ";
                                       
                                       uw_Basis_string arg1 =
                                        uw_Basis_intToString(ctx, __uwr_n_2);
                                        uw_Basis_strcat(ctx, arg0, arg1);
                                      })));
             uw_end_region(ctx);
              __uwr_n_2;
             });
           }));
   }
  
  static uw_Basis_int
   __uwn_inside_5(uw_context ctx, uw_Basis_int __uwr_k_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_n_2 =
           (uw_begin_region(ctx), ({
                                   uw_io_transaction __uwt;
                                   uw_Basis_int
                                   __uwr;
                                   uw_io_transaction_begin(ctx, &__uwt);
                                   while (1) {
                                   int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                    if (__uwfk == 0) {
                                    __uwr =
                                     uw_Basis_strlen(ctx,
                                      ({
                                       uw_Basis_string arg0 = "x";
                                        uw_Basis_int arg1 = __uwr_k_0;
                                       __uwn_grow_3(ctx, arg0, arg1);
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
             (uw_begin_region(ctx), uw_Basis_debug(ctx,
                                     ({
                                      uw_Basis_string arg0 = "inside: built ";
                                       
                                       uw_Basis_string arg1 =
                                        uw_Basis_intToString(ctx, __uwr_n_2);
                                        uw_Basis_strcat(ctx, arg0, arg1);
                                      })));
             uw_end_region(ctx);
              __uwr_n_2;
             });
           }));
   }
  
  static uw_Basis_int
   __uwn_handled_6(uw_context ctx, uw_Basis_int __uwr_k_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int* __uwr_r_2 =
           ({
            uw_io_transaction __uwt;
            uw_Basis_int
            *__uwr = NULL;
            uw_io_transaction_begin(ctx, &__uwt);
            while (1) {
            int __uwfk = setjmp(*uw_jmp_buf(ctx));
             if (__uwfk == 0) {
             uw_Basis_int __uwv =
              uw_Basis_strlen(ctx,
               ({
                uw_Basis_string arg0 = "x";
                 uw_Basis_int arg1 = __uwr_k_0;
                __uwn_grow_3(ctx, arg0, arg1);
                }));
              __uwr = uw_malloc(ctx, sizeof(uw_Basis_int));
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
            uw_unit __uwr___3 =
            (uw_begin_region(ctx), ({
                                    uw_Basis_int* disc =
                                    __uwr_r_2;
                                    
                                    disc == NULL ?
                                     uw_Basis_debug(ctx,
                                      ({
                                       uw_Basis_string arg0 = "handled: ";
                                        
                                        uw_Basis_string arg1 =
                                         uw_Basis_io_errorMessage(ctx);
                                         uw_Basis_strcat(ctx, arg0, arg1);
                                       }))
                                      :
                                     disc != NULL && 1 ?
                                      ({uw_Basis_int __uwr_n_3 = (*disc);
                                         uw_Basis_debug(ctx,
                                          ({
                                           uw_Basis_string arg0 =
                                            "handled: built ";
                                            
                                            uw_Basis_string arg1 =
                                             uw_Basis_intToString(ctx,
                                              __uwr_n_3);
                                             uw_Basis_strcat(ctx, arg0, arg1);
                                           }));
                                       })
                                       :
                                      ({
                                       uw_unit
                                       tmp;
                                       uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:28:4-32:47: pattern match failure");
                                       tmp;
                                       });
                                    }));
            uw_end_region(ctx);
             ({
              uw_unit __uwr___4 =
              (uw_begin_region(ctx), ({
                                      uw_io_transaction __uwt;
                                      uw_unit
                                      __uwr;
                                      uw_io_transaction_begin(ctx, &__uwt);
                                      while (1) {
                                      int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                       if (__uwfk == 0) {
                                       __uwr =
                                        (uw_begin_region(ctx), ({
                                         uw_Basis_int arg1 = __uwr_k_0;
                                          
                                          uw_ensure_transaction(ctx);
                                          
                                          uw_conn *conn = uw_get_db(ctx);
                                           sqlite3_stmt *stmt = conn->p0;
                                           
                                           if (stmt == NULL) {
                                           if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_heap_limit_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_heap_limit_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                            conn->p0 = stmt;
                                            }
                                           uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                           uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                           if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:31:4-31:59: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                            
                                           int r;
                                            if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                            sleep(1);
                                             uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                             }
                                            
                                            if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:31:4-31:59: DML step failed: %s<br />%s", 
                                                                   "INSERT INTO uw_Io_heap_limit_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                   else if (uw_dml_log_enabled()) {
                                            char *expanded = sqlite3_expanded_sql(stmt);
                                             uw_log_dml(ctx, "tests/io_heap_limit/io_heap_limit.ur:31:4-31:59", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                uw_Basis_int* disc =
                __uwr_r_2;
                
                disc == NULL ? - 1LL
                  :
                 disc != NULL && 1 ?
                  ({uw_Basis_int __uwr_n_5 = (*disc);
                     __uwr_n_5;
                   })
                   :
                  ({
                   uw_Basis_int
                   tmp;
                   uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:31:4-32:47: pattern match failure");
                   tmp;
                   });
                });
              });
            });
           }));
   }
  
  
  static uw_unit
   __uwn_wrap_big_7(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">built "), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                              uw_Basis_strlen(ctx,
                                                                               ({
                                                                               uw_Basis_string
                                                                               arg0
                                                                               =
                                                                               "x";
                                                                               
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               28LL;
                                                                               __uwn_grow_3(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               })
                                                                               )
                                                                              ),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_rows_8(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
                                                                              
                                                                               
                                                                               uw_conn *conn = uw_get_db(ctx);
                                                                               sqlite3_stmt *stmt = conn->p1;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N FROM uw_Io_heap_limit_t AS T_T ORDER BY T_T.uw_N", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_N FROM uw_Io_heap_limit_t AS T_T ORDER BY T_T.uw_N<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p1 = stmt;
                                                                               }
                                                                               
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                               
                                                                               
                                                                               int r;
                                                                               sqlite3_reset(stmt);
                                                                               uw_end_region(ctx);
                                                                               while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                                                               struct __uws_2 __uwr_r_2;
                                                                               uw_unit
                                                                               __uwr_acc_3
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_N
                                                                               =
                                                                               (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:56:20-56:26: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               sqlite3_column_int64(stmt, 0));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_N
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               " "), 0));
                                                                               }
                                                                               
                                                                               if (r == SQLITE_BUSY) {
                                                                               sleep(1);
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                               }
                                                                               
                                                                               if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:56:20-56:26: query step failed: %s<br />%s", 
                                                                               "SELECT T_T.uw_N FROM uw_Io_heap_limit_t AS T_T ORDER BY T_T.uw_N", sqlite3_errmsg(conn->conn));
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                              uw_end_region(ctx);
                                                                               acc;
                                                                              })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_main_9(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
           ((uw_write(ctx, "<body"), 0),
            (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                   uw_Basis_get_settings(ctx,
                                                    0))), 0),
             uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                        "")), 0),
                                  uw_end_region(ctx), ((uw_write(ctx, ">\n<button onclick='uw_event=event;exec("), 0),
                                                       (uw_begin_region(ctx), ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                        uw_end_region(ctx), ((uw_write(ctx, 
                                                                              ")'>outside</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                             (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                              uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>inside</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:3},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>handled</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
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
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))))));
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static void uw_periodic0(uw_context ctx) {
  uw_unit __uwr_$x_0 = 0, __uwr_$y_1 = 0;
   ({
    uw_Basis_int __uwr_k_2 =
    (uw_begin_region(ctx), uw_Probe_step(ctx));
    uw_end_region(ctx);
     ({
      uw_Basis_bool disc =
      __uwr_k_2 == 1LL;
      
      disc == uw_Basis_True ?
       uw_Basis_debug(ctx,
        ({
         uw_Basis_string arg0 = "task: built ";
          
          uw_Basis_string arg1 =
           uw_Basis_intToString(ctx,
            uw_Basis_strlen(ctx,
             ({
              uw_Basis_string arg0 = "x";
               uw_Basis_int arg1 = 28LL;
              __uwn_grow_3(ctx, arg0, arg1);
              })));
           uw_Basis_strcat(ctx, arg0, arg1);
         }))
        :
       disc == uw_Basis_False ?
        ({
         uw_Basis_bool disc =
         __uwr_k_2 == 2LL;
         
         disc == uw_Basis_True ?
          ({
           uw_unit __uwr___3 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_int* disc =
                                   ({
                                    uw_io_transaction __uwt;
                                    uw_Basis_int
                                    *__uwr = NULL;
                                    uw_io_transaction_begin(ctx, &__uwt);
                                    while (1) {
                                    int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                     if (__uwfk == 0) {
                                     uw_Basis_int __uwv =
                                      uw_Basis_strlen(ctx,
                                       ({
                                        uw_Basis_string arg0 = "x";
                                         uw_Basis_int arg1 = 28LL;
                                        __uwn_grow_3(ctx, arg0, arg1);
                                        }));
                                      __uwr = uw_malloc(ctx, sizeof(uw_Basis_int));
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
                                   
                                   disc == NULL ?
                                    uw_Basis_debug(ctx,
                                     ({
                                      uw_Basis_string arg0 = "task: handled, ";
                                       
                                       uw_Basis_string arg1 =
                                        uw_Basis_io_errorMessage(ctx);
                                        uw_Basis_strcat(ctx, arg0, arg1);
                                      }))
                                     :
                                    disc != NULL && 1 ?
                                     ({uw_Basis_int __uwr_n_3 = (*disc);
                                        uw_Basis_debug(ctx,
                                         ({
                                          uw_Basis_string arg0 =
                                           "task: handled, built ";
                                           
                                           uw_Basis_string arg1 =
                                            uw_Basis_intToString(ctx,
                                             __uwr_n_3);
                                            uw_Basis_strcat(ctx, arg0, arg1);
                                          }));
                                      })
                                      :
                                     ({
                                      uw_unit
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:41:8-45:57: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            uw_Basis_debug(ctx,
             ({
              uw_Basis_string arg0 = "task: still running, built ";
               
               uw_Basis_string arg1 =
                uw_Basis_intToString(ctx,
                 ({
                  uw_io_transaction __uwt;
                  uw_Basis_int
                  __uwr;
                  uw_io_transaction_begin(ctx, &__uwt);
                  while (1) {
                  int __uwfk = setjmp(*uw_jmp_buf(ctx));
                   if (__uwfk == 0) {
                   __uwr =
                    uw_Basis_strlen(ctx,
                     ({
                      uw_Basis_string arg0 = "x";
                       uw_Basis_int arg1 = 20LL;
                      __uwn_grow_3(ctx, arg0, arg1);
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
                uw_Basis_strcat(ctx, arg0, arg1);
              }));
           })
           :
          disc == uw_Basis_False ? 0
            :
           ({
            uw_unit
            tmp;
            uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:40:8-45:57: pattern match failure");
            tmp;
            });
         })
         :
        ({
         uw_unit
         tmp;
         uw_error(ctx, FATAL, "tests/io_heap_limit/io_heap_limit.ur:35:4-47:17: pattern match failure");
         tmp;
         });
      });
    });
   }
  
  static uw_periodic my_periodics[] = {{uw_periodic0, 1, 1},{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Io_heap_limit/main")) return 1;
    if (!strcmp(s, "/Io_heap_limit/rows")) return 1;
     if (!strcmp(s, "/Io_heap_limit/big")) return 1;
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
  
  
  if (!strcmp(request, "/app.4809095F845257AAB2BAE8E52F173E0D476E82D6.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Io_heap_limit/main", 19) && (request[19] == 0 || request[19] == '/')) {
  request += 19;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.4809095F845257AAB2BAE8E52F173E0D476E82D6.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_9(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Io_heap_limit/rows", 19) && (request[19] == 0 || request[19] == '/')) {
   request += 19;
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
     __uwn_wrap_rows_8(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Io_heap_limit/big", 18) && (request[18] == 0 || request[18] == '/')) {
   request += 18;
   if (*request == '/') ++request;
   uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
    uw_write(ctx, uw_begin_html5);
    uw_mayReturnIndirectly(ctx);
    uw_set_script_header(ctx, "");
    uw_set_could_write_db(ctx, 0);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     __uwn_wrap_big_7(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Io_heap_limit/outside", 22) && (request[22] == 0 || request[22] == '/')) {
   request += 22;
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
      __uwn_outside_4(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_heap_limit/inside", 21) && (request[21] == 0 || request[21] == '/')) {
   request += 21;
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
      __uwn_inside_5(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Io_heap_limit/handled", 22) && (request[22] == 0 || request[22] == '/')) {
   request += 22;
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
 