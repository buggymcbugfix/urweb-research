#include "include/urweb/config.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <time.h>
 #include <sqlite3.h>
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
     static int validated;
     uw_conn *conn = uw_get_db(ctx);
     sqlite3_stmt *stmt;
     int res;
     
     if (validated)
     return;
      
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Conditionals_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Query preparation failed:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Conditionals_t' COLLATE NOCASE");
       }
      
      while ((res = sqlite3_step(stmt)) == SQLITE_BUSY)
      sleep(1);
       
      if (res == SQLITE_DONE) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "No row returned:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Conditionals_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Error getting row:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Conditionals_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Bad column count:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Conditionals_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Table 'uw_Conditionals_t' does not exist.");
       }
      
      sqlite3_finalize(stmt);
      
     validated = 1;
     }
     
     static void uw_db_prepare(uw_context ctx) {
     uw_conn *conn = uw_get_db(ctx);
     
     if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (1, 'cond')", -1, &conn->p0, NULL) != SQLITE_OK) {
      char msg[1024];
       strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
       msg[1023] = 0;
       sqlite3_close(conn->conn);
        uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (1, 'cond')<br />%s"
        , msg);
        }
      
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'caseOpt')", -1, &conn->p1, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'caseOpt')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (3, 'chosen')", -1, &conn->p2, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (3, 'chosen')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'local')", -1, &conn->p3, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'local')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'two')", -1, &conn->p4, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'two')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N, T_T.uw_S FROM uw_Conditionals_t AS T_T ORDER BY T_T.uw_S, T_T.uw_N", -1, &conn->p5, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_N, T_T.uw_S FROM uw_Conditionals_t AS T_T ORDER BY T_T.uw_S, T_T.uw_N<br />%s"
         , msg);
         }
       }
    
    static void uw_db_init(uw_context ctx) {
    sqlite3 *sqlite;
    sqlite3_stmt *stmt;
    uw_conn *conn;
    
    char *env_db_str = getenv("URWEB_SQLITE_DB_PATH");
    const char *sqlite_db_path = env_db_str == NULL ? "conditionals.db" : env_db_str;
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
      if (conn->p2) sqlite3_finalize(conn->p2);
       if (conn->p3) sqlite3_finalize(conn->p3);
        if (conn->p4) sqlite3_finalize(conn->p4);
         if (conn->p5) sqlite3_finalize(conn->p5);
          sqlite3_close(conn->conn);
    }
    
    static int uw_db_begin(uw_context ctx, int could_write) {
    uw_conn *conn = uw_get_db(ctx);
    int r;
    
    r = sqlite3_exec(conn->conn, could_write ? "BEGIN IMMEDIATE" : "BEGIN", NULL, NULL, NULL);
    
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
   uw_Basis_int __uwf_N;
    uw_Basis_string __uwf_S;
     };
  struct __uws_2 {
   struct __uws_1 __uwf_T;
    };
  struct __uws_3 {
   struct __uws_2 __uwf_1;
    struct __uws_3* __uwf_2;
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
  
  /* SQL table uw_Conditionals_t constraints   */
   
  
  static uw_Basis_int
   __uwn_cond_3(uw_context ctx, uw_Basis_bool __uwr_c_0, uw_unit __uwr_y_1)
   {
   return(({
           uw_Basis_bool disc =
           __uwr_c_0;
           
           disc == uw_Basis_True ?
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
                uw_unit __uwr___2 =
                (uw_begin_region(ctx), ({
                 
                  
                  uw_ensure_transaction(ctx);
                  
                  uw_conn *conn = uw_get_db(ctx);
                   sqlite3_stmt *stmt = conn->p0;
                   
                   if (stmt == NULL) {
                   if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (1, 'cond')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (1, 'cond')<br />%s", sqlite3_errmsg(conn->conn));
                    conn->p0 = stmt;
                    }
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                   
                   int r;
                    if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                    uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                     }
                    
                    if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:11:30-11:85: DML step failed: %s<br />%s", 
                                           "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (1, 'cond')", sqlite3_errmsg(conn->conn));
                                           else if (uw_dml_log_enabled()) {
                    char *expanded = sqlite3_expanded_sql(stmt);
                     uw_log_dml(ctx, "tests/io_shapes/conditionals/conditionals.ur:11:30-11:85", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                     sqlite3_free(expanded);
                     }
                    uw_pop_cleanup(ctx);
                   uw_pop_cleanup(ctx);
                   
                 
                 uw_end_region(ctx);
                 0;
                 }));
                1LL;
                });
               uw_io_transaction_commit(ctx, &__uwt);
               break;
               } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
              break;
               }
              }
             uw_io_transaction_end(ctx, &__uwt, 1);
             __uwr;
             })
             :
            disc == uw_Basis_False ? 2LL
              :
             ({
              uw_Basis_int
              tmp;
              uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:11:14-11:86: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_Basis_int
   __uwn_caseOpt_4(uw_context ctx, uw_Basis_int* __uwr_o_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_x_2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_int* disc =
                                   __uwr_o_0;
                                   
                                   disc == NULL ? 0LL
                                     :
                                    disc != NULL && 1 ?
                                     ({uw_Basis_int __uwr_v_2 = (*disc);
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
                                            uw_unit __uwr___3 =
                                            (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                                    uw_Basis_int
                                                                     arg1 =
                                                                     __uwr_v_2;
                                                                     
                                                                     uw_ensure_transaction(ctx);
                                                                     
                                                                     uw_conn *conn = uw_get_db(ctx);
                                                                      sqlite3_stmt *stmt = conn->p1;
                                                                      
                                                                      if (stmt == NULL) {
                                                                      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'caseOpt')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'caseOpt')<br />%s", sqlite3_errmsg(conn->conn));
                                                                       conn->p1 = stmt;
                                                                       }
                                                                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                      if (sqlite3_bind_int64(stmt, 1, 
                                                                           arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:17:40-17:102: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                                       
                                                                      int r;
                                                                       if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                       uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                        }
                                                                       
                                                                       if (r != SQLITE_DONE) 
                                                                       uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:17:40-17:102: DML step failed: %s<br />%s", 
                                                                        "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'caseOpt')", sqlite3_errmsg(conn->conn));
                                                                        else if (uw_dml_log_enabled()) {
                                                                       char *expanded = sqlite3_expanded_sql(stmt);
                                                                        uw_log_dml(ctx, "tests/io_shapes/conditionals/conditionals.ur:17:40-17:102", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                        sqlite3_free(expanded);
                                                                        }
                                                                       uw_pop_cleanup(ctx);
                                                                      uw_pop_cleanup(ctx);
                                                                      
                                                                    
                                                                    uw_end_region(ctx);
                                                                    0;
                                                                    })));
                                            uw_end_region(ctx);
                                             __uwr_v_2;
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
                                      })
                                      :
                                     ({
                                      uw_Basis_int
                                      tmp;
                                      uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:15:4-18:20: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            __uwr_x_2 + 100LL;
           }));
   }
  
  static uw_Basis_int
   __uwn_chosen_5(uw_context ctx, uw_Basis_bool __uwr_c_0, uw_unit __uwr_y_1)
   {
   return(({
           uw_Basis_bool disc =
           __uwr_c_0;
           
           disc == uw_Basis_True ?
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
                uw_unit __uwr___2 =
                (uw_begin_region(ctx), ({
                 
                  
                  uw_ensure_transaction(ctx);
                  
                  uw_conn *conn = uw_get_db(ctx);
                   sqlite3_stmt *stmt = conn->p2;
                   
                   if (stmt == NULL) {
                   if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (3, 'chosen')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (3, 'chosen')<br />%s", sqlite3_errmsg(conn->conn));
                    conn->p2 = stmt;
                    }
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                   
                   int r;
                    if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                    uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                     }
                    
                    if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:24:42-24:99: DML step failed: %s<br />%s", 
                                           "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (3, 'chosen')", sqlite3_errmsg(conn->conn));
                                           else if (uw_dml_log_enabled()) {
                    char *expanded = sqlite3_expanded_sql(stmt);
                     uw_log_dml(ctx, "tests/io_shapes/conditionals/conditionals.ur:24:42-24:99", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                     sqlite3_free(expanded);
                     }
                    uw_pop_cleanup(ctx);
                   uw_pop_cleanup(ctx);
                   
                 
                 uw_end_region(ctx);
                 0;
                 }));
                3LL;
                });
               uw_io_transaction_commit(ctx, &__uwt);
               break;
               } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
              break;
               }
              }
             uw_io_transaction_end(ctx, &__uwt, 1);
             __uwr;
             })
             :
            disc == uw_Basis_False ?
             ({
              uw_Basis_int* disc =
              ({
               uw_io_transaction __uwt;
               uw_Basis_int
               *__uwr = NULL;
               uw_set_could_write_db(ctx, 0);
               uw_io_transaction_begin(ctx, &__uwt);
               while (1) {
               int __uwfk = setjmp(*uw_jmp_buf(ctx));
                if (__uwfk == 0) {
                uw_Basis_int __uwv =
                 ({
                  uw_Basis_int
                  tmp;
                  uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:26:45-26:68: %s", 
                  "no");
                  tmp;
                  });
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
              
              disc == NULL ? - 1LL
                :
               disc != NULL && 1 ?
                ({uw_Basis_int __uwr_v_2 = (*disc);
                   __uwr_v_2;
                 })
                 :
                ({
                 uw_Basis_int
                 tmp;
                 uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:26:20-27:46: pattern match failure");
                 tmp;
                 });
              })
              :
             ({
              uw_Basis_int
              tmp;
              uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:24:26-24:100: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_Basis_int __uwn_$step_6(uw_context, uw_Basis_int, uw_unit);
   static uw_Basis_int
    __uwn_$step_6(uw_context ctx, uw_Basis_int __uwr_i_0, uw_unit __uwr___1)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            __uwr_i_0 <= 0LL;
            
            disc == uw_Basis_True ? 0LL
              :
             disc == uw_Basis_False ?
              ({
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
                                          uw_Basis_int arg1 = __uwr_i_0;
                                           
                                           uw_ensure_transaction(ctx);
                                           
                                           uw_conn *conn = uw_get_db(ctx);
                                            sqlite3_stmt *stmt = conn->p3;
                                            
                                            if (stmt == NULL) {
                                            if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'local')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'local')<br />%s", sqlite3_errmsg(conn->conn));
                                             conn->p3 = stmt;
                                             }
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                            if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:38:16-38:83: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                             
                                            int r;
                                             if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                             uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                              }
                                             
                                             if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:38:16-38:83: DML step failed: %s<br />%s", 
                                                                    "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'local')", sqlite3_errmsg(conn->conn));
                                                                    else if (uw_dml_log_enabled()) {
                                             char *expanded = sqlite3_expanded_sql(stmt);
                                              uw_log_dml(ctx, "tests/io_shapes/conditionals/conditionals.ur:38:16-38:83", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                 uw_Basis_int rearg0 = __uwr_i_0 - 1LL;
                  uw_unit rearg1 = 0;
                 uw_Basis_int
                 tmp;
                 __uwr_i_0 = rearg0;
                  __uwr___1 = rearg1;
                 goto restart;
                 tmp;
                 });
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:33:0-43:7: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int
   __uwn_local_7(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return((({
            uw_Basis_int arg0 = __uwr_n_0;
             uw_unit arg1 = 0;
            __uwn_$step_6(ctx, arg0, arg1);
            })) + __uwr_n_0);
   }
  
  static uw_Basis_int*
   __uwn_twoAttempts_8(uw_context ctx, uw_Basis_int __uwr_n_0, 
                        uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int* disc =
           ({
            uw_io_transaction __uwt;
            uw_Basis_int
            *__uwr = NULL;
            uw_set_could_write_db(ctx, 1);
            uw_io_transaction_begin(ctx, &__uwt);
            while (1) {
            int __uwfk = setjmp(*uw_jmp_buf(ctx));
             if (__uwfk == 0) {
             uw_Basis_int __uwv =
              ({
               uw_unit __uwr___2 =
               (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                       uw_Basis_int arg1 = __uwr_n_0;
                                        
                                        uw_ensure_transaction(ctx);
                                        
                                        uw_conn *conn = uw_get_db(ctx);
                                         sqlite3_stmt *stmt = conn->p4;
                                         
                                         if (stmt == NULL) {
                                         if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'two')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'two')<br />%s", sqlite3_errmsg(conn->conn));
                                          conn->p4 = stmt;
                                          }
                                         uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                         uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                         if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:48:29-48:87: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                          
                                         int r;
                                          if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                          uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                           }
                                          
                                          if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:48:29-48:87: DML step failed: %s<br />%s", 
                                                                 "INSERT INTO uw_Conditionals_t (uw_N, uw_S) VALUES (\?, 'two')", sqlite3_errmsg(conn->conn));
                                                                 else if (uw_dml_log_enabled()) {
                                          char *expanded = sqlite3_expanded_sql(stmt);
                                           uw_log_dml(ctx, "tests/io_shapes/conditionals/conditionals.ur:48:29-48:87", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
           
           disc == NULL ? NULL
             :
            disc != NULL && 1 ?
             ({uw_Basis_int __uwr_a_2 = (*disc);
                ({
                 uw_io_transaction __uwt;
                 uw_Basis_int
                 *__uwr = NULL;
                 uw_set_could_write_db(ctx, 0);
                 uw_io_transaction_begin(ctx, &__uwt);
                 while (1) {
                 int __uwfk = setjmp(*uw_jmp_buf(ctx));
                  if (__uwfk == 0) {
                  uw_Basis_int __uwv =
                   ({
                    uw_Basis_bool disc =
                    ! (__uwr_a_2 <= 5LL);
                    
                    disc == uw_Basis_True ?
                     ({
                      uw_Basis_int
                      tmp;
                      uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:51:52-51:76: %s", 
                      "big");
                      tmp;
                      })
                      :
                     disc == uw_Basis_False ? __uwr_a_2 * 2LL
                       :
                      ({
                       uw_Basis_int
                       tmp;
                       uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:51:18-51:93: pattern match failure");
                       tmp;
                       });
                    });
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
              })
              :
             ({
              uw_Basis_int*
              tmp;
              uw_error(ctx, FATAL, "tests/io_shapes/conditionals/conditionals.ur:48:4-53:0: pattern match failure");
              tmp;
              });
           }));
   }
  
  static struct __uws_3* __uwn_$revPRIME_unpoly_9(uw_context,
   struct __uws_3*, struct __uws_3*);
   static struct __uws_3*
    __uwn_$revPRIME_unpoly_9(uw_context ctx, struct __uws_3* __uwr_acc_0, 
                              struct __uws_3* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_3* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_2 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_3* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_3* rearg0 =
                    ({
                     struct __uws_3 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_3));
                     *tmp =
                     ({ struct __uws_3 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_3* rearg1 = __uwr_ls_3;
                   struct __uws_3*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_3*
               tmp;
               uw_error(ctx, FATAL, "$/top.ur:279:21-282:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_string __uwn_$foldlPRIME_unpoly_unpoly_10(uw_context,
   uw_Basis_string, struct __uws_3*);
   static uw_Basis_string
    __uwn_$foldlPRIME_unpoly_unpoly_10(uw_context ctx, 
                                        uw_Basis_string __uwr_acc_0, 
                                        struct __uws_3* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_3* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_2 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_3* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_Basis_string rearg0 =
                    ({
                     uw_Basis_string arg0 = __uwr_acc_0;
                      uw_Basis_string arg1 = __uwr_x_2.__uwf_T.__uwf_S;
                       uw_Basis_string arg2 = ":";
                        
                      uw_Basis_string arg3 =
                       uw_Basis_intToString(ctx, __uwr_x_2.__uwf_T.__uwf_N);
                       uw_Basis_string arg4 = " ";
                        uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4,
                                                                           NULL);
                     });
                    struct __uws_3* rearg1 = __uwr_ls_3;
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
   __uwn_summary_11(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
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
               
               struct __uws_3* arg1 =
                ({
                 struct __uws_3* arg0 = NULL;
                  
                  struct __uws_3* arg1 =
                   ({
                    struct __uws_3* acc =
                    NULL;
                    int dummy = (uw_begin_region(ctx), 0);
                    uw_ensure_transaction(ctx);
                    
                     
                     uw_conn *conn = uw_get_db(ctx);
                      sqlite3_stmt *stmt = conn->p5;
                       
                       if (stmt == NULL) {
                       if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N, T_T.uw_S FROM uw_Conditionals_t AS T_T ORDER BY T_T.uw_S, T_T.uw_N", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_N, T_T.uw_S FROM uw_Conditionals_t AS T_T ORDER BY T_T.uw_S, T_T.uw_N<br />%s", sqlite3_errmsg(conn->conn));
                      conn->p5 = stmt;
                       }
                       
                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                       
                      
                      int r;
                       sqlite3_reset(stmt);
                       uw_end_region(ctx);
                       while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                       struct __uws_2 __uwr_r_2;
                        struct __uws_3* __uwr_acc_3 =
                        acc;
                        
                        __uwr_r_2.__uwf_T.__uwf_N =
                         (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? ({uw_Basis_int
                                                                         tmp;
                                                                         uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: Unexpectedly NULL field #0");
                                                                         tmp;
                                                                         }) : sqlite3_column_int64(stmt, 0));
                         
                         __uwr_r_2.__uwf_T.__uwf_S =
                          (sqlite3_column_type(stmt, 1) == SQLITE_NULL ? ({uw_Basis_string
                                                                          tmp;
                                                                          uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: Unexpectedly NULL field #1");
                                                                          tmp;
                                                                          }) : uw_strdup(ctx, (uw_Basis_string)sqlite3_column_text(stmt, 1)));
                          
                        
                        acc =
                        ({
                         struct __uws_3 *tmp =
                         uw_malloc(ctx, sizeof(struct __uws_3));
                         *tmp =
                         ({ struct __uws_3 tmp = {__uwr_r_2, __uwr_acc_3}; tmp;
                          });
                         tmp;
                         });
                        }
                       
                       if (r == SQLITE_BUSY) {
                       uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                        }
                       
                       if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: query step failed: %s<br />%s", 
                       "SELECT T_T.uw_N, T_T.uw_S FROM uw_Conditionals_t AS T_T ORDER BY T_T.uw_S, T_T.uw_N", sqlite3_errmsg(conn->conn));
                       
                       uw_pop_cleanup(ctx);
                      uw_pop_cleanup(ctx);
                       
                    acc;
                    });
                 __uwn_$revPRIME_unpoly_9(ctx, arg0, arg1);
                 });
              __uwn_$foldlPRIME_unpoly_unpoly_10(ctx, arg0, arg1);
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
   __uwn_wrap_main_12(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
                                                                               ")'>cond</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>caseOpt</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>chosen</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>local</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>twoAttempts</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:6},x:{c:\"c\",v:"), 0),
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
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:7},x:{c:\"c\",v:"), 0),
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
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:8},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s2_3
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
   if (!strcmp(s, "/Conditionals/main")) return 1;
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
  
  
  if (!strcmp(request, "/app.164AD59DBA35717939860AB1D4C66517DFF9DA0F.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Conditionals/main", 18) && (request[18] == 0 || request[18] == '/')) {
  request += 18;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.164AD59DBA35717939860AB1D4C66517DFF9DA0F.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_12(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Conditionals/cond", 18) && (request[18] == 0 || request[18] == '/')) {
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
    uw_Basis_bool arg0 = uw_Basis_unurlifyBool(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_int it0 =
      __uwn_cond_3(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Conditionals/caseOpt", 21) && (request[21] == 0 || request[21] == '/')) {
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
    uw_Basis_int* arg0 =
     (request[0] == '/' ? ++request : request, ((!strncmp(request, "None", 4) && (request[4] == 0 || request[4] == '/')) ? (request += (request[4] == 0 ? 4 : 5), NULL) : ((!strncmp(request, "Some", 4) && request[4] == '/') ? (request += 5, 
      ({
       uw_Basis_int *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
       *tmp = uw_Basis_unurlifyInt(ctx, &request);
       tmp;
       })) : (uw_error(ctx, FATAL, "Error unurlifying option type"), NULL))));
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_int it0 =
      __uwn_caseOpt_4(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Conditionals/chosen", 20) && (request[20] == 0 || request[20] == '/')) {
   request += 20;
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
    uw_Basis_bool arg0 = uw_Basis_unurlifyBool(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_int it0 =
      __uwn_chosen_5(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Conditionals/local", 19) && (request[19] == 0 || request[19] == '/')) {
   request += 19;
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
      __uwn_local_7(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Conditionals/twoAttempts", 25) && (request[25] == 0 || request[25] == '/')) {
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_int* it0 =
      __uwn_twoAttempts_8(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      if (it0) {uw_Basis_int it1 =
                 *it0;
                 uw_write(ctx, "Some/");
                 uw_Basis_urlifyInt_w(ctx, it1);
                  ;
                 } else {
                          uw_write(ctx, "None");
                          }
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Conditionals/summary", 21) && (request[21] == 0 || request[21] == '/')) {
   request += 21;
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
      __uwn_summary_11(ctx, arg0, 0);
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
 