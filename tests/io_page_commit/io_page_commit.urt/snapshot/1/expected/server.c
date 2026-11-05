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
               sqlite3_stmt *p10;
                sqlite3_stmt *p11;
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
      
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_page_commit_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      uw_set_error_message(ctx, "Query preparation failed: %s: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_page_commit_t' COLLATE NOCASE", sqlite3_errmsg(conn->conn));
       return 1;
       }
      
      res = sqlite3_step(stmt);
      
      if (res == SQLITE_BUSY) {
      sqlite3_finalize(stmt);
       uw_error(ctx, busy, "Database is busy; can't validate: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_page_commit_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      uw_set_error_message(ctx, "Error getting row: %s: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_page_commit_t' COLLATE NOCASE", res == SQLITE_DONE ? "no row" : sqlite3_errmsg(conn->conn));
       sqlite3_finalize(stmt);
       return 1;
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       uw_set_error_message(ctx, "Bad column count: %s", "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Io_page_commit_t' COLLATE NOCASE");
       return 1;
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
       sqlite3_finalize(stmt);
        uw_set_error_message(ctx, "Table '%s' does not exist.", "uw_Io_page_commit_t");
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
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (21)", -1, &conn->p0, NULL) != SQLITE_OK) {
       uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (21)", sqlite3_errmsg(conn->conn));
        return 1;
        }
       
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (11)", -1, &conn->p1, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (11)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (8)", -1, &conn->p2, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (8)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (7)", -1, &conn->p3, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (7)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (\?)", -1, &conn->p4, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (5)", -1, &conn->p5, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (5)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (6)", -1, &conn->p6, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (6)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (3)", -1, &conn->p7, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (3)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (4)", -1, &conn->p8, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (4)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (1)", -1, &conn->p9, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (1)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (2)", -1, &conn->p10, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (2)", sqlite3_errmsg(conn->conn));
         return 1;
         }
        
       
       if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N FROM uw_Io_page_commit_t AS T_T ORDER BY T_T.uw_N", -1, &conn->p11, NULL) != SQLITE_OK) {
        uw_set_error_message(ctx, "Error preparing statement: %s: %s", "SELECT T_T.uw_N FROM uw_Io_page_commit_t AS T_T ORDER BY T_T.uw_N", sqlite3_errmsg(conn->conn));
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
    const char *sqlite_db_path = env_db_str == NULL ? "io_page_commit.db" : env_db_str;
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
      if (conn->p2) sqlite3_finalize(conn->p2);
       if (conn->p3) sqlite3_finalize(conn->p3);
        if (conn->p4) sqlite3_finalize(conn->p4);
         if (conn->p5) sqlite3_finalize(conn->p5);
          if (conn->p6) sqlite3_finalize(conn->p6);
           if (conn->p7) sqlite3_finalize(conn->p7);
            if (conn->p8) sqlite3_finalize(conn->p8);
             if (conn->p9) sqlite3_finalize(conn->p9);
              if (conn->p10) sqlite3_finalize(conn->p10);
               if (conn->p11) sqlite3_finalize(conn->p11);
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
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Io_page_commit_t constraints   */
   
  
  static uw_unit
   __uwn_wrap_adoptedHandled_3(uw_context ctx, uw_unit __uwr_x0_0, 
                                uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 =
                                    "uw_Io_page_commit_t";
                                    uw_Basis_int arg1 = 20LL;
                                     uw_Probe_dmlIo(ctx, arg0, arg1);
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">adoptedHandled: "), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_unit*
                                                                               disc
                                                                               =
                                                                               ({
                                                                               uw_io_transaction __uwt;
                                                                               uw_unit
                                                                               *__uwr = NULL;
                                                                               uw_set_could_write_db(ctx, 1);
                                                                               uw_io_transaction_begin(ctx, &__uwt);
                                                                               while (1) {
                                                                               int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                                                               if (__uwfk == 0) {
                                                                               uw_unit
                                                                               __uwv =
                                                                               ({
                                                                               uw_unit
                                                                               __uwr___3
                                                                               =
                                                                               (uw_begin_region(ctx), ({
                                                                               
                                                                               
                                                                               uw_ensure_transaction(ctx);
                                                                               
                                                                               uw_conn *conn = uw_get_db(ctx);
                                                                               sqlite3_stmt *stmt = conn->p0;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (21)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (21)<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p0 = stmt;
                                                                               }
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                               
                                                                               int r;
                                                                               if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                               }
                                                                               
                                                                               if (r != SQLITE_DONE) 
                                                                               uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:83:29-84:74: DML step failed: %s<br />%s", 
                                                                               "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (21)", sqlite3_errmsg(conn->conn));
                                                                               else if (uw_dml_log_enabled()) {
                                                                               char *expanded = sqlite3_expanded_sql(stmt);
                                                                               uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:83:29-84:74", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                               sqlite3_free(expanded);
                                                                               }
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               
                                                                               uw_end_region(ctx);
                                                                               0;
                                                                               }))
                                                                               ;
                                                                               ({
                                                                               uw_unit
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:84:30-84:58: %s", 
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
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               "None"
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               "Some"
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               })
                                                                               ),
                                                         uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ";\nopen afterwards: "), 0),
                                                                              (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyBool_w(ctx,
                                                                               uw_Probe_inTransaction(ctx
                                                                               )),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</body>"), 0))))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_adopted_4(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 =
                                    "uw_Io_page_commit_t";
                                    uw_Basis_int arg1 = 10LL;
                                     uw_Probe_dmlIo(ctx, arg0, arg1);
                                   }));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">adopted: committed by attempt "), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
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
                                                                               uw_Basis_int
                                                                               __uwr_k_3
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               uw_Probe_stepT(ctx
                                                                               ));
                                                                               uw_end_region(ctx);
                                                                               ({
                                                                               uw_unit
                                                                               __uwr___4
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               uw_Probe_armConflict(ctx,
                                                                               "adopted"
                                                                               ));
                                                                               uw_end_region(ctx);
                                                                               ({
                                                                               uw_unit
                                                                               __uwr___5
                                                                               =
                                                                               (uw_begin_region(ctx), ({
                                                                               
                                                                               
                                                                               uw_ensure_transaction(ctx);
                                                                               
                                                                               uw_conn *conn = uw_get_db(ctx);
                                                                               sqlite3_stmt *stmt = conn->p1;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (11)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (11)<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p1 = stmt;
                                                                               }
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                               
                                                                               int r;
                                                                               if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                               }
                                                                               
                                                                               if (r != SQLITE_DONE) 
                                                                               uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:76:25-77:33: DML step failed: %s<br />%s", 
                                                                               "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (11)", sqlite3_errmsg(conn->conn));
                                                                               else if (uw_dml_log_enabled()) {
                                                                               char *expanded = sqlite3_expanded_sql(stmt);
                                                                               uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:76:25-77:33", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                               sqlite3_free(expanded);
                                                                               }
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               
                                                                               uw_end_region(ctx);
                                                                               0;
                                                                               }))
                                                                               ;
                                                                               __uwr_k_3;
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
                                                                               })
                                                                               ),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_unlock_5(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), uw_Probe_finishPendingWriteIo(ctx));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">unlocked; open: "), 0),
                                                        (uw_begin_region(ctx), uw_Basis_htmlifyBool_w(ctx,
                                                                               uw_Probe_inTransaction(ctx
                                                                               )),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_busyFatal_6(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
                                      (uw_begin_region(ctx), uw_Probe_register(ctx,
                                                              "busyFatal"));
                                      uw_end_region(ctx);
                                       ({
                                        uw_unit __uwr___3 =
                                        (uw_begin_region(ctx), ({
                                         
                                          
                                          uw_ensure_transaction(ctx);
                                          
                                          uw_conn *conn = uw_get_db(ctx);
                                           sqlite3_stmt *stmt = conn->p2;
                                           
                                           if (stmt == NULL) {
                                           if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (8)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (8)<br />%s", sqlite3_errmsg(conn->conn));
                                            conn->p2 = stmt;
                                            }
                                           uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                           uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                           
                                           int r;
                                            if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                            uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                             }
                                            
                                            if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:65:20-66:60: DML step failed: %s<br />%s", 
                                                                   "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (8)", sqlite3_errmsg(conn->conn));
                                                                   else if (uw_dml_log_enabled()) {
                                            char *expanded = sqlite3_expanded_sql(stmt);
                                             uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:65:20-66:60", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                             sqlite3_free(expanded);
                                             }
                                            uw_pop_cleanup(ctx);
                                           uw_pop_cleanup(ctx);
                                           
                                         
                                         uw_end_region(ctx);
                                         0;
                                         }));
                                        uw_Probe_pendingWrite(ctx,
                                         "uw_Io_page_commit_t");
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
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), (uw_write(ctx, ">never sent</body>"), 0))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_raisesFatal_7(uw_context ctx, uw_unit __uwr_x0_0, 
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
                                      (uw_begin_region(ctx), uw_Probe_raiseInCommit(ctx,
                                                              "raisesFatal"));
                                      uw_end_region(ctx);
                                       (uw_begin_region(ctx), ({
                                        
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p3;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (7)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (7)<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p3 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:59:20-60:54: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (7)", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:59:20-60:54", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), (uw_write(ctx, ">never sent</body>"), 0))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_conflict_8(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">conflict: committed by attempt "), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
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
                                                                               uw_Basis_int
                                                                               __uwr_k_2
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               uw_Probe_stepT(ctx
                                                                               ));
                                                                               uw_end_region(ctx);
                                                                               ({
                                                                               uw_unit
                                                                               __uwr___3
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               uw_Probe_armConflict(ctx,
                                                                               "conflict"
                                                                               ));
                                                                               uw_end_region(ctx);
                                                                               ({
                                                                               uw_unit
                                                                               __uwr___4
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               (uw_begin_region(ctx), ({
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               __uwr_k_2;
                                                                               
                                                                               uw_ensure_transaction(ctx);
                                                                               
                                                                               uw_conn *conn = uw_get_db(ctx);
                                                                               sqlite3_stmt *stmt = conn->p4;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p4 = stmt;
                                                                               }
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                               if (
                                                                               sqlite3_bind_int64(stmt, 1, 
                                                                               arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:53:25-54:33: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                                               
                                                                               int r;
                                                                               if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                               }
                                                                               
                                                                               if (r != SQLITE_DONE) 
                                                                               uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:53:25-54:33: DML step failed: %s<br />%s", 
                                                                               "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                               else if (uw_dml_log_enabled()) {
                                                                               char *expanded = sqlite3_expanded_sql(stmt);
                                                                               uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:53:25-54:33", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                               sqlite3_free(expanded);
                                                                               }
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               
                                                                               uw_end_region(ctx);
                                                                               0;
                                                                               })));
                                                                               uw_end_region(ctx);
                                                                               __uwr_k_2;
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
                                                                               })
                                                                              ),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_raises_9(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit* __uwr_r_2 =
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
               (uw_begin_region(ctx), uw_Probe_raiseInCommit(ctx, "raises"));
               uw_end_region(ctx);
                (uw_begin_region(ctx), ({
                 
                  
                  uw_ensure_transaction(ctx);
                  
                  uw_conn *conn = uw_get_db(ctx);
                   sqlite3_stmt *stmt = conn->p5;
                   
                   if (stmt == NULL) {
                   if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (5)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (5)<br />%s", sqlite3_errmsg(conn->conn));
                    conn->p5 = stmt;
                    }
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                   
                   int r;
                    if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                    uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                     }
                    
                    if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:41:29-42:63: DML step failed: %s<br />%s", 
                                           "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (5)", sqlite3_errmsg(conn->conn));
                                           else if (uw_dml_log_enabled()) {
                    char *expanded = sqlite3_expanded_sql(stmt);
                     uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:41:29-42:63", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                     sqlite3_free(expanded);
                     }
                    uw_pop_cleanup(ctx);
                   uw_pop_cleanup(ctx);
                   
                 
                 uw_end_region(ctx);
                 0;
                 }));
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
            uw_Basis_string __uwr_m_3 =
            uw_Basis_io_errorMessage(ctx);
            ({
             uw_unit __uwr___4 =
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
                                        
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p6;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (6)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (6)<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p6 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:44:4-44:55: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (6)", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:44:4-44:55", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
              ((uw_write(ctx, "<body"), 0),
               (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                      uw_Basis_get_settings(ctx,
                                                       0))), 0),
                uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                           "")), 0),
                                     uw_end_region(ctx), ((uw_write(ctx, ">raises: "), 0),
                                                          (uw_begin_region(ctx),
                                                            uw_Basis_htmlifyString_w(ctx,
                                                             ({
                                                              uw_unit* disc =
                                                              __uwr_r_2;
                                                              
                                                              disc == NULL ?
                                                               "None"
                                                                :
                                                               disc != NULL &&
                                                                1 ? "Some"
                                                                 :
                                                                ({
                                                                 uw_Basis_string
                                                                 tmp;
                                                                 uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                 tmp;
                                                                 });
                                                              })),
                                                           uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ";\nmessage: "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_m_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</body>"), 0))))))));
             });
            });
           }));
   }
  
  static uw_unit
   __uwn_wrap_failsBefore_10(uw_context ctx, uw_unit __uwr_x0_0, 
                              uw_unit __uwr___1)
   {
   return(({
           uw_unit* __uwr_r_2 =
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
               (uw_begin_region(ctx), uw_Probe_failInCommitBeforeDb(ctx,
                                       "failsBefore"));
               uw_end_region(ctx);
                (uw_begin_region(ctx), ({
                 
                  
                  uw_ensure_transaction(ctx);
                  
                  uw_conn *conn = uw_get_db(ctx);
                   sqlite3_stmt *stmt = conn->p7;
                   
                   if (stmt == NULL) {
                   if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (3)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (3)<br />%s", sqlite3_errmsg(conn->conn));
                    conn->p7 = stmt;
                    }
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                   uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                   
                   int r;
                    if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                    uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                     }
                    
                    if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:30:29-31:63: DML step failed: %s<br />%s", 
                                           "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (3)", sqlite3_errmsg(conn->conn));
                                           else if (uw_dml_log_enabled()) {
                    char *expanded = sqlite3_expanded_sql(stmt);
                     uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:30:29-31:63", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                     sqlite3_free(expanded);
                     }
                    uw_pop_cleanup(ctx);
                   uw_pop_cleanup(ctx);
                   
                 
                 uw_end_region(ctx);
                 0;
                 }));
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
            uw_Basis_string __uwr_m_3 =
            uw_Basis_io_errorMessage(ctx);
            ({
             uw_unit __uwr___4 =
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
                                        
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p8;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (4)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (4)<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p8 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:33:4-33:55: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (4)", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:33:4-33:55", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
              ((uw_write(ctx, "<body"), 0),
               (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                      uw_Basis_get_settings(ctx,
                                                       0))), 0),
                uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                           "")), 0),
                                     uw_end_region(ctx), ((uw_write(ctx, ">failsBefore: "), 0),
                                                          (uw_begin_region(ctx),
                                                            uw_Basis_htmlifyString_w(ctx,
                                                             ({
                                                              uw_unit* disc =
                                                              __uwr_r_2;
                                                              
                                                              disc == NULL ?
                                                               "None"
                                                                :
                                                               disc != NULL &&
                                                                1 ? "Some"
                                                                 :
                                                                ({
                                                                 uw_Basis_string
                                                                 tmp;
                                                                 uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                 tmp;
                                                                 });
                                                              })),
                                                           uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ";\nmessage: "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_m_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</body>"), 0))))))));
             });
            });
           }));
   }
  
  static uw_unit
   __uwn_wrap_busy_11(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit* __uwr_r_2 =
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
               (uw_begin_region(ctx), uw_Probe_register(ctx, "busy"));
               uw_end_region(ctx);
                ({
                 uw_unit __uwr___3 =
                 (uw_begin_region(ctx), ({
                  
                   
                   uw_ensure_transaction(ctx);
                   
                   uw_conn *conn = uw_get_db(ctx);
                    sqlite3_stmt *stmt = conn->p9;
                    
                    if (stmt == NULL) {
                    if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (1)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (1)<br />%s", sqlite3_errmsg(conn->conn));
                     conn->p9 = stmt;
                     }
                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                    
                    int r;
                     if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                     uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                      }
                     
                     if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:18:29-19:69: DML step failed: %s<br />%s", 
                                            "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (1)", sqlite3_errmsg(conn->conn));
                                            else if (uw_dml_log_enabled()) {
                     char *expanded = sqlite3_expanded_sql(stmt);
                      uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:18:29-19:69", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                      sqlite3_free(expanded);
                      }
                     uw_pop_cleanup(ctx);
                    uw_pop_cleanup(ctx);
                    
                  
                  uw_end_region(ctx);
                  0;
                  }));
                 uw_Probe_pendingWrite(ctx, "uw_Io_page_commit_t");
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
            uw_Basis_string __uwr_m_3 =
            uw_Basis_io_errorMessage(ctx);
            ({
             uw_unit __uwr___4 =
             (uw_begin_region(ctx), uw_Probe_finishPendingWriteIo(ctx));
             uw_end_region(ctx);
              ({
               uw_Basis_bool __uwr_isOpen_5 =
               (uw_begin_region(ctx), uw_Probe_inTransaction(ctx));
               uw_end_region(ctx);
                ({
                 uw_unit __uwr___6 =
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
                                            
                                             
                                             uw_ensure_transaction(ctx);
                                             
                                             uw_conn *conn = uw_get_db(ctx);
                                              sqlite3_stmt *stmt = conn->p10;
                                              
                                              if (stmt == NULL) {
                                              if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (2)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (2)<br />%s", sqlite3_errmsg(conn->conn));
                                               conn->p10 = stmt;
                                               }
                                              uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                              uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                              
                                              int r;
                                               if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                }
                                               
                                               if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_page_commit/io_page_commit.ur:23:4-23:55: DML step failed: %s<br />%s", 
                                                                      "INSERT INTO uw_Io_page_commit_t (uw_N) VALUES (2)", sqlite3_errmsg(conn->conn));
                                                                      else if (uw_dml_log_enabled()) {
                                               char *expanded = sqlite3_expanded_sql(stmt);
                                                uw_log_dml(ctx, "tests/io_page_commit/io_page_commit.ur:23:4-23:55", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                  ((uw_write(ctx, "<body"), 0),
                   (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                          uw_Basis_get_settings(ctx,
                                                           0))), 0),
                    uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                               ""
                                                                               )), 0),
                                         uw_end_region(ctx), ((uw_write(ctx, ">busy: "), 0),
                                                              (uw_begin_region(ctx),
                                                                uw_Basis_htmlifyString_w(ctx,
                                                                 ({
                                                                  uw_unit* disc
                                                                  =
                                                                  __uwr_r_2;
                                                                  
                                                                  disc == NULL
                                                                   ? "None"
                                                                    :
                                                                   disc != NULL
                                                                    && 1 ?
                                                                    "Some"
                                                                     :
                                                                    ({
                                                                     uw_Basis_string
                                                                     tmp;
                                                                     uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                                                     tmp;
                                                                     });
                                                                  })),
                                                               uw_end_region(ctx),
                                                                ((uw_write(ctx, 
                                                                  ";\nmessage: "), 0),
                                                                 (uw_begin_region(ctx),
                                                                   uw_Basis_htmlifyString_w(ctx,
                                                                    __uwr_m_3),
                                                                  uw_end_region(ctx),
                                                                   ((uw_write(ctx, 
                                                                     "; open afterwards: "), 0),
                                                                    (uw_begin_region(ctx),
                                                                      uw_Basis_htmlifyBool_w(ctx,
                                                                       __uwr_isOpen_5
                                                                       ),
                                                                     uw_end_region(ctx),
                                                                      (uw_write(ctx, 
                                                                       "</body>"), 0))))))))));
                 });
               });
             });
            });
           }));
   }
  
  static struct __uws_2* __uwn_$revPRIME_unpoly_12(uw_context,
   struct __uws_2*, struct __uws_2*);
   static struct __uws_2*
    __uwn_$revPRIME_unpoly_12(uw_context ctx, struct __uws_2* __uwr_acc_0, 
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
   
  
  static uw_unit __uwn_$mapXPRIME_unpoly_unpoly_13(uw_context,
   struct __uws_2*);
   static uw_unit
    __uwn_$mapXPRIME_unpoly_unpoly_13(uw_context ctx, 
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
   __uwn_wrap_list_14(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">stored:"), 0),
                                                      (uw_begin_region(ctx), __uwn_$mapXPRIME_unpoly_unpoly_13(ctx,
                                                                              ({
                                                                               uw_io_transaction __uwt;
                                                                               struct
                                                                               __uws_2*
                                                                               __uwr;
                                                                               uw_set_could_write_db(ctx, 0);
                                                                               uw_io_transaction_begin(ctx, &__uwt);
                                                                               while (1) {
                                                                               int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                                                               if (__uwfk == 0) {
                                                                               __uwr =
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
                                                                               sqlite3_stmt *stmt = conn->p11;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_N FROM uw_Io_page_commit_t AS T_T ORDER BY T_T.uw_N", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_N FROM uw_Io_page_commit_t AS T_T ORDER BY T_T.uw_N<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p11 = stmt;
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
                                                                               "SELECT T_T.uw_N FROM uw_Io_page_commit_t AS T_T ORDER BY T_T.uw_N", sqlite3_errmsg(conn->conn));
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               acc;
                                                                               })
                                                                               ;
                                                                               __uwn_$revPRIME_unpoly_12(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               });
                                                                               uw_io_transaction_commit(ctx, &__uwt);
                                                                               break;
                                                                               } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                                                                               break;
                                                                               }
                                                                               }
                                                                               uw_io_transaction_end(ctx, &__uwt, 1);
                                                                               __uwr;
                                                                               })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_main_15(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), (uw_write(ctx, ">\n<a href=\"/Io_page_commit/list\">list</a>\n<a href=\"/Io_page_commit/busy\">busy</a>\n<a href=\"/Io_page_commit/failsBefore\">failsBefore</a>\n<a href=\"/Io_page_commit/raises\">raises</a>\n<a href=\"/Io_page_commit/conflict\">conflict</a>\n<a href=\"/Io_page_commit/raisesFatal\">raisesFatal</a>\n<a href=\"/Io_page_commit/busyFatal\">busyFatal</a>\n<a href=\"/Io_page_commit/unlock\">unlock</a>\n<a href=\"/Io_page_commit/adopted\">adopted</a>\n<a href=\"/Io_page_commit/adoptedHandled\">adoptedHandled</a>\n</body>"), 0)))));
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
 
 if (!strncmp(request, "/Io_page_commit/main", 20) && (request[20] == 0 || request[20] == '/')) {
  request += 20;
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
    __uwn_wrap_main_15(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    uw_finish_page(ctx);
    return;
   }
   }
  
  if (!strncmp(request, "/Io_page_commit/list", 20) && (request[20] == 0 || request[20] == '/')) {
   request += 20;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_list_14(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/busy", 20) && (request[20] == 0 || request[20] == '/')) {
   request += 20;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_busy_11(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/failsBefore", 27) && (request[27] == 0 || request[27] == '/')) {
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_failsBefore_10(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/raises", 22) && (request[22] == 0 || request[22] == '/')) {
   request += 22;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_raises_9(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/conflict", 24) && (request[24] == 0 || request[24] == '/')) {
   request += 24;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_conflict_8(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/raisesFatal", 27) && (request[27] == 0 || request[27] == '/')) {
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_raisesFatal_7(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/busyFatal", 25) && (request[25] == 0 || request[25] == '/')) {
   request += 25;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_busyFatal_6(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/unlock", 22) && (request[22] == 0 || request[22] == '/')) {
   request += 22;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_unlock_5(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/adopted", 23) && (request[23] == 0 || request[23] == '/')) {
   request += 23;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_adopted_4(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Io_page_commit/adoptedHandled", 30) && (request[30] == 0 || request[30] == '/')) {
   request += 30;
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_adoptedHandled_3(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
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
 