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
      if (pin_rand) { uw_pinned_vfs.xRandomness = uw_pinned_randomness; }
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
     
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Sqlcache_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Query preparation failed:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Sqlcache_t' COLLATE NOCASE");
       }
      
      while ((res = sqlite3_step(stmt)) == SQLITE_BUSY)
      sleep(1);
       
      if (res == SQLITE_DONE) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "No row returned:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Sqlcache_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Error getting row:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Sqlcache_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Bad column count:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Sqlcache_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Table 'uw_Sqlcache_t' does not exist.");
       }
      
      sqlite3_finalize(stmt);
      }
     
     static void uw_db_prepare(uw_context ctx) {
     uw_conn *conn = uw_get_db(ctx);
     
     if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Sqlcache_t (uw_Id, uw_Name, uw_Val) VALUES (\?, \?, 3)", -1, &conn->p0, NULL) != SQLITE_OK) {
      char msg[1024];
       strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
       msg[1023] = 0;
       sqlite3_close(conn->conn);
        uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Sqlcache_t (uw_Id, uw_Name, uw_Val) VALUES (\?, \?, 3)<br />%s"
        , msg);
        }
      
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))", -1, &conn->p1, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))", -1, &conn->p2, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))<br />%s"
         , msg);
         }
       }
    
    static void uw_db_init(uw_context ctx) {
    sqlite3 *sqlite;
    sqlite3_stmt *stmt;
    uw_conn *conn;
    
    char *env_db_str = getenv("URWEB_SQLITE_DB_PATH");
    const char *sqlite_db_path = env_db_str == NULL ? "sqlcache" : env_db_str;
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
       sqlite3_close(conn->conn);
    }
    
    static int uw_db_begin(uw_context ctx, int could_write) {
    uw_conn *conn = uw_get_db(ctx);
    
    if (uw_reproducible_rand_pinned()) { sqlite3_randomness(0, NULL); }
    
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
  static uw_Sqlcache_Cache cacheStruct1 = {
     .lockIn = PTHREAD_RWLOCK_INITIALIZER,
     .lockOut = PTHREAD_RWLOCK_INITIALIZER,
     .table = NULL,
     .numKeys = 0,
     .timeInvalid = 0,
     .timeNow = 0};
   static uw_Sqlcache_Cache *cache1 = &cacheStruct1;
   
   static void uw_Sqlcache_rlock1(uw_context ctx) {
     uw_Sqlcache_rlock(ctx, cache1);
   }
   
   static void uw_Sqlcache_wlock1(uw_context ctx) {
     uw_Sqlcache_wlock(ctx, cache1);
   }
   
   static uw_Basis_string uw_Sqlcache_check1(uw_context ctx) {
     char *ks[] = {};
     uw_Sqlcache_Value *v = uw_Sqlcache_check(ctx, cache1, ks);
     if (v && v->output != NULL) {
       uw_write(ctx, v->output);
       uw_write_script(ctx, v->scriptOutput);
       return v->result;
     } else {
       uw_recordingStart(ctx);
       return NULL;
     }
   }
   
   static uw_unit uw_Sqlcache_store1(uw_context ctx, uw_Basis_string s) {
     char *ks[] = {};
     uw_Sqlcache_Value *v = malloc(sizeof(uw_Sqlcache_Value));
     v->result = strdup(s);
     v->output = uw_recordingRead(ctx);
     v->scriptOutput = uw_recordingReadScript(ctx);
     uw_Sqlcache_store(ctx, cache1, ks, v);
     return uw_unit_v;
   }
   
   static uw_unit uw_Sqlcache_flush1(uw_context ctx) {
     char *ks[] = {};
     uw_Sqlcache_flush(ctx, cache1, ks);
     return uw_unit_v;
   }
   
   static uw_Sqlcache_Cache cacheStruct0 = {
      .lockIn = PTHREAD_RWLOCK_INITIALIZER,
      .lockOut = PTHREAD_RWLOCK_INITIALIZER,
      .table = NULL,
      .numKeys = 0,
      .timeInvalid = 0,
      .timeNow = 0};
    static uw_Sqlcache_Cache *cache0 = &cacheStruct0;
    
    static void uw_Sqlcache_rlock0(uw_context ctx) {
      uw_Sqlcache_rlock(ctx, cache0);
    }
    
    static void uw_Sqlcache_wlock0(uw_context ctx) {
      uw_Sqlcache_wlock(ctx, cache0);
    }
    
    static uw_Basis_string uw_Sqlcache_check0(uw_context ctx) {
      char *ks[] = {};
      uw_Sqlcache_Value *v = uw_Sqlcache_check(ctx, cache0, ks);
      if (v && v->output != NULL) {
        uw_write(ctx, v->output);
        uw_write_script(ctx, v->scriptOutput);
        return v->result;
      } else {
        uw_recordingStart(ctx);
        return NULL;
      }
    }
    
    static uw_unit uw_Sqlcache_store0(uw_context ctx, uw_Basis_string s) {
      char *ks[] = {};
      uw_Sqlcache_Value *v = malloc(sizeof(uw_Sqlcache_Value));
      v->result = strdup(s);
      v->output = uw_recordingRead(ctx);
      v->scriptOutput = uw_recordingReadScript(ctx);
      uw_Sqlcache_store(ctx, cache0, ks, v);
      return uw_unit_v;
    }
    
    static uw_unit uw_Sqlcache_flush0(uw_context ctx) {
      char *ks[] = {};
      uw_Sqlcache_flush(ctx, cache0, ks);
      return uw_unit_v;
    }
    
    
 
  
  struct __uws_1 {
   uw_Basis_string __uwf_Id;
    uw_Basis_string __uwf_Name;
     };
  struct __uws_2 {
   uw_Basis_string __uwf_Name;
    };
  struct __uws_3 {
   struct __uws_2 __uwf_T;
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
  
  /* SQL table uw_Sqlcache_t keys uw_Id constraints   */
   
  
  static uw_unit
   __uwn_wrap_add_3(uw_context ctx, struct __uws_1 __uwr_x0_0, 
                     uw_unit __uwr___1)
   {
   return((uw_begin_region(ctx), uw_Sqlcache_wlock0(ctx),
           uw_end_region(ctx), (uw_begin_region(ctx), uw_Sqlcache_wlock1(ctx),
                                uw_end_region(ctx), ({
                                                     uw_unit __uwr___2 =
                                                     (uw_begin_region(ctx), (uw_begin_region(ctx),
                                                                              (uw_begin_region(ctx),
                                                                               uw_Sqlcache_flush0(ctx
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               uw_Sqlcache_flush1(ctx
                                                                               )),
                                                                             uw_end_region(ctx),
                                                                              (uw_begin_region(ctx), ({
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               uw_Basis_stringToInt_error(ctx,
                                                                               __uwr_x0_0.__uwf_Id);
                                                                               
                                                                               uw_Basis_string
                                                                               arg2
                                                                               =
                                                                               __uwr_x0_0.__uwf_Name;
                                                                               
                                                                               uw_ensure_transaction(ctx);
                                                                               
                                                                               uw_conn *conn = uw_get_db(ctx);
                                                                               sqlite3_stmt *stmt = conn->p0;
                                                                               
                                                                               if (stmt == NULL) {
                                                                               if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Sqlcache_t (uw_Id, uw_Name, uw_Val) VALUES (\?, \?, 3)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Sqlcache_t (uw_Id, uw_Name, uw_Val) VALUES (\?, \?, 3)<br />%s", sqlite3_errmsg(conn->conn));
                                                                               conn->p0 = stmt;
                                                                               }
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                               if (
                                                                               sqlite3_bind_int64(stmt, 1, 
                                                                               arg1) != SQLITE_OK) uw_error(ctx, FATAL, ":0:0-0:0: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                                               
                                                                               if (
                                                                               sqlite3_bind_text(stmt, 2, 
                                                                               arg2, -1, SQLITE_TRANSIENT) != SQLITE_OK) uw_error(ctx, FATAL, ":0:0-0:0: Error binding parameter #2: %s", sqlite3_errmsg(conn->conn));
                                                                               
                                                                               int r;
                                                                               if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                               sleep(1);
                                                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                               }
                                                                               
                                                                               if (r != SQLITE_DONE) 
                                                                               uw_error(ctx, FATAL, ":0:0-0:0: DML step failed: %s<br />%s", 
                                                                               "INSERT INTO uw_Sqlcache_t (uw_Id, uw_Name, uw_Val) VALUES (\?, \?, 3)", sqlite3_errmsg(conn->conn));
                                                                               else if (uw_dml_log_enabled()) {
                                                                               char *expanded = sqlite3_expanded_sql(stmt);
                                                                               uw_log_dml(ctx, ":0:0-0:0", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                               sqlite3_free(expanded);
                                                                               }
                                                                               uw_pop_cleanup(ctx);
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               
                                                                               uw_end_region(ctx);
                                                                               0;
                                                                               }))));
                                                     uw_end_region(ctx);
                                                      ({
                                                       uw_Basis_string disc =
                                                       uw_Sqlcache_check0(ctx);
                                                       
                                                       disc == NULL ?
                                                        ({
                                                         uw_Basis_source
                                                         __uwr_s_3 =
                                                         uw_Basis_new_client_source(ctx,
                                                          ({
                                                           uw_Basis_string arg0
                                                            = "{c:\"c\",v:";
                                                            
                                                            uw_Basis_string
                                                             arg1 =
                                                             uw_Basis_htmlifyInt(ctx,
                                                              2LL);
                                                             
                                                            uw_Basis_string
                                                             arg2 = "}";
                                                             uw_Basis_mstrcat(ctx, 
                                                           arg0, arg1, arg2,
                                                                            NULL);
                                                           }));
                                                         ({
                                                          uw_unit __uwr_q_4 =
                                                          (uw_begin_region(ctx),
                                                           ((uw_write(ctx, "<body"), 0),
                                                            (uw_begin_region(ctx),
                                                              (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                                              uw_Basis_get_settings(ctx,
                                                                               0
                                                                               )
                                                                              )), 0),
                                                             uw_end_region(ctx),
                                                              (uw_begin_region(ctx),
                                                                (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                               ""
                                                                               )), 0),
                                                               uw_end_region(ctx),
                                                                ((uw_write(ctx, 
                                                                  ">\n"), 0),
                                                                 (uw_begin_region(ctx),
                                                                   (uw_begin_region(ctx), ({
                                                                    uw_unit acc
                                                                    =
                                                                    0;
                                                                    int dummy = (uw_begin_region(ctx), 0);
                                                                    uw_ensure_transaction(ctx);
                                                                    
                                                                     
                                                                     uw_conn *conn = uw_get_db(ctx);
                                                                      sqlite3_stmt *stmt = conn->p1;
                                                                       
                                                                       if (stmt == NULL) {
                                                                       if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))<br />%s", sqlite3_errmsg(conn->conn));
                                                                      conn->p1 = stmt;
                                                                       }
                                                                       
                                                                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                       
                                                                      
                                                                      int r;
                                                                       sqlite3_reset(stmt);
                                                                       uw_end_region(ctx);
                                                                       while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                                                       struct __uws_3 __uwr_r_4;
                                                                        uw_unit
                                                                        __uwr_acc_5
                                                                        =
                                                                        acc;
                                                                        
                                                                        __uwr_r_4.__uwf_T.__uwf_Name
                                                                         =
                                                                         (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                          ({uw_Basis_string
                                                                           tmp;
                                                                           uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:25:3-30:8: Unexpectedly NULL field #0");
                                                                           tmp;
                                                                           }) : 
                                                                          (uw_Basis_string)sqlite3_column_text(stmt, 0));
                                                                         
                                                                        
                                                                        acc =
                                                                        ((uw_write(ctx, 
                                                                          "<div>"), 0),
                                                                         (uw_begin_region(ctx),
                                                                           uw_Basis_htmlifyString_w(ctx,
                                                                            __uwr_r_4.__uwf_T.__uwf_Name
                                                                            ),
                                                                          uw_end_region(ctx),
                                                                           (uw_write(ctx, 
                                                                            "</div>"), 0)));
                                                                        }
                                                                       
                                                                       if (r == SQLITE_BUSY) {
                                                                       sleep(1);
                                                                        uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                        }
                                                                       
                                                                       if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:25:3-30:8: query step failed: %s<br />%s", 
                                                                       "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))", sqlite3_errmsg(conn->conn));
                                                                       
                                                                       uw_pop_cleanup(ctx);
                                                                      uw_pop_cleanup(ctx);
                                                                       
                                                                    uw_end_region(ctx);
                                                                     acc;
                                                                    })),
                                                                  uw_end_region(ctx),
                                                                   ((uw_write(ctx, 
                                                                     "\n"), 0),
                                                                    (uw_begin_region(ctx),
                                                                      (uw_begin_region(ctx), ({
                                                                       uw_unit
                                                                       acc =
                                                                       0;
                                                                       int dummy = (uw_begin_region(ctx), 0);
                                                                       uw_ensure_transaction(ctx);
                                                                       
                                                                        
                                                                        uw_conn *conn = uw_get_db(ctx);
                                                                         sqlite3_stmt *stmt = conn->p2;
                                                                          
                                                                          if (stmt == NULL) {
                                                                          if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))<br />%s", sqlite3_errmsg(conn->conn));
                                                                         conn->p2 = stmt;
                                                                          }
                                                                          
                                                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                          
                                                                         
                                                                         int r;
                                                                          sqlite3_reset(stmt);
                                                                          uw_end_region(ctx);
                                                                          while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                                                          struct __uws_3 __uwr_r_4;
                                                                           uw_unit
                                                                           __uwr_acc_5
                                                                           =
                                                                           acc;
                                                                           
                                                                           __uwr_r_4.__uwf_T.__uwf_Name
                                                                            =
                                                                            (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                             ({uw_Basis_string
                                                                              tmp;
                                                                              uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:26:3-30:8: Unexpectedly NULL field #0");
                                                                              tmp;
                                                                              }) : 
                                                                             (uw_Basis_string)sqlite3_column_text(stmt, 0));
                                                                            
                                                                           
                                                                           acc
                                                                           =
                                                                           ((uw_write(ctx, 
                                                                             "<div>"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_r_4.__uwf_T.__uwf_Name
                                                                               ),
                                                                             uw_end_region(ctx),
                                                                              (uw_write(ctx, 
                                                                               "</div>"), 0)));
                                                                           }
                                                                          
                                                                          if (r == SQLITE_BUSY) {
                                                                          sleep(1);
                                                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                           }
                                                                          
                                                                          if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:26:3-30:8: query step failed: %s<br />%s", 
                                                                          "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))", sqlite3_errmsg(conn->conn));
                                                                          
                                                                          uw_pop_cleanup(ctx);
                                                                         uw_pop_cleanup(ctx);
                                                                          
                                                                       uw_end_region(ctx);
                                                                        acc;
                                                                       })),
                                                                     uw_end_region(ctx),
                                                                      ((uw_write(ctx, 
                                                                        "\n<form method=\"post\" action=\"/Sqlcache/add\"><input type=\"text\" name=\"Id\" /><input type=\"text\" name=\"Name\" /><input type=\"submit\" /></form>\n<button onclick='uw_event=event;exec({c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                       (uw_begin_region(ctx),
                                                                         uw_Basis_htmlifySource_w(ctx,
                                                                          __uwr_s_3
                                                                          ),
                                                                        uw_end_region(ctx),
                                                                         ((uw_write(ctx, 
                                                                           "}},x:{c:\"c\",v:null}})'>Dot</button>\n<script type=\"text/javascript\">dyn(\"span\", execD({c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                          (uw_begin_region(ctx),
                                                                            uw_Basis_htmlifySource_w(ctx,
                                                                             __uwr_s_3
                                                                             ),
                                                                           uw_end_region(ctx),
                                                                            (uw_write(ctx, 
                                                                             "}},x:{c:\"c\",v:null}}))</script>\n</body>"), 0)))))))))))));
                                                          uw_end_region(ctx);
                                                           (uw_begin_region(ctx),
                                                             uw_Sqlcache_store0(ctx,
                                                              "_"),
                                                            uw_end_region(ctx), __uwr_q_4);
                                                          });
                                                         })
                                                         :
                                                        disc != NULL && 1 ?
                                                         ({uw_Basis_string
                                                            __uwr_hit_3 = disc;
                                                            ({
                                                             uw_Basis_string request = uw_maybe_strdup(ctx, __uwr_hit_3);
                                                             
                                                             uw_Basis_unurlifyUnit(ctx, &request);
                                                             });
                                                          })
                                                          :
                                                         ({
                                                          uw_unit
                                                          tmp;
                                                          uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:16:0-31:3: pattern match failure");
                                                          tmp;
                                                          });
                                                       });
                                                     }))));
   }
  
  static uw_unit
   __uwn_wrap_main_4(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return((uw_begin_region(ctx), uw_Sqlcache_rlock1(ctx),
           uw_end_region(ctx), ({
                                uw_Basis_string disc =
                                uw_Sqlcache_check1(ctx);
                                
                                disc == NULL ?
                                 ({
                                  uw_Basis_source __uwr_s_2 =
                                  uw_Basis_new_client_source(ctx,
                                   ({
                                    uw_Basis_string arg0 = "{c:\"c\",v:";
                                     
                                     uw_Basis_string arg1 =
                                      uw_Basis_htmlifyInt(ctx, 2LL);
                                      uw_Basis_string arg2 = "}";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    }));
                                  ({
                                   uw_unit __uwr_q_3 =
                                   (uw_begin_region(ctx), ((uw_write(ctx, "<body"), 0),
                                                           (uw_begin_region(ctx),
                                                             (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                                             uw_Basis_get_settings(ctx,
                                                                              0)
                                                                             )), 0),
                                                            uw_end_region(ctx), 
                                                            (uw_begin_region(ctx),
                                                              (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                              ""
                                                                              )), 0),
                                                             uw_end_region(ctx),
                                                              ((uw_write(ctx, ">\n"), 0),
                                                               (uw_begin_region(ctx),
                                                                 (uw_begin_region(ctx), ({
                                                                  uw_unit acc =
                                                                  0;
                                                                  int dummy = (uw_begin_region(ctx), 0);
                                                                  uw_ensure_transaction(ctx);
                                                                  
                                                                   
                                                                   uw_conn *conn = uw_get_db(ctx);
                                                                    sqlite3_stmt *stmt = conn->p1;
                                                                     
                                                                     if (stmt == NULL) {
                                                                     if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))<br />%s", sqlite3_errmsg(conn->conn));
                                                                    conn->p1 = stmt;
                                                                     }
                                                                     
                                                                     uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                     uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                     
                                                                    
                                                                    int r;
                                                                     sqlite3_reset(stmt);
                                                                     uw_end_region(ctx);
                                                                     while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                                                     struct __uws_3 __uwr_r_3;
                                                                      uw_unit
                                                                      __uwr_acc_4
                                                                      =
                                                                      acc;
                                                                      
                                                                      __uwr_r_3.__uwf_T.__uwf_Name
                                                                       =
                                                                       (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                        ({uw_Basis_string
                                                                         tmp;
                                                                         uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:25:3-30:8: Unexpectedly NULL field #0");
                                                                         tmp;
                                                                         }) : (uw_Basis_string)sqlite3_column_text(stmt, 0));
                                                                       
                                                                      
                                                                      acc =
                                                                      ((uw_write(ctx, 
                                                                        "<div>"), 0),
                                                                       (uw_begin_region(ctx),
                                                                         uw_Basis_htmlifyString_w(ctx,
                                                                          __uwr_r_3.__uwf_T.__uwf_Name
                                                                          ),
                                                                        uw_end_region(ctx),
                                                                         (uw_write(ctx, 
                                                                          "</div>"), 0)));
                                                                      }
                                                                     
                                                                     if (r == SQLITE_BUSY) {
                                                                     sleep(1);
                                                                      uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                      }
                                                                     
                                                                     if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:25:3-30:8: query step failed: %s<br />%s", 
                                                                     "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM 3))", sqlite3_errmsg(conn->conn));
                                                                     
                                                                     uw_pop_cleanup(ctx);
                                                                    uw_pop_cleanup(ctx);
                                                                     
                                                                  uw_end_region(ctx);
                                                                   acc;
                                                                  })),
                                                                uw_end_region(ctx),
                                                                 ((uw_write(ctx, 
                                                                   "\n"), 0),
                                                                  (uw_begin_region(ctx),
                                                                    (uw_begin_region(ctx), ({
                                                                     uw_unit
                                                                     acc =
                                                                     0;
                                                                     int dummy = (uw_begin_region(ctx), 0);
                                                                     uw_ensure_transaction(ctx);
                                                                     
                                                                      
                                                                      uw_conn *conn = uw_get_db(ctx);
                                                                       sqlite3_stmt *stmt = conn->p2;
                                                                        
                                                                        if (stmt == NULL) {
                                                                        if (sqlite3_prepare_v2(conn->conn, "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))<br />%s", sqlite3_errmsg(conn->conn));
                                                                       conn->p2 = stmt;
                                                                        }
                                                                        
                                                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                        
                                                                       
                                                                       int r;
                                                                        sqlite3_reset(stmt);
                                                                        uw_end_region(ctx);
                                                                        while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                                                        struct __uws_3 __uwr_r_3;
                                                                         uw_unit
                                                                         __uwr_acc_4
                                                                         =
                                                                         acc;
                                                                         
                                                                         __uwr_r_3.__uwf_T.__uwf_Name
                                                                          =
                                                                          (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                           ({uw_Basis_string
                                                                            tmp;
                                                                            uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:26:3-30:8: Unexpectedly NULL field #0");
                                                                            tmp;
                                                                            }) : 
                                                                           (uw_Basis_string)sqlite3_column_text(stmt, 0));
                                                                          
                                                                         
                                                                         acc =
                                                                         ((uw_write(ctx, 
                                                                           "<div>"), 0),
                                                                          (uw_begin_region(ctx),
                                                                            uw_Basis_htmlifyString_w(ctx,
                                                                             __uwr_r_3.__uwf_T.__uwf_Name
                                                                             ),
                                                                           uw_end_region(ctx),
                                                                            (uw_write(ctx, 
                                                                             "</div>"), 0)));
                                                                         }
                                                                        
                                                                        if (r == SQLITE_BUSY) {
                                                                        sleep(1);
                                                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                         }
                                                                        
                                                                        if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/sql/sqlcache.ur:26:3-30:8: query step failed: %s<br />%s", 
                                                                        "SELECT T_T.uw_Name FROM uw_Sqlcache_t AS T_T WHERE ((T_T.uw_Val IS NOT DISTINCT FROM NULL))", sqlite3_errmsg(conn->conn));
                                                                        
                                                                        uw_pop_cleanup(ctx);
                                                                       uw_pop_cleanup(ctx);
                                                                        
                                                                     uw_end_region(ctx);
                                                                      acc;
                                                                     })),
                                                                   uw_end_region(ctx),
                                                                    ((uw_write(ctx, 
                                                                      "\n<form method=\"post\" action=\"/Sqlcache/add\"><input type=\"text\" name=\"Id\" /><input type=\"text\" name=\"Name\" /><input type=\"submit\" /></form>\n<button onclick='uw_event=event;exec({c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                     (uw_begin_region(ctx),
                                                                       uw_Basis_htmlifySource_w(ctx,
                                                                        __uwr_s_2
                                                                        ),
                                                                      uw_end_region(ctx),
                                                                       ((uw_write(ctx, 
                                                                         "}},x:{c:\"c\",v:null}})'>Dot</button>\n<script type=\"text/javascript\">dyn(\"span\", execD({c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                        (uw_begin_region(ctx),
                                                                          uw_Basis_htmlifySource_w(ctx,
                                                                           __uwr_s_2
                                                                           ),
                                                                         uw_end_region(ctx),
                                                                          (uw_write(ctx, 
                                                                           "}},x:{c:\"c\",v:null}}))</script>\n</body>"), 0)))))))))))));
                                   uw_end_region(ctx);
                                    (uw_begin_region(ctx), uw_Sqlcache_store1(ctx,
                                                            "_"),
                                     uw_end_region(ctx), __uwr_q_3);
                                   });
                                  })
                                  :
                                 disc != NULL && 1 ?
                                  ({uw_Basis_string __uwr_hit_2 = disc;
                                     ({
                                      uw_Basis_string request = uw_maybe_strdup(ctx, __uwr_hit_2);
                                      
                                      uw_Basis_unurlifyUnit(ctx, &request);
                                      });
                                   })
                                   :
                                  ({
                                   uw_unit
                                   tmp;
                                   uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
                                   tmp;
                                   });
                                })));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0]) {
  case 'I':
   return 0;
   case 'N':
    return 1;
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
  
  
  if (!strcmp(request, "/app.48A12728A0A763F724E97FCB1173AB1D8B66A37C.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 
 if (!strncmp(request, "/Sqlcache/main", 14) && (request[14] == 0 || request[14] == '/')) {
  request += 14;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.48A12728A0A763F724E97FCB1173AB1D8B66A37C.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_4(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Sqlcache/add", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
   if (*request == '/') ++request;
   uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
    uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
     uw_write(ctx, uw_begin_html5);
    uw_mayReturnIndirectly(ctx);
    uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.48A12728A0A763F724E97FCB1173AB1D8B66A37C.js\"></script>\n");
    uw_set_could_write_db(ctx, 1);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_Basis_string uw_input_Id;
     uw_Basis_string uw_input_Name;
      
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input Id");
      uw_input_Id = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input Name");
       uw_input_Name = uw_Basis_unurlifyString_fromClient(ctx, &request);
       struct __uws_1 uw_inputs = {
        uw_input_Id,
         uw_input_Name,
          };
     __uwn_wrap_add_3(ctx, uw_inputs, 0);
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
 uw_app uw_application = {2,
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
                                                                  NULL};
 