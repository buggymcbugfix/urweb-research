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
       sqlite3_stmt *p2;
        sqlite3_stmt *p3;
         sqlite3_stmt *p4;
          sqlite3_stmt *p5;
           sqlite3_stmt *p6;
            sqlite3_stmt *p7;
             sqlite3_stmt *p8;
              sqlite3_stmt *p9;
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
      
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Closures_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      uw_set_error_message(ctx, "Query preparation failed: %s: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Closures_t' COLLATE NOCASE", sqlite3_errmsg(conn->conn));
       return 1;
       }
      
      res = sqlite3_step(stmt);
      
      if (res == SQLITE_BUSY) {
      sqlite3_finalize(stmt);
       uw_error(ctx, busy, "Database is busy; can't validate: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Closures_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      uw_set_error_message(ctx, "Error getting row: %s: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Closures_t' COLLATE NOCASE", res == SQLITE_DONE ? "no row" : sqlite3_errmsg(conn->conn));
       sqlite3_finalize(stmt);
       return 1;
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       uw_set_error_message(ctx, "Bad column count: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Closures_t' COLLATE NOCASE");
       return 1;
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
       sqlite3_finalize(stmt);
        uw_set_error_message(ctx, "Table '%s' does not exist.", "uw_Closures_t");
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
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'inner')", -1, &conn->p0, NULL) != SQLITE_OK) {
       uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'inner')", sqlite3_errmsg(conn->conn));
        return 1;
        }
       
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mk')", -1, &conn->p1, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mk')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')", -1, &conn->p2, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'A')", -1, &conn->p3, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'A')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'app')", -1, &conn->p4, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'app')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'foldlM')", -1, &conn->p5, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'foldlM')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mapM')", -1, &conn->p6, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mapM')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop')", -1, &conn->p7, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop2')", -1, &conn->p8, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop2')", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Closures_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", -1, &conn->p9, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Closures_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", sqlite3_errmsg(conn->conn));
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
    const char *sqlite_db_path = env_db_str == NULL ? "closures.db" : env_db_str;
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
      if (conn->p2) sqlite3_finalize(conn->p2);
       if (conn->p3) sqlite3_finalize(conn->p3);
        if (conn->p4) sqlite3_finalize(conn->p4);
         if (conn->p5) sqlite3_finalize(conn->p5);
          if (conn->p6) sqlite3_finalize(conn->p6);
           if (conn->p7) sqlite3_finalize(conn->p7);
            if (conn->p8) sqlite3_finalize(conn->p8);
             if (conn->p9) sqlite3_finalize(conn->p9);
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
    if (sqlite3_exec(conn->conn, "ROLLBACK", NULL, NULL, NULL) == SQLITE_OK)
    return 0;
     else {
    fprintf(stderr, "Rollback error: %s<br />", sqlite3_errmsg(conn->conn));
     return 1;
     }
    }
    
    
 
 /* No global setup for LRU cache. */
  
 
  
  struct __uws_1 {
   uw_Basis_int __uwf_1;
    struct __uws_1* __uwf_2;
     };
  struct __uws_2 {
   uw_Basis_string __uwf_S;
    };
  
  struct __uws_3
   {
   uw_Basis_int __uwf_C;
    uw_Basis_int* __uwf_Sum;
     struct __uws_2 __uwf_T;
      };
  struct __uws_4 {
   struct __uws_3 __uwf_1;
    struct __uws_4* __uwf_2;
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
  
  /* SQL table uw_Closures_t constraints   */
   
  
  static uw_Basis_int
   __uwn_$inner_3(uw_context ctx, uw_Basis_int __uwr_k_0, 
                   uw_Basis_int __uwr_m_1, uw_unit __uwr___2)
   {
   return(({
           uw_unit __uwr___3 =
           (uw_begin_region(ctx), uw_Probe_effect(ctx,
                                   ({
                                    uw_Basis_string arg0 = "inner ";
                                     
                                     uw_Basis_string arg1 =
                                      uw_Basis_intToString(ctx, __uwr_m_1);
                                      uw_Basis_strcat(ctx, arg0, arg1);
                                    })));
           uw_end_region(ctx);
            ({
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
                uw_unit __uwr___4 =
                (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                        uw_Basis_int arg1 =
                                         __uwr_k_0 + __uwr_m_1;
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p0;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'inner')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'inner')<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p0 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:12:28-13:42: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                           
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:12:28-13:42: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'inner')", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:12:28-13:42", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                            sqlite3_free(expanded);
                                            }
                                           uw_pop_cleanup(ctx);
                                          uw_pop_cleanup(ctx);
                                          
                                        
                                        uw_end_region(ctx);
                                        0;
                                        })));
                uw_end_region(ctx);
                 __uwr_k_0 + __uwr_m_1;
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
  
  static uw_Basis_int
   __uwn_local_4(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_k_2 =
           __uwr_n_0 * 10LL;
           (({
             uw_Basis_int arg0 = __uwr_k_2;
              uw_Basis_int arg1 = 1LL;
              uw_unit arg2 = 0;
             __uwn_$inner_3(ctx, arg0, arg1, arg2);
             })) +
            (({
              uw_Basis_int arg0 = __uwr_k_2;
               uw_Basis_int arg1 = 2LL;
               uw_unit arg2 = 0;
              __uwn_$inner_3(ctx, arg0, arg1, arg2);
              }));
           }));
   }
  
  static uw_Basis_int
   __uwn_$f_5(uw_context ctx, uw_Basis_int __uwr_n_0, uw_Basis_int __uwr_m_1, 
               uw_unit __uwr___2)
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
              uw_unit __uwr___3 =
              (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                      uw_Basis_int arg1 =
                                       __uwr_n_0 + __uwr_m_1;
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p1;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mk')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mk')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p1 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:22:28-22:95: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:22:28-22:95: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mk')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:22:28-22:95", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                          sqlite3_free(expanded);
                                          }
                                         uw_pop_cleanup(ctx);
                                        uw_pop_cleanup(ctx);
                                        
                                      
                                      uw_end_region(ctx);
                                      0;
                                      })));
              uw_end_region(ctx);
               __uwr_n_0 + __uwr_m_1;
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
   __uwn_returned_6(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return((({
            uw_Basis_int arg0 = __uwr_n_0;
             uw_Basis_int arg1 = 1LL;
             uw_unit arg2 = 0;
            __uwn_$f_5(ctx, arg0, arg1, arg2);
            })) +
           (({
             uw_Basis_int arg0 = __uwr_n_0;
              uw_Basis_int arg1 = 2LL;
              uw_unit arg2 = 0;
             __uwn_$f_5(ctx, arg0, arg1, arg2);
             })));
   }
  
  static uw_Basis_int
   __uwn_stored_7(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), uw_Probe_effect(ctx, "run B then A"));
           uw_end_region(ctx);
            ({
             uw_Basis_int __uwr_b_3 =
             (uw_begin_region(ctx), ({
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
                                        uw_unit __uwr___3 =
                                        (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                                uw_Basis_int
                                                                 arg1 =
                                                                 __uwr_n_0 +
                                                                  1LL;
                                                                 
                                                                 uw_ensure_transaction(ctx);
                                                                 
                                                                 uw_conn *conn = uw_get_db(ctx);
                                                                  sqlite3_stmt *stmt = conn->p2;
                                                                  
                                                                  if (stmt == NULL) {
                                                                  if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')<br />%s", sqlite3_errmsg(conn->conn));
                                                                   conn->p2 = stmt;
                                                                   }
                                                                  uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                  uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                  if (sqlite3_bind_int64(stmt, 1, 
                                                                       arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:37:37-37:99: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                                   
                                                                  int r;
                                                                   if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                   uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                    }
                                                                   
                                                                   if (r != SQLITE_DONE) 
                                                                   uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:37:37-37:99: DML step failed: %s<br />%s", 
                                                                    "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')", sqlite3_errmsg(conn->conn));
                                                                    else if (uw_dml_log_enabled()) {
                                                                   char *expanded = sqlite3_expanded_sql(stmt);
                                                                    uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:37:37-37:99", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                    sqlite3_free(expanded);
                                                                    }
                                                                   uw_pop_cleanup(ctx);
                                                                  uw_pop_cleanup(ctx);
                                                                  
                                                                
                                                                uw_end_region(ctx);
                                                                0;
                                                                })));
                                        uw_end_region(ctx);
                                         __uwr_n_0 + 1LL;
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
               uw_Basis_int __uwr_a_4 =
               (uw_begin_region(ctx), ({
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
                                          uw_unit __uwr___4 =
                                          (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                                  uw_Basis_int
                                                                   arg1 =
                                                                   __uwr_n_0;
                                                                   
                                                                   uw_ensure_transaction(ctx);
                                                                   
                                                                   uw_conn *conn = uw_get_db(ctx);
                                                                    sqlite3_stmt *stmt = conn->p3;
                                                                    
                                                                    if (stmt == NULL) {
                                                                    if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'A')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'A')<br />%s", sqlite3_errmsg(conn->conn));
                                                                     conn->p3 = stmt;
                                                                     }
                                                                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                    if (sqlite3_bind_int64(stmt, 1, 
                                                                         arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:36:37-36:93: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                                     
                                                                    int r;
                                                                     if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                     uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                      }
                                                                     
                                                                     if (r != SQLITE_DONE) 
                                                                     uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:36:37-36:93: DML step failed: %s<br />%s", 
                                                                      "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'A')", sqlite3_errmsg(conn->conn));
                                                                      else if (uw_dml_log_enabled()) {
                                                                     char *expanded = sqlite3_expanded_sql(stmt);
                                                                      uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:36:37-36:93", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                 uw_unit __uwr___5 =
                 (uw_begin_region(ctx), uw_Probe_effect(ctx, "run B again"));
                 uw_end_region(ctx);
                  (__uwr_a_4 + __uwr_b_3) +
                   (({
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
                        uw_unit __uwr___6 =
                        (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                uw_Basis_int arg1 =
                                                 __uwr_n_0 + 1LL;
                                                 
                                                 uw_ensure_transaction(ctx);
                                                 
                                                 uw_conn *conn = uw_get_db(ctx);
                                                  sqlite3_stmt *stmt = conn->p2;
                                                  
                                                  if (stmt == NULL) {
                                                  if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')<br />%s", sqlite3_errmsg(conn->conn));
                                                   conn->p2 = stmt;
                                                   }
                                                  uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                  uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                  if (sqlite3_bind_int64(stmt, 1, 
                                                       arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:37:37-37:99: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                   
                                                  int r;
                                                   if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                   uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                    }
                                                   
                                                   if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:37:37-37:99: DML step failed: %s<br />%s", 
                                                                          "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'B')", sqlite3_errmsg(conn->conn));
                                                                          else if (uw_dml_log_enabled()) {
                                                   char *expanded = sqlite3_expanded_sql(stmt);
                                                    uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:37:37-37:99", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                    sqlite3_free(expanded);
                                                    }
                                                   uw_pop_cleanup(ctx);
                                                  uw_pop_cleanup(ctx);
                                                  
                                                
                                                uw_end_region(ctx);
                                                0;
                                                })));
                        uw_end_region(ctx);
                         __uwr_n_0 + 1LL;
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
                 });
               });
             });
           }));
   }
  
  static struct __uws_1* __uwn_$tab_8(uw_context, uw_Basis_int, uw_Basis_int);
   static struct __uws_1*
    __uwn_$tab_8(uw_context ctx, uw_Basis_int __uwr_n_0, uw_Basis_int __uwr_i_1)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            ! (__uwr_i_1 < __uwr_n_0);
            
            disc == uw_Basis_True ? NULL
              :
             disc == uw_Basis_False ?
              ({
               struct __uws_1 *tmp =
               uw_malloc(ctx, sizeof(struct __uws_1));
               *tmp =
               ({ struct __uws_1 tmp =
                {__uwr_i_1, 
                  ({
                   uw_Basis_int arg0 = __uwr_n_0;
                    uw_Basis_int arg1 = __uwr_i_1 + 1LL;
                   __uwn_$tab_8(ctx, arg0, arg1);
                   })}; tmp; });
               tmp;
               })
               :
              ({
               struct __uws_1*
               tmp;
               uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:51:37-51:76: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static struct __uws_1* __uwn_$revPRIME_unpoly_9(uw_context,
   struct __uws_1*, struct __uws_1*);
   static struct __uws_1*
    __uwn_$revPRIME_unpoly_9(uw_context ctx, struct __uws_1* __uwr_acc_0, 
                              struct __uws_1* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_1* rearg0 =
                    ({
                     struct __uws_1 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_1));
                     *tmp =
                     ({ struct __uws_1 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_1* rearg1 = __uwr_ls_3;
                   struct __uws_1*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_1*
               tmp;
               uw_error(ctx, FATAL, "$/list.ur:47:21-50:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_$appPRIME_unpoly_unpoly_10(uw_context,
   struct __uws_1*, uw_unit);
   static uw_unit
    __uwn_$appPRIME_unpoly_unpoly_10(uw_context ctx, 
                                      struct __uws_1* __uwr_ls_0, 
                                      uw_unit __uwr___1)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_0;
            
            disc == NULL ? 0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_unit __uwr___4 =
                   (uw_begin_region(ctx), uw_Probe_effect(ctx,
                                           ({
                                            uw_Basis_string arg0 = "app ";
                                             
                                             uw_Basis_string arg1 =
                                              uw_Basis_intToString(ctx,
                                               __uwr_x_2);
                                              uw_Basis_strcat(ctx, arg0, arg1);
                                            })));
                   uw_end_region(ctx);
                    ({
                     uw_unit __uwr___5 =
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
                                                uw_Basis_int arg1 = __uwr_x_2;
                                                 
                                                 uw_ensure_transaction(ctx);
                                                 
                                                 uw_conn *conn = uw_get_db(ctx);
                                                  sqlite3_stmt *stmt = conn->p4;
                                                  
                                                  if (stmt == NULL) {
                                                  if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'app')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'app')<br />%s", sqlite3_errmsg(conn->conn));
                                                   conn->p4 = stmt;
                                                   }
                                                  uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                  uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                  if (sqlite3_bind_int64(stmt, 1, 
                                                       arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:56:22-56:87: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                   
                                                  int r;
                                                   if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                   uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                    }
                                                   
                                                   if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:56:22-56:87: DML step failed: %s<br />%s", 
                                                                          "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'app')", sqlite3_errmsg(conn->conn));
                                                                          else if (uw_dml_log_enabled()) {
                                                   char *expanded = sqlite3_expanded_sql(stmt);
                                                    uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:56:22-56:87", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                       struct __uws_1* rearg0 = __uwr_ls_3;
                        uw_unit rearg1 = 0;
                       uw_unit
                       tmp;
                       __uwr_ls_0 = rearg0;
                        __uwr___1 = rearg1;
                       goto restart;
                       tmp;
                       });
                     });
                   });
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
   
  
  static uw_Basis_int __uwn_$foldlMPRIME_unpoly_unpoly_unpoly_11(uw_context,
   uw_Basis_int, struct __uws_1*, uw_unit);
   static uw_Basis_int
    __uwn_$foldlMPRIME_unpoly_unpoly_unpoly_11(uw_context ctx, 
                                                uw_Basis_int __uwr_acc_0, 
                                                struct __uws_1* __uwr_ls_1, 
                                                uw_unit __uwr___2)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_3 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_ls_4 = (*disc).__uwf_2;
                  ({
                   uw_unit __uwr___5 =
                   (uw_begin_region(ctx), uw_Probe_effect(ctx,
                                           ({
                                            uw_Basis_string arg0 = "foldlM ";
                                             
                                             uw_Basis_string arg1 =
                                              uw_Basis_intToString(ctx,
                                               __uwr_x_3);
                                              uw_Basis_strcat(ctx, arg0, arg1);
                                            })));
                   uw_end_region(ctx);
                    ({
                     uw_Basis_int rearg0 =
                      ({
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
                          uw_unit __uwr___6 =
                          (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                  uw_Basis_int arg1 =
                                                   __uwr_x_3;
                                                   
                                                   uw_ensure_transaction(ctx);
                                                   
                                                   uw_conn *conn = uw_get_db(ctx);
                                                    sqlite3_stmt *stmt = conn->p5;
                                                    
                                                    if (stmt == NULL) {
                                                    if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'foldlM')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'foldlM')<br />%s", sqlite3_errmsg(conn->conn));
                                                     conn->p5 = stmt;
                                                     }
                                                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                    if (sqlite3_bind_int64(stmt, 1, 
                                                         arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:53:50-53:119: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                     
                                                    int r;
                                                     if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                     uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                      }
                                                     
                                                     if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:53:50-53:119: DML step failed: %s<br />%s", 
                                                                            "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'foldlM')", sqlite3_errmsg(conn->conn));
                                                                            else if (uw_dml_log_enabled()) {
                                                     char *expanded = sqlite3_expanded_sql(stmt);
                                                      uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:53:50-53:119", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                      sqlite3_free(expanded);
                                                      }
                                                     uw_pop_cleanup(ctx);
                                                    uw_pop_cleanup(ctx);
                                                    
                                                  
                                                  uw_end_region(ctx);
                                                  0;
                                                  })));
                          uw_end_region(ctx);
                           __uwr_acc_0 + __uwr_x_3;
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
                      struct __uws_1* rearg1 = __uwr_ls_4;
                      uw_unit rearg2 = 0;
                     uw_Basis_int
                     tmp;
                     __uwr_acc_0 = rearg0;
                      __uwr_ls_1 = rearg1;
                      __uwr___2 = rearg2;
                     goto restart;
                     tmp;
                     });
                   });
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static struct __uws_1* __uwn_$mapMPRIME_unpoly_unpoly_unpoly_12(uw_context,
   struct __uws_1*, struct __uws_1*, uw_unit);
   static struct __uws_1*
    __uwn_$mapMPRIME_unpoly_unpoly_unpoly_12(uw_context ctx, 
                                              struct __uws_1* __uwr_acc_0, 
                                              struct __uws_1* __uwr_ls_1, 
                                              uw_unit __uwr___2)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_1;
            
            disc == NULL ?
             ({
              struct __uws_1* arg0 = NULL;
               struct __uws_1* arg1 = __uwr_acc_0;
              __uwn_$revPRIME_unpoly_9(ctx, arg0, arg1);
              })
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_3 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_ls_4 = (*disc).__uwf_2;
                  ({
                   uw_unit __uwr___5 =
                   (uw_begin_region(ctx), uw_Probe_effect(ctx,
                                           ({
                                            uw_Basis_string arg0 = "mapM ";
                                             
                                             uw_Basis_string arg1 =
                                              uw_Basis_intToString(ctx,
                                               __uwr_x_3);
                                              uw_Basis_strcat(ctx, arg0, arg1);
                                            })));
                   uw_end_region(ctx);
                    ({
                     struct __uws_1* rearg0 =
                      ({
                       struct __uws_1 *tmp =
                       uw_malloc(ctx, sizeof(struct __uws_1));
                       *tmp =
                       ({ struct __uws_1 tmp =
                        {({
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
                             uw_unit __uwr___6 =
                             (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                     uw_Basis_int arg1 =
                                                      __uwr_x_3;
                                                      
                                                      uw_ensure_transaction(ctx);
                                                      
                                                      uw_conn *conn = uw_get_db(ctx);
                                                       sqlite3_stmt *stmt = conn->p6;
                                                       
                                                       if (stmt == NULL) {
                                                       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mapM')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mapM')<br />%s", sqlite3_errmsg(conn->conn));
                                                        conn->p6 = stmt;
                                                        }
                                                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                       if (sqlite3_bind_int64(stmt, 1, 
                                                            arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:50:45-50:110: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                        
                                                       int r;
                                                        if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                        uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                         }
                                                        
                                                        if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:50:45-50:110: DML step failed: %s<br />%s", 
                                                                               "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'mapM')", sqlite3_errmsg(conn->conn));
                                                                               else if (uw_dml_log_enabled()) {
                                                        char *expanded = sqlite3_expanded_sql(stmt);
                                                         uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:50:45-50:110", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                         sqlite3_free(expanded);
                                                         }
                                                        uw_pop_cleanup(ctx);
                                                       uw_pop_cleanup(ctx);
                                                       
                                                     
                                                     uw_end_region(ctx);
                                                     0;
                                                     })));
                             uw_end_region(ctx);
                              __uwr_x_3 * 2LL;
                             });
                            uw_io_transaction_commit(ctx, &__uwt);
                            break;
                            } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                           break;
                            }
                           }
                          uw_io_transaction_end(ctx, &__uwt, 1);
                          __uwr;
                          }), __uwr_acc_0}; tmp; });
                       tmp;
                       });
                      struct __uws_1* rearg1 = __uwr_ls_4;
                      uw_unit rearg2 = 0;
                     struct __uws_1*
                     tmp;
                     __uwr_acc_0 = rearg0;
                      __uwr_ls_1 = rearg1;
                      __uwr___2 = rearg2;
                     goto restart;
                     tmp;
                     });
                   });
               })
               :
              ({
               struct __uws_1*
               tmp;
               uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int
   __uwn_mapped_13(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_1* __uwr_xs_2 =
           ({
            struct __uws_1* arg0 = NULL;
             
             struct __uws_1* arg1 =
              ({
               uw_Basis_int arg0 = __uwr_n_0;
                uw_Basis_int arg1 = 0LL;
               __uwn_$tab_8(ctx, arg0, arg1);
               });
             uw_unit arg2 = 0;
            __uwn_$mapMPRIME_unpoly_unpoly_unpoly_12(ctx, arg0, arg1, arg2);
            });
           ({
            uw_Basis_int __uwr_s_3 =
            (uw_begin_region(ctx), ({
                                    uw_Basis_int arg0 = 0LL;
                                     struct __uws_1* arg1 = __uwr_xs_2;
                                     uw_unit arg2 = 0;
                                    __uwn_$foldlMPRIME_unpoly_unpoly_unpoly_11(ctx,
                                    arg0, arg1, arg2);
                                    }));
            uw_end_region(ctx);
             ({
              uw_unit __uwr___4 =
              (uw_begin_region(ctx), ({
                                      struct __uws_1* arg0 = __uwr_xs_2;
                                       uw_unit arg1 = 0;
                                      __uwn_$appPRIME_unpoly_unpoly_10(ctx,
                                      arg0, arg1);
                                      }));
              uw_end_region(ctx);
               __uwr_s_3;
              });
            });
           }));
   }
  
  static uw_Basis_int __uwn_loop_14(uw_context,
   uw_Basis_int, uw_Basis_int, uw_unit);
   static uw_Basis_int
    __uwn_loop_14(uw_context ctx, uw_Basis_int __uwr_n_0, 
                   uw_Basis_int __uwr_acc_1, uw_unit __uwr___2)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            __uwr_n_0 == 0LL;
            
            disc == uw_Basis_True ? __uwr_acc_1
              :
             disc == uw_Basis_False ?
              ({
               uw_Basis_int rearg0 = __uwr_n_0 - 1LL;
                
                uw_Basis_int rearg1 =
                 __uwr_acc_1 +
                  (({
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
                       uw_unit __uwr___3 =
                       (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                               uw_Basis_int arg1 = __uwr_n_0;
                                                
                                                uw_ensure_transaction(ctx);
                                                
                                                uw_conn *conn = uw_get_db(ctx);
                                                 sqlite3_stmt *stmt = conn->p7;
                                                 
                                                 if (stmt == NULL) {
                                                 if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop')<br />%s", sqlite3_errmsg(conn->conn));
                                                  conn->p7 = stmt;
                                                  }
                                                 uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                 uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                 if (sqlite3_bind_int64(stmt, 1, 
                                                      arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:64:29-64:88: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                  
                                                 int r;
                                                  if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                  uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                   }
                                                  
                                                  if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:64:29-64:88: DML step failed: %s<br />%s", 
                                                                         "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop')", sqlite3_errmsg(conn->conn));
                                                                         else if (uw_dml_log_enabled()) {
                                                  char *expanded = sqlite3_expanded_sql(stmt);
                                                   uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:64:29-64:88", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                      uw_io_transaction_commit(ctx, &__uwt);
                      break;
                      } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                     break;
                      }
                     }
                    uw_io_transaction_end(ctx, &__uwt, 1);
                    __uwr;
                    }));
                uw_unit rearg2 = 0;
               uw_Basis_int
               tmp;
               __uwr_n_0 = rearg0;
                __uwr_acc_1 = rearg1;
                __uwr___2 = rearg2;
               goto restart;
               tmp;
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:61:0-65:30: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int
   __uwn_recur_15(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr_x_1)
   {
   return(({
           uw_Basis_int arg0 = __uwr_n_0;
            uw_Basis_int arg1 = 0LL;
            uw_unit arg2 = __uwr_x_1;
           __uwn_loop_14(ctx, arg0, arg1, arg2);
           }));
   }
  
  static uw_Basis_int __uwn_loop2_16(uw_context, uw_Basis_int, uw_unit);
   static uw_Basis_int
    __uwn_loop2_16(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            __uwr_n_0 == 0LL;
            
            disc == uw_Basis_True ? 0LL
              :
             disc == uw_Basis_False ?
              (({
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
                                             sqlite3_stmt *stmt = conn->p8;
                                             
                                             if (stmt == NULL) {
                                             if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop2')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop2')<br />%s", sqlite3_errmsg(conn->conn));
                                              conn->p8 = stmt;
                                              }
                                             uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                             uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                             if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:73:29-73:89: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                              
                                             int r;
                                              if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                              uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                               }
                                              
                                              if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:73:29-73:89: DML step failed: %s<br />%s", 
                                                                     "INSERT INTO uw_Closures_t (uw_N, uw_S) VALUES (\?, 'loop2')", sqlite3_errmsg(conn->conn));
                                                                     else if (uw_dml_log_enabled()) {
                                              char *expanded = sqlite3_expanded_sql(stmt);
                                               uw_log_dml(ctx, "tests/io_shapes/closures/closures.ur:73:29-73:89", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                  uw_io_transaction_commit(ctx, &__uwt);
                  break;
                  } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                 break;
                  }
                 }
                uw_io_transaction_end(ctx, &__uwt, 1);
                __uwr;
                })) +
               (({
                 uw_Basis_int arg0 = __uwr_n_0 - 1LL;
                  uw_unit arg1 = 0;
                 __uwn_loop2_16(ctx, arg0, arg1);
                 }))
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:70:0-75:22: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static struct __uws_4* __uwn_$revPRIME_unpoly_17(uw_context,
   struct __uws_4*, struct __uws_4*);
   static struct __uws_4*
    __uwn_$revPRIME_unpoly_17(uw_context ctx, struct __uws_4* __uwr_acc_0, 
                               struct __uws_4* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_4* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_3 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_4* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_4* rearg0 =
                    ({
                     struct __uws_4 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_4));
                     *tmp =
                     ({ struct __uws_4 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_4* rearg1 = __uwr_ls_3;
                   struct __uws_4*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_4*
               tmp;
               uw_error(ctx, FATAL, "$/top.ur:279:21-282:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_string __uwn_$foldlPRIME_unpoly_unpoly_18(uw_context,
   uw_Basis_string, struct __uws_4*);
   static uw_Basis_string
    __uwn_$foldlPRIME_unpoly_unpoly_18(uw_context ctx, 
                                        uw_Basis_string __uwr_acc_0, 
                                        struct __uws_4* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_4* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_3 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_4* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_Basis_string rearg0 =
                    ({
                     uw_Basis_string arg0 = __uwr_acc_0;
                      uw_Basis_string arg1 = __uwr_x_2.__uwf_T.__uwf_S;
                       uw_Basis_string arg2 = ":";
                        
                      uw_Basis_string arg3 =
                       uw_Basis_intToString(ctx, __uwr_x_2.__uwf_C);
                       uw_Basis_string arg4 = ":";
                        
                      uw_Basis_string arg5 =
                       ({
                        uw_Basis_int* disc =
                        __uwr_x_2.__uwf_Sum;
                        
                        disc == NULL ? ""
                          :
                         disc != NULL && 1 ?
                          ({uw_Basis_int __uwr_x_4 = (*disc);
                             uw_Basis_intToString(ctx, __uwr_x_4);
                           })
                           :
                          ({
                           uw_Basis_string
                           tmp;
                           uw_error(ctx, FATAL, "tests/io_shapes/closures/closures.ur:79:89-79:99: pattern match failure");
                           tmp;
                           });
                        });
                       uw_Basis_string arg6 = " ";
                        uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, 
                                               arg5, arg6, NULL);
                     });
                    struct __uws_4* rearg1 = __uwr_ls_3;
                   uw_Basis_string
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "$/list.ur:27:12-30:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_string
   __uwn_summary_19(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(({
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
              uw_Basis_string arg0 = "";
               
               struct __uws_4* arg1 =
                ({
                 struct __uws_4* arg0 = NULL;
                  
                  struct __uws_4* arg1 =
                   ({
                    struct __uws_4* acc =
                    NULL;
                    int dummy = (uw_begin_region(ctx), 0);
                    uw_ensure_transaction(ctx);
                    
                     
                     uw_conn *conn = uw_get_db(ctx);
                      sqlite3_stmt *stmt = conn->p9;
                       
                       if (stmt == NULL) {
                       if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Closures_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Closures_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S<br />%s", sqlite3_errmsg(conn->conn));
                      conn->p9 = stmt;
                       }
                       
                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                       
                      
                      int r;
                       sqlite3_reset(stmt);
                       uw_end_region(ctx);
                       while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                       struct __uws_3 __uwr_r_2;
                        struct __uws_4* __uwr_acc_3 =
                        acc;
                        
                        __uwr_r_2.__uwf_C =
                         (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? ({uw_Basis_int
                                                                         tmp;
                                                                         uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: Unexpectedly NULL field #0");
                                                                         tmp;
                                                                         }) : sqlite3_column_int64(stmt, 0));
                         
                         __uwr_r_2.__uwf_Sum =
                          (sqlite3_column_type(stmt, 1) == SQLITE_NULL ? NULL : 
                           ({
                            uw_Basis_int
                            *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
                            *tmp = (sqlite3_column_type(stmt, 1) == SQLITE_NULL ? 
                                    ({uw_Basis_int
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: Unexpectedly NULL field #1");
                                     tmp;
                                     }) : sqlite3_column_int64(stmt, 1));
                            tmp;
                            }));
                          
                         __uwr_r_2.__uwf_T.__uwf_S =
                          (sqlite3_column_type(stmt, 2) == SQLITE_NULL ? ({uw_Basis_string
                                                                          tmp;
                                                                          uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: Unexpectedly NULL field #2");
                                                                          tmp;
                                                                          }) : uw_strdup(ctx, (uw_Basis_string)sqlite3_column_text(stmt, 2)));
                          
                        
                        acc =
                        ({
                         struct __uws_4 *tmp =
                         uw_malloc(ctx, sizeof(struct __uws_4));
                         *tmp =
                         ({ struct __uws_4 tmp = {__uwr_r_2, __uwr_acc_3}; tmp;
                          });
                         tmp;
                         });
                        }
                       
                       if (r == SQLITE_BUSY) {
                       uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                        }
                       
                       if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: query step failed: %s<br />%s", 
                       "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Closures_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", sqlite3_errmsg(conn->conn));
                       
                       uw_pop_cleanup(ctx);
                      uw_pop_cleanup(ctx);
                       
                    acc;
                    });
                 __uwn_$revPRIME_unpoly_17(ctx, arg0, arg1);
                 });
              __uwn_$foldlPRIME_unpoly_unpoly_18(ctx, arg0, arg1);
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
  
  static uw_unit
   __uwn_wrap_main_20(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
            uw_Basis_source __uwr_s2_3 =
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
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                         uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ")'>local</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>returned</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>stored</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>mapped</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>recur</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>loop2</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:7},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s2_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>summary</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
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
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))))))))))))));
            });
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Closures/main")) return 1;
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
  
  
  if (!strcmp(request, "/app.FBE4A017F1737D75EA509073433C0CF4454A8B00.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Closures/main", 14) && (request[14] == 0 || request[14] == '/')) {
  request += 14;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.FBE4A017F1737D75EA509073433C0CF4454A8B00.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_20(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Closures/local", 15) && (request[15] == 0 || request[15] == '/')) {
   request += 15;
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
      __uwn_local_4(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Closures/returned", 18) && (request[18] == 0 || request[18] == '/')) {
   request += 18;
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
      __uwn_returned_6(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Closures/stored", 16) && (request[16] == 0 || request[16] == '/')) {
   request += 16;
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
      __uwn_stored_7(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Closures/mapped", 16) && (request[16] == 0 || request[16] == '/')) {
   request += 16;
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
      __uwn_mapped_13(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Closures/recur", 15) && (request[15] == 0 || request[15] == '/')) {
   request += 15;
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
      __uwn_recur_15(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Closures/loop2", 15) && (request[15] == 0 || request[15] == '/')) {
   request += 15;
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
      __uwn_loop2_16(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Closures/summary", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
   uw_set_at_most_one_query(ctx, 1);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_string it0 =
      __uwn_summary_19(ctx, arg0, 0);
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
 