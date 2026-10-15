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
           sqlite3_stmt *p6;
            sqlite3_stmt *p7;
             sqlite3_stmt *p8;
              sqlite3_stmt *p9;
               sqlite3_stmt *p10;
                sqlite3_stmt *p11;
                 sqlite3_stmt *p12;
                  sqlite3_stmt *p13;
                   sqlite3_stmt *p14;
                    sqlite3_stmt *p15;
                     sqlite3_stmt *p16;
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
      
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Rpc_types_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Query preparation failed:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Rpc_types_t' COLLATE NOCASE");
       }
      
      while ((res = sqlite3_step(stmt)) == SQLITE_BUSY)
      sleep(1);
       
      if (res == SQLITE_DONE) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "No row returned:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Rpc_types_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Error getting row:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Rpc_types_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Bad column count:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Rpc_types_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Table 'uw_Rpc_types_t' does not exist.");
       }
      
      sqlite3_finalize(stmt);
      
     validated = 1;
     }
     
     static void uw_db_prepare(uw_context ctx) {
     uw_conn *conn = uw_get_db(ctx);
     
     if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'viaAlias')", -1, &conn->p0, NULL) != SQLITE_OK) {
      char msg[1024];
       strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
       msg[1023] = 0;
       sqlite3_close(conn->conn);
        uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'viaAlias')<br />%s"
        , msg);
        }
      
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'wrapped')", -1, &conn->p1, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'wrapped')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)", -1, &conn->p2, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'min')", -1, &conn->p3, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'min')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'max')", -1, &conn->p4, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'max')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT MIN(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'min')", -1, &conn->p5, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT MIN(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'min')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT MAX(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'max')", -1, &conn->p6, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT MAX(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'max')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)", -1, &conn->p7, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = \?)", -1, &conn->p8, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = \?)<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'unit')", -1, &conn->p9, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'unit')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'opt')", -1, &conn->p10, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'opt')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'list')", -1, &conn->p11, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_finalize(conn->p10);
                   sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'list')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, 'color')", -1, &conn->p12, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_finalize(conn->p10);
                   sqlite3_finalize(conn->p11);
                    sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, 'color')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'xb')", -1, &conn->p13, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_finalize(conn->p10);
                   sqlite3_finalize(conn->p11);
                    sqlite3_finalize(conn->p12);
                     sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'xb')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'tryNone')", -1, &conn->p14, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_finalize(conn->p10);
                   sqlite3_finalize(conn->p11);
                    sqlite3_finalize(conn->p12);
                     sqlite3_finalize(conn->p13);
                      sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'tryNone')<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Rpc_types_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", -1, &conn->p15, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_finalize(conn->p10);
                   sqlite3_finalize(conn->p11);
                    sqlite3_finalize(conn->p12);
                     sqlite3_finalize(conn->p13);
                      sqlite3_finalize(conn->p14);
                       sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Rpc_types_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S<br />%s"
         , msg);
         }
       
      
      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'partial')", -1, &conn->p16, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_finalize(conn->p1);
          sqlite3_finalize(conn->p2);
           sqlite3_finalize(conn->p3);
            sqlite3_finalize(conn->p4);
             sqlite3_finalize(conn->p5);
              sqlite3_finalize(conn->p6);
               sqlite3_finalize(conn->p7);
                sqlite3_finalize(conn->p8);
                 sqlite3_finalize(conn->p9);
                  sqlite3_finalize(conn->p10);
                   sqlite3_finalize(conn->p11);
                    sqlite3_finalize(conn->p12);
                     sqlite3_finalize(conn->p13);
                      sqlite3_finalize(conn->p14);
                       sqlite3_finalize(conn->p15);
                        sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'partial')<br />%s"
         , msg);
         }
       }
    
    static void uw_db_init(uw_context ctx) {
    sqlite3 *sqlite;
    sqlite3_stmt *stmt;
    uw_conn *conn;
    
    char *env_db_str = getenv("URWEB_SQLITE_DB_PATH");
    const char *sqlite_db_path = env_db_str == NULL ? "rpc_types.db" : env_db_str;
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
          if (conn->p6) sqlite3_finalize(conn->p6);
           if (conn->p7) sqlite3_finalize(conn->p7);
            if (conn->p8) sqlite3_finalize(conn->p8);
             if (conn->p9) sqlite3_finalize(conn->p9);
              if (conn->p10) sqlite3_finalize(conn->p10);
               if (conn->p11) sqlite3_finalize(conn->p11);
                if (conn->p12) sqlite3_finalize(conn->p12);
                 if (conn->p13) sqlite3_finalize(conn->p13);
                  if (conn->p14) sqlite3_finalize(conn->p14);
                   if (conn->p15) sqlite3_finalize(conn->p15);
                    if (conn->p16) sqlite3_finalize(conn->p16);
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
   uw_Basis_string __uwf_1;
    uw_Basis_int* __uwf_2;
     };
  
  enum __uwe_color_3 { __uwc_Red_4, __uwc_Green_5, __uwc_Blue_6 };
   
   struct __uwd_color_3 {
   enum __uwe_color_3
   tag;
   union {
    uw_Basis_int uw_Green;
     struct __uws_1 uw_Blue;
    } data;
    };
  struct __uws_2 {
   uw_Basis_string __uwf_1;
    struct __uws_2* __uwf_2;
     };
  
  struct __uws_3
   {
   uw_Basis_int __uwf_A;
    uw_Basis_string __uwf_B;
     uw_Basis_float* __uwf_C;
      };
  struct __uws_4 {
   uw_Basis_int __uwf_1;
    uw_Basis_int __uwf_2;
     };
  struct __uws_5 {
   uw_Basis_int* __uwf_1;
    };
  struct __uws_6 {
   uw_Basis_int __uwf_1;
    };
  struct __uws_7 {
   uw_Basis_int __uwf_1;
    struct __uws_7* __uwf_2;
     };
  struct __uws_8 {
   uw_Basis_string __uwf_S;
    };
  
  struct __uws_9
   {
   uw_Basis_int __uwf_C;
    uw_Basis_int* __uwf_Sum;
     struct __uws_8 __uwf_T;
      };
  struct __uws_10 {
   struct __uws_9 __uwf_1;
    struct __uws_10* __uwf_2;
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
  
  /* SQL table uw_Rpc_types_t constraints   */
   
  
  static uw_Basis_int
   __uwn_viaAlias_7(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
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
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'viaAlias')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'viaAlias')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p0 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:12:20-12:89: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:12:20-12:89: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'viaAlias')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:12:20-12:89", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
   }
  
  static uw_Basis_int
   __uwn_wrapped_8(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
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
                                        sqlite3_stmt *stmt = conn->p1;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'wrapped')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'wrapped')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p1 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:15:20-15:88: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:15:20-15:88: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'wrapped')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:15:20-15:88", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                          sqlite3_free(expanded);
                                          }
                                         uw_pop_cleanup(ctx);
                                        uw_pop_cleanup(ctx);
                                        
                                      
                                      uw_end_region(ctx);
                                      0;
                                      })));
              uw_end_region(ctx);
               __uwr_n_0 + 2LL;
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
  
  static uw_Basis_int __uwn_$lengthPRIME_unpoly_9(uw_context,
   uw_Basis_int, struct __uws_2*);
   static uw_Basis_int
    __uwn_$lengthPRIME_unpoly_9(uw_context ctx, uw_Basis_int __uwr_acc_0, 
                                 struct __uws_2* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_string __uwr___2 = (*disc).__uwf_1;
                 struct __uws_2* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_Basis_int rearg0 = __uwr_acc_0 + 1LL;
                    struct __uws_2* rearg1 = __uwr_ls_3;
                   uw_Basis_int
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "$/list.ur:72:24-75:16: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_string __uwn_$foldlPRIME_unpoly_unpoly_10(uw_context,
   uw_Basis_string, struct __uws_2*);
   static uw_Basis_string
    __uwn_$foldlPRIME_unpoly_unpoly_10(uw_context ctx, 
                                        uw_Basis_string __uwr_acc_0, 
                                        struct __uws_2* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_string __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_2* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_Basis_string rearg0 =
                    ({
                     uw_Basis_string arg0 = __uwr_acc_0;
                      uw_Basis_string arg1 = __uwr_x_2;
                       uw_Basis_string arg2 = ",";
                        uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                     });
                    struct __uws_2* rearg1 = __uwr_ls_3;
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
   __uwn_six_11(uw_context ctx, uw_Basis_int __uwr_a_0, 
                 uw_Basis_string __uwr_b_1, uw_Basis_float __uwr_c_2, 
                 uw_Basis_int* __uwr_d_3, struct __uws_2* __uwr_e_4, 
                 struct __uwd_color_3* __uwr_f_5, uw_unit __uwr___6)
   {
   return(({
           uw_unit __uwr___7 =
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
                                      uw_Basis_int arg1 = __uwr_a_0;
                                       uw_Basis_string arg2 = __uwr_b_1;
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p2;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p2 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:21:4-21:69: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                         if (sqlite3_bind_text(stmt, 2, arg2, -1, SQLITE_TRANSIENT) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:21:4-21:69: Error binding parameter #2: %s", sqlite3_errmsg(conn->conn));
                                          
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:21:4-21:69: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:21:4-21:69", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
             uw_Basis_string arg0 = uw_Basis_intToString(ctx, __uwr_a_0);
              uw_Basis_string arg1 = "|";
               uw_Basis_string arg2 = __uwr_b_1;
                uw_Basis_string arg3 = "|";
                 uw_Basis_string arg4 = uw_Basis_floatToString(ctx, __uwr_c_2);
                  uw_Basis_string arg5 = "|";
                   
              uw_Basis_string arg6 =
               ({
                uw_Basis_int* disc =
                __uwr_d_3;
                
                disc == NULL ? ""
                  :
                 disc != NULL && 1 ?
                  ({uw_Basis_int __uwr_x_8 = (*disc);
                     uw_Basis_intToString(ctx, __uwr_x_8);
                   })
                   :
                  ({
                   uw_Basis_string
                   tmp;
                   uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:22:52-22:58: pattern match failure");
                   tmp;
                   });
                });
               uw_Basis_string arg7 = "|";
                
              uw_Basis_string arg8 =
               uw_Basis_intToString(ctx,
                ({
                 uw_Basis_int arg0 = 0LL;
                  struct __uws_2* arg1 = __uwr_e_4;
                 __uwn_$lengthPRIME_unpoly_9(ctx, arg0, arg1);
                 }));
               uw_Basis_string arg9 = ":";
                
              uw_Basis_string arg10 =
               ({
                uw_Basis_string arg0 = "";
                 struct __uws_2* arg1 = __uwr_e_4;
                __uwn_$foldlPRIME_unpoly_unpoly_10(ctx, arg0, arg1);
                });
               uw_Basis_string arg11 = "|";
                
              uw_Basis_string arg12 =
               ({
                struct __uwd_color_3* disc =
                __uwr_f_5;
                
                disc->tag == __uwc_Red_4 ? "Red"
                  :
                 disc->tag == __uwc_Green_5 && 1 ?
                  ({uw_Basis_int __uwr_n_8 = disc->data.uw_Green;
                     ({
                      uw_Basis_string arg0 = "Green ";
                       
                       uw_Basis_string arg1 =
                        uw_Basis_intToString(ctx, __uwr_n_8);
                        uw_Basis_strcat(ctx, arg0, arg1);
                      });
                   })
                   :
                  disc->tag == __uwc_Blue_6 && 1 && 1 ?
                   ({uw_Basis_string __uwr_s_8 = disc->data.uw_Blue.__uwf_1;
                      uw_Basis_int* __uwr_o_9 = disc->data.uw_Blue.__uwf_2;
                       ({
                        uw_Basis_string arg0 = "Blue ";
                         uw_Basis_string arg1 = __uwr_s_8;
                          uw_Basis_string arg2 = " ";
                           
                         uw_Basis_string arg3 =
                          ({
                           uw_Basis_int* disc =
                           __uwr_o_9;
                           
                           disc == NULL ? ""
                             :
                            disc != NULL && 1 ?
                             ({uw_Basis_int __uwr_x_10 = (*disc);
                                uw_Basis_intToString(ctx, __uwr_x_10);
                              })
                              :
                             ({
                              uw_Basis_string
                              tmp;
                              uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:23:104-23:110: pattern match failure");
                              tmp;
                              });
                           });
                          uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, NULL);
                        });
                    })
                    :
                   ({
                    uw_Basis_string
                    tmp;
                    uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:23:12-23:111: pattern match failure");
                    tmp;
                    });
                });
               uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, arg5, arg6, 
                                      arg7, arg8, arg9, arg10, arg11, arg12,
                                                                            NULL);
             });
           }));
   }
  
  static struct __uws_3
   __uwn_rec1_12(uw_context ctx, struct __uws_3 __uwr_r_0, uw_unit __uwr___1)
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
                                      uw_Basis_int arg1 = __uwr_r_0.__uwf_A;
                                       
                                       uw_Basis_string arg2 =
                                        __uwr_r_0.__uwf_B;
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p2;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p2 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:26:4-26:73: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                         if (sqlite3_bind_text(stmt, 2, arg2, -1, SQLITE_TRANSIENT) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:26:4-26:73: Error binding parameter #2: %s", sqlite3_errmsg(conn->conn));
                                          
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:26:4-26:73: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, \?)", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:26:4-26:73", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
            ({ struct __uws_3 tmp =
             {__uwr_r_0.__uwf_A + 1LL, 
               ({
                uw_Basis_string arg0 = __uwr_r_0.__uwf_B;
                 uw_Basis_string arg1 = "!";
                  uw_Basis_strcat(ctx, arg0, arg1);
                }), 
               ({
                uw_Basis_float* disc =
                __uwr_r_0.__uwf_C;
                
                disc == NULL ? NULL
                  :
                 disc != NULL && 1 ?
                  ({uw_Basis_float __uwr_y_3 = (*disc);
                     ({
                      uw_Basis_float *tmp =
                      uw_malloc(ctx, sizeof(uw_Basis_float));
                      *tmp = __uwr_y_3 + 1;
                      tmp;
                      });
                   })
                   :
                  ({
                   uw_Basis_float*
                   tmp;
                   uw_error(ctx, FATAL, "$/option.ur:39:4-43:0: pattern match failure");
                   tmp;
                   });
                })}; tmp; });
           }));
   }
  
  static struct __uws_4
   __uwn_minmax_13(uw_context ctx, uw_Basis_int __uwr_a_0, 
                    uw_Basis_int __uwr_b_1, uw_unit __uwr___2)
   {
   return(({
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
                                      (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                              uw_Basis_int arg1
                                                               = __uwr_a_0;
                                                               
                                                               uw_ensure_transaction(ctx);
                                                               
                                                               uw_conn *conn = uw_get_db(ctx);
                                                                sqlite3_stmt *stmt = conn->p3;
                                                                
                                                                if (stmt == NULL) {
                                                                if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'min')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'min')<br />%s", sqlite3_errmsg(conn->conn));
                                                                 conn->p3 = stmt;
                                                                 }
                                                                uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                                uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                                if (sqlite3_bind_int64(stmt, 1, 
                                                                     arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:30:20-30:118: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                                 
                                                                int r;
                                                                 if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                                 uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                  }
                                                                 
                                                                 if (r != SQLITE_DONE) 
                                                                 uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:30:20-30:118: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'min')", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                                                 char *expanded = sqlite3_expanded_sql(stmt);
                                                                  uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:30:20-30:118", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                                                  sqlite3_free(expanded);
                                                                  }
                                                                 uw_pop_cleanup(ctx);
                                                                uw_pop_cleanup(ctx);
                                                                
                                                              
                                                              uw_end_region(ctx);
                                                              0;
                                                              })));
                                      uw_end_region(ctx);
                                       (uw_begin_region(ctx), ({
                                        uw_Basis_int arg1 = __uwr_b_1;
                                         
                                         uw_ensure_transaction(ctx);
                                         
                                         uw_conn *conn = uw_get_db(ctx);
                                          sqlite3_stmt *stmt = conn->p4;
                                          
                                          if (stmt == NULL) {
                                          if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'max')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'max')<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p4 = stmt;
                                           }
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                          uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                          if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:30:20-30:118: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                           
                                          int r;
                                           if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                           uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                            }
                                           
                                           if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:30:20-30:118: DML step failed: %s<br />%s", 
                                                                  "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'max')", sqlite3_errmsg(conn->conn));
                                                                  else if (uw_dml_log_enabled()) {
                                           char *expanded = sqlite3_expanded_sql(stmt);
                                            uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:30:20-30:118", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
             struct __uws_4
             __uwr;
             uw_set_could_write_db(ctx, 0);
             uw_io_transaction_begin(ctx, &__uwt);
             while (1) {
             int __uwfk = setjmp(*uw_jmp_buf(ctx));
              if (__uwfk == 0) {
              __uwr =
               ({
                uw_Basis_int* __uwr_mn_4 =
                ({
                 struct __uws_5* disc =
                 ({
                  struct __uws_5* acc =
                  NULL;
                  int dummy = (uw_begin_region(ctx), 0);
                  uw_ensure_transaction(ctx);
                  
                   
                   uw_conn *conn = uw_get_db(ctx);
                    sqlite3_stmt *stmt = conn->p5;
                     
                     if (stmt == NULL) {
                     if (sqlite3_prepare_v2(conn->conn, "SELECT MIN(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'min')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT MIN(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'min')<br />%s", sqlite3_errmsg(conn->conn));
                    conn->p5 = stmt;
                     }
                     
                     uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                     uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                     
                    
                    int r;
                     sqlite3_reset(stmt);
                     uw_end_region(ctx);
                     while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                     struct __uws_5 __uwr_r_4;
                      struct __uws_5* __uwr_acc_5 =
                      acc;
                      
                      __uwr_r_4.__uwf_1 =
                       (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? NULL : ({
                                                                              uw_Basis_int
                                                                              *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
                                                                              *tmp = 
                                                                              (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               sqlite3_column_int64(stmt, 0));
                                                                              tmp;
                                                                              }));
                       
                      
                      acc =
                      ({
                       struct __uws_5 *tmp =
                       uw_malloc(ctx, sizeof(struct __uws_5));
                       *tmp = __uwr_r_4;
                       tmp;
                       });
                      }
                     
                     if (r == SQLITE_BUSY) {
                     uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                      }
                     
                     if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: query step failed: %s<br />%s", 
                     "SELECT MIN(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'min')", sqlite3_errmsg(conn->conn));
                     
                     uw_pop_cleanup(ctx);
                    uw_pop_cleanup(ctx);
                     
                  acc;
                  });
                 
                 disc == NULL ?
                  ({
                   uw_Basis_int*
                   tmp;
                   uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", "Query returned no rows");
                   tmp;
                   })
                   :
                  disc != NULL && 1 ?
                   ({struct __uws_5 __uwr_r_4 = (*disc);
                      __uwr_r_4.__uwf_1;
                    })
                    :
                   ({
                    uw_Basis_int*
                    tmp;
                    uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:31:20-33:61: pattern match failure");
                    tmp;
                    });
                 });
                ({
                 uw_Basis_int* __uwr_mx_5 =
                 ({
                  struct __uws_5* disc =
                  ({
                   struct __uws_5* acc =
                   NULL;
                   int dummy = (uw_begin_region(ctx), 0);
                   uw_ensure_transaction(ctx);
                   
                    
                    uw_conn *conn = uw_get_db(ctx);
                     sqlite3_stmt *stmt = conn->p6;
                      
                      if (stmt == NULL) {
                      if (sqlite3_prepare_v2(conn->conn, "SELECT MAX(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'max')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT MAX(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'max')<br />%s", sqlite3_errmsg(conn->conn));
                     conn->p6 = stmt;
                      }
                      
                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                      
                     
                     int r;
                      sqlite3_reset(stmt);
                      uw_end_region(ctx);
                      while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                      struct __uws_5 __uwr_r_5;
                       struct __uws_5* __uwr_acc_6 =
                       acc;
                       
                       __uwr_r_5.__uwf_1 =
                        (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? NULL : ({
                                                                               uw_Basis_int
                                                                               *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
                                                                               *tmp = 
                                                                               (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               sqlite3_column_int64(stmt, 0));
                                                                               tmp;
                                                                               }));
                        
                       
                       acc =
                       ({
                        struct __uws_5 *tmp =
                        uw_malloc(ctx, sizeof(struct __uws_5));
                        *tmp = __uwr_r_5;
                        tmp;
                        });
                       }
                      
                      if (r == SQLITE_BUSY) {
                      uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                       }
                      
                      if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: query step failed: %s<br />%s", 
                      "SELECT MAX(T_T.uw_N) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = 'max')", sqlite3_errmsg(conn->conn));
                      
                      uw_pop_cleanup(ctx);
                     uw_pop_cleanup(ctx);
                      
                   acc;
                   });
                  
                  disc == NULL ?
                   ({
                    uw_Basis_int*
                    tmp;
                    uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", "Query returned no rows");
                    tmp;
                    })
                    :
                   disc != NULL && 1 ?
                    ({struct __uws_5 __uwr_r_5 = (*disc);
                       __uwr_r_5.__uwf_1;
                     })
                     :
                    ({
                     uw_Basis_int*
                     tmp;
                     uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:32:20-33:61: pattern match failure");
                     tmp;
                     });
                  });
                 ({ struct __uws_4 tmp =
                  {({
                    uw_Basis_int* disc =
                    __uwr_mn_4;
                    
                    disc == NULL ? 0LL
                      :
                     disc != NULL && 1 ?
                      ({uw_Basis_int __uwr_v_6 = (*disc);
                         __uwr_v_6;
                       })
                       :
                      ({
                       uw_Basis_int
                       tmp;
                       uw_error(ctx, FATAL, "$/option.ur:54:4-59:0: pattern match failure");
                       tmp;
                       });
                    }), 
                    ({
                     uw_Basis_int* disc =
                     __uwr_mx_5;
                     
                     disc == NULL ? 0LL
                       :
                      disc != NULL && 1 ?
                       ({uw_Basis_int __uwr_v_6 = (*disc);
                          __uwr_v_6;
                        })
                        :
                       ({
                        uw_Basis_int
                        tmp;
                        uw_error(ctx, FATAL, "$/option.ur:54:4-59:0: pattern match failure");
                        tmp;
                        });
                     })}; tmp; });
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
             });
           }));
   }
  
  static uw_Basis_string
   __uwn_strs_14(uw_context ctx, uw_Basis_string __uwr_s_0, uw_unit __uwr___1)
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
                                      uw_Basis_string arg1 = __uwr_s_0;
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p7;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p7 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_text(stmt, 1, arg1, -1, SQLITE_TRANSIENT) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:36:4-36:65: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:36:4-36:65: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:36:4-36:65", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                uw_Basis_int __uwr_r_3 =
                (uw_begin_region(ctx), ({
                                        struct __uws_6* disc =
                                        ({
                                         struct __uws_6* acc =
                                         NULL;
                                         int dummy = (uw_begin_region(ctx), 0);
                                         uw_ensure_transaction(ctx);
                                         uw_Basis_string arg1 = __uwr_s_0;
                                          
                                          uw_conn *conn = uw_get_db(ctx);
                                           sqlite3_stmt *stmt = conn->p8;
                                            
                                            if (stmt == NULL) {
                                            if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = \?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = \?)<br />%s", sqlite3_errmsg(conn->conn));
                                           conn->p8 = stmt;
                                            }
                                            
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                            uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                            
                                           if (sqlite3_bind_text(stmt, 1, arg1, -1, SQLITE_TRANSIENT) != SQLITE_OK) uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                            
                                           int r;
                                            sqlite3_reset(stmt);
                                            uw_end_region(ctx);
                                            while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                            struct __uws_6 __uwr_r_3;
                                             struct __uws_6* __uwr_acc_4 =
                                             acc;
                                             
                                             __uwr_r_3.__uwf_1 =
                                              (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                               ({uw_Basis_int
                                                tmp;
                                                uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                tmp;
                                                }) : sqlite3_column_int64(stmt, 0));
                                              
                                             
                                             acc =
                                             ({
                                              struct __uws_6 *tmp =
                                              uw_malloc(ctx, sizeof(struct
                                                                     __uws_6));
                                              *tmp = __uwr_r_3;
                                              tmp;
                                              });
                                             }
                                            
                                            if (r == SQLITE_BUSY) {
                                            uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                             }
                                            
                                            if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: query step failed: %s<br />%s", 
                                            "SELECT COUNT(*) AS uw_1 FROM uw_Rpc_types_t AS T_T WHERE (T_T.uw_S = \?)", sqlite3_errmsg(conn->conn));
                                            
                                            uw_pop_cleanup(ctx);
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
                                          ({struct __uws_6 __uwr_r_3 = (*disc);
                                             __uwr_r_3.__uwf_1;
                                           })
                                           :
                                          ({
                                           uw_Basis_int
                                           tmp;
                                           uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:37:20-38:69: pattern match failure");
                                           tmp;
                                           });
                                        }));
                uw_end_region(ctx);
                 ({
                  uw_Basis_string arg0 = __uwr_s_0;
                   uw_Basis_string arg1 = "#";
                    
                   uw_Basis_string arg2 = uw_Basis_intToString(ctx, __uwr_r_3);
                    uw_Basis_string arg3 = "#";
                     
                   uw_Basis_string arg4 =
                    uw_Basis_intToString(ctx, uw_Basis_strlen(ctx, __uwr_s_0));
                    uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, NULL);
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
             });
           }));
   }
  
  static uw_unit
   __uwn_unitR_15(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
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
                sqlite3_stmt *stmt = conn->p9;
                
                if (stmt == NULL) {
                if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'unit')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'unit')<br />%s", sqlite3_errmsg(conn->conn));
                 conn->p9 = stmt;
                 }
                uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:41:4-41:70: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                 
                int r;
                 if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                 uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                  }
                 
                 if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:41:4-41:70: DML step failed: %s<br />%s", 
                                        "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'unit')", sqlite3_errmsg(conn->conn));
                                        else if (uw_dml_log_enabled()) {
                 char *expanded = sqlite3_expanded_sql(stmt);
                  uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:41:4-41:70", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
   }
  
  static uw_Basis_float
   __uwn_floats_16(uw_context ctx, uw_Basis_float __uwr_f_0, uw_unit __uwr___1)
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
                                      uw_Basis_string arg1 =
                                       uw_Basis_floatToString(ctx, __uwr_f_0);
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p7;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p7 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_text(stmt, 1, arg1, -1, SQLITE_TRANSIENT) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:44:4-44:70: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:44:4-44:70: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, \?)", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:44:4-44:70", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
            __uwr_f_0 * 2;
           }));
   }
  
  static uw_Basis_int*
   __uwn_opts_17(uw_context ctx, uw_Basis_int* __uwr_o_0, uw_unit __uwr___1)
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
                                      uw_Basis_int arg1 =
                                       ({
                                        uw_Basis_int* disc =
                                        __uwr_o_0;
                                        
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
                                           uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
                                           tmp;
                                           });
                                        });
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p10;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'opt')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'opt')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p10 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:48:4-48:85: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:48:4-48:85: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'opt')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:48:4-48:85", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
             __uwr_o_0;
             
             disc == NULL ? NULL
               :
              disc != NULL && 1 ?
               ({uw_Basis_int __uwr_y_3 = (*disc);
                  ({
                   uw_Basis_int *tmp =
                   uw_malloc(ctx, sizeof(uw_Basis_int));
                   *tmp = __uwr_y_3 * 2LL;
                   tmp;
                   });
                })
                :
               ({
                uw_Basis_int*
                tmp;
                uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:48:4-49:40: pattern match failure");
                tmp;
                });
             });
           }));
   }
  
  static struct __uws_7* __uwn_$revPRIME_unpoly_18(uw_context,
   struct __uws_7*, struct __uws_7*);
   static struct __uws_7*
    __uwn_$revPRIME_unpoly_18(uw_context ctx, struct __uws_7* __uwr_acc_0, 
                               struct __uws_7* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_7* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_7* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_7* rearg0 =
                    ({
                     struct __uws_7 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_7));
                     *tmp =
                     ({ struct __uws_7 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_7* rearg1 = __uwr_ls_3;
                   struct __uws_7*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_7*
               tmp;
               uw_error(ctx, FATAL, "$/list.ur:47:21-50:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_$appPRIME_unpoly_unpoly_19(uw_context,
   struct __uws_7*, uw_unit);
   static uw_unit
    __uwn_$appPRIME_unpoly_unpoly_19(uw_context ctx, 
                                      struct __uws_7* __uwr_ls_0, 
                                      uw_unit __uwr___1)
    {restart:
      
    return(({
            struct __uws_7* disc =
            __uwr_ls_0;
            
            disc == NULL ? 0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_7* __uwr_ls_3 = (*disc).__uwf_2;
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
                                              uw_Basis_int arg1 = __uwr_x_2;
                                               
                                               uw_ensure_transaction(ctx);
                                               
                                               uw_conn *conn = uw_get_db(ctx);
                                                sqlite3_stmt *stmt = conn->p11;
                                                
                                                if (stmt == NULL) {
                                                if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'list')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'list')<br />%s", sqlite3_errmsg(conn->conn));
                                                 conn->p11 = stmt;
                                                 }
                                                uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                if (sqlite3_bind_int64(stmt, 1, 
                                                     arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:52:22-52:88: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                 
                                                int r;
                                                 if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                 uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                  }
                                                 
                                                 if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:52:22-52:88: DML step failed: %s<br />%s", 
                                                                        "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'list')", sqlite3_errmsg(conn->conn));
                                                                        else if (uw_dml_log_enabled()) {
                                                 char *expanded = sqlite3_expanded_sql(stmt);
                                                  uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:52:22-52:88", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                     struct __uws_7* rearg0 = __uwr_ls_3;
                      uw_unit rearg1 = 0;
                     uw_unit
                     tmp;
                     __uwr_ls_0 = rearg0;
                      __uwr___1 = rearg1;
                     goto restart;
                     tmp;
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
   
  
  static struct __uws_7*
   __uwn_lists_20(uw_context ctx, struct __uws_7* __uwr_l_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   struct __uws_7* arg0 = __uwr_l_0;
                                    uw_unit arg1 = 0;
                                   __uwn_$appPRIME_unpoly_unpoly_19(ctx,
                                   arg0, arg1);
                                   }));
           uw_end_region(ctx);
            ({
             struct __uws_7* arg0 = NULL;
              struct __uws_7* arg1 = __uwr_l_0;
             __uwn_$revPRIME_unpoly_18(ctx, arg0, arg1);
             });
           }));
   }
  
  static struct __uwd_color_3*
   __uwn_colors_21(uw_context ctx, struct __uwd_color_3* __uwr_c_0, 
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
                                      
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p12;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, 'color')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, 'color')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p12 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:56:4-56:67: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (0, 'color')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:56:4-56:67", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
             struct __uwd_color_3* disc =
             __uwr_c_0;
             
             disc->tag == __uwc_Red_4 ?
              ({
               struct __uwd_color_3 *tmp =
               uw_malloc(ctx, sizeof(struct __uwd_color_3));
               tmp->tag = __uwc_Green_5;
               tmp->data.uw_Green = 1LL;
                tmp;
               })
               :
              disc->tag == __uwc_Green_5 && 1 ?
               ({uw_Basis_int __uwr_n_3 = disc->data.uw_Green;
                  ({
                   struct __uwd_color_3 *tmp =
                   uw_malloc(ctx, sizeof(struct __uwd_color_3));
                   tmp->tag =
                   __uwc_Blue_6;
                   tmp->data.uw_Blue =
                    ({ struct __uws_1 tmp =
                     {"g", 
                       ({
                        uw_Basis_int *tmp =
                        uw_malloc(ctx, sizeof(uw_Basis_int));
                        *tmp = __uwr_n_3;
                        tmp;
                        })}; tmp; });
                    tmp;
                   });
                })
                :
               disc->tag == __uwc_Blue_6 && 1 && 1 ?
                ({uw_Basis_string __uwr_s_3 = disc->data.uw_Blue.__uwf_1;
                   uw_Basis_int* __uwr_o_4 = disc->data.uw_Blue.__uwf_2;
                    ({
                     struct __uwd_color_3 *tmp =
                     uw_malloc(ctx, sizeof(struct __uwd_color_3));
                     tmp->tag = __uwc_Red_4;
                     tmp;
                     });
                 })
                 :
                ({
                 struct __uwd_color_3*
                 tmp;
                 uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:56:4-57:90: pattern match failure");
                 tmp;
                 });
             });
           }));
   }
  
  static uw_Basis_string
   __uwn_xb_22(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
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
                                        sqlite3_stmt *stmt = conn->p13;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'xb')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'xb')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p13 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:60:4-60:68: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:60:4-60:68: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'xb')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:60:4-60:68", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
             uw_Basis_string arg0 = "<b>bold ";
              uw_Basis_string arg1 = uw_Basis_htmlifyInt(ctx, __uwr_n_0);
               uw_Basis_string arg2 = "</b>";
                uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
             });
           }));
   }
  
  static uw_Basis_int
   __uwn_tryNone_23(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
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
                                        sqlite3_stmt *stmt = conn->p14;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'tryNone')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'tryNone')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p14 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:64:4-64:73: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:64:4-64:73: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'tryNone')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:64:4-64:73", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:65:19-65:51: %s", 
                "tryNone fails");
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
  
  static struct __uws_10* __uwn_$revPRIME_unpoly_24(uw_context,
   struct __uws_10*, struct __uws_10*);
   static struct __uws_10*
    __uwn_$revPRIME_unpoly_24(uw_context ctx, struct __uws_10* __uwr_acc_0, 
                               struct __uws_10* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_10* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_9 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_10* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_10* rearg0 =
                    ({
                     struct __uws_10 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_10));
                     *tmp =
                     ({ struct __uws_10 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_10* rearg1 = __uwr_ls_3;
                   struct __uws_10*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_10*
               tmp;
               uw_error(ctx, FATAL, "$/top.ur:279:21-282:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_string __uwn_$foldlPRIME_unpoly_unpoly_25(uw_context,
   uw_Basis_string, struct __uws_10*);
   static uw_Basis_string
    __uwn_$foldlPRIME_unpoly_unpoly_25(uw_context ctx, 
                                        uw_Basis_string __uwr_acc_0, 
                                        struct __uws_10* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_10* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_9 __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_10* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_Basis_string rearg0 =
                    ({
                     uw_Basis_string arg0 = __uwr_acc_0;
                      uw_Basis_string arg1 = __uwr_x_2.__uwf_T.__uwf_S;
                       uw_Basis_string arg2 = "=";
                        
                      uw_Basis_string arg3 =
                       uw_Basis_intToString(ctx, __uwr_x_2.__uwf_C);
                       uw_Basis_string arg4 = ",";
                        
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
                           uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:69:89-69:99: pattern match failure");
                           tmp;
                           });
                        });
                       uw_Basis_string arg6 = "; ";
                        uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, 
                                               arg5, arg6, NULL);
                     });
                    struct __uws_10* rearg1 = __uwr_ls_3;
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
   __uwn_summary_26(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
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
               
               struct __uws_10* arg1 =
                ({
                 struct __uws_10* arg0 = NULL;
                  
                  struct __uws_10* arg1 =
                   ({
                    struct __uws_10* acc =
                    NULL;
                    int dummy = (uw_begin_region(ctx), 0);
                    uw_ensure_transaction(ctx);
                    
                     
                     uw_conn *conn = uw_get_db(ctx);
                      sqlite3_stmt *stmt = conn->p15;
                       
                       if (stmt == NULL) {
                       if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Rpc_types_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Rpc_types_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S<br />%s", sqlite3_errmsg(conn->conn));
                      conn->p15 = stmt;
                       }
                       
                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                       uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                       
                      
                      int r;
                       sqlite3_reset(stmt);
                       uw_end_region(ctx);
                       while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                       struct __uws_9 __uwr_r_2;
                        struct __uws_10* __uwr_acc_3 =
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
                         struct __uws_10 *tmp =
                         uw_malloc(ctx, sizeof(struct __uws_10));
                         *tmp =
                         ({ struct __uws_10 tmp = {__uwr_r_2, __uwr_acc_3};
                          tmp; });
                         tmp;
                         });
                        }
                       
                       if (r == SQLITE_BUSY) {
                       uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                        }
                       
                       if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:288:16-288:18: query step failed: %s<br />%s", 
                       "SELECT COUNT(*) AS uw_C, SUM(T_T.uw_N) AS uw_Sum, T_T.uw_S FROM uw_Rpc_types_t AS T_T GROUP BY T_T.uw_S ORDER BY T_T.uw_S", sqlite3_errmsg(conn->conn));
                       
                       uw_pop_cleanup(ctx);
                      uw_pop_cleanup(ctx);
                       
                    acc;
                    });
                 __uwn_$revPRIME_unpoly_24(ctx, arg0, arg1);
                 });
              __uwn_$foldlPRIME_unpoly_unpoly_25(ctx, arg0, arg1);
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
   __uwn_$p_27(uw_context ctx, uw_Basis_int __uwr_c_0, uw_unit __uwr___1)
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
                                      uw_Basis_int arg1 = 120LL + __uwr_c_0;
                                       
                                       uw_ensure_transaction(ctx);
                                       
                                       uw_conn *conn = uw_get_db(ctx);
                                        sqlite3_stmt *stmt = conn->p16;
                                        
                                        if (stmt == NULL) {
                                        if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'partial')", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'partial')<br />%s", sqlite3_errmsg(conn->conn));
                                         conn->p16 = stmt;
                                         }
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                        uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                        if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:18:20-18:122: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                         
                                        int r;
                                         if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                         uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                          }
                                         
                                         if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/rpc_types/rpc_types.ur:18:20-18:122: DML step failed: %s<br />%s", 
                                                                "INSERT INTO uw_Rpc_types_t (uw_N, uw_S) VALUES (\?, 'partial')", sqlite3_errmsg(conn->conn));
                                                                else if (uw_dml_log_enabled()) {
                                         char *expanded = sqlite3_expanded_sql(stmt);
                                          uw_log_dml(ctx, "tests/io_shapes/rpc_types/rpc_types.ur:18:20-18:122", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                          sqlite3_free(expanded);
                                          }
                                         uw_pop_cleanup(ctx);
                                        uw_pop_cleanup(ctx);
                                        
                                      
                                      uw_end_region(ctx);
                                      0;
                                      })));
              uw_end_region(ctx);
               120LL + __uwr_c_0;
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
   __uwn_wrap_main_28(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
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
            ({
             uw_Basis_source __uwr_sx_4 =
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
                                                         (uw_begin_region(ctx), 
                                                          ((uw_write(ctx, "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                           (uw_begin_region(ctx),
                                                             uw_Basis_htmlifySource_w(ctx,
                                                              __uwr_s_2),
                                                            uw_end_region(ctx), 
                                                            (uw_write(ctx, "}},x:{c:\"c\",v:null}}"), 0))),
                                                          uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ")'>alias</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>wrapped</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>partial</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:4},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s2_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>six</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>rec1</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>minmax</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:9},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s2_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>strs</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>unit</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:11},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s2_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>floats</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:12},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s2_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>opts</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>lists</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:16},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>colors</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:17},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_sx_4
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>xb</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:18},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>tryNone</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:19},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>tryNone2</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:20},x:{c:\"c\",v:"), 0),
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
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:21},x:{c:\"c\",v:"), 0),
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
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:22},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_sx_4
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))))))))))))))))))))))))))))))))));
             });
            });
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Rpc_types/main")) return 1;
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
 
 static struct __uws_2* unurlify_list_2(uw_context, char **);
  static struct __uwd_color_3* unurlify_3(uw_context, char **);
   static struct __uws_7* unurlify_list_7(uw_context, char **);
    static void urlifyl_7(uw_context, struct __uws_7 *);
     static void urlify_3(uw_context, struct __uwd_color_3*);
      static struct __uws_2*
       unurlify_list_2(uw_context ctx, char **request) {
       return ((*request)[0] == '/' ? ++*request : *request,
        ((!strncmp(*request, "Nil", 3) && ((*request)[3] == 0 || (*request)[3] == '/')) ? (*request
        +=
        3, ((*request)[0] == '/' ? ((*request)[0] = 0, ++*request) : NULL), NULL) : ((!strncmp(*request, "Cons", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? (*request
        += 4, ((*request)[0] == '/' ? ++*request : NULL), 
        ({
        struct __uws_2* tmp = uw_malloc(ctx, sizeof(struct __uws_2));
        *tmp =
        ({
         uw_Basis_string uwr_1 = uw_Basis_unurlifyString(ctx, request);
          struct __uws_2* uwr_2 = unurlify_list_2(ctx, request);
           struct __uws_2 tmp = { uwr_1, uwr_2 };
         tmp;
         });
        tmp;
        }))
        :
        (uw_error(ctx, FATAL, "Error unurlifying list: %s", *request), NULL))));
        }
       
       static struct __uwd_color_3*
        unurlify_3(uw_context ctx, char **request) {
        return
         ((!strncmp(*request, "Red", 3) && ((*request)[3] == 0 || (*request)[3] == '/')) ? ({
          struct __uwd_color_3
          *tmp = uw_malloc(ctx, sizeof(struct __uwd_color_3));
          tmp->tag = __uwc_Red_4;
          *request += 3;
          if ((*request)[0] == '/') ++*request;
          tmp;
          }) :
          ((!strncmp(*request, "Green", 5) && ((*request)[5] == 0 || (*request)[5] == '/')) ? ({
           struct __uwd_color_3
           *tmp = uw_malloc(ctx, sizeof(struct __uwd_color_3));
           tmp->tag = __uwc_Green_5;
           *request +=
           5;
           if ((*request)[0] == '/') ++*request;
           tmp->data.uw_Green = uw_Basis_unurlifyInt(ctx, request);
            tmp;
           }) :
           ((!strncmp(*request, "Blue", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? ({
            struct __uwd_color_3
            *tmp = uw_malloc(ctx, sizeof(struct __uwd_color_3));
            tmp->tag = __uwc_Blue_6;
            *request +=
            4;
            if ((*request)[0] == '/') ++*request;
            tmp->data.uw_Blue =
             ({
              uw_Basis_string uwr_1 = uw_Basis_unurlifyString(ctx, request);
               uw_Basis_int* uwr_2 =
                ((*request)[0] == '/' ? ++(*request) : (*request), ((!strncmp((*request), "None", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? ((*request) += ((*request)[4] == 0 ? 4 : 5), NULL) : ((!strncmp((*request), "Some", 4) && (*request)[4] == '/') ? ((*request) += 5, 
                 ({
                  uw_Basis_int *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
                  *tmp = uw_Basis_unurlifyInt(ctx, request);
                  tmp;
                  })) :
                 (uw_error(ctx, FATAL, "Error unurlifying option type"), NULL))));
                struct __uws_1 tmp = { uwr_1, uwr_2 };
              tmp;
              });
             tmp;
            }) :
            (uw_error(ctx, FATAL, "Error unurlifying datatype color"), NULL))));
         }
        
        static struct __uws_7*
         unurlify_list_7(uw_context ctx, char **request) {
         return ((*request)[0] == '/' ? ++*request : *request,
          ((!strncmp(*request, "Nil", 3) && ((*request)[3] == 0 || (*request)[3] == '/')) ? (*request
          +=
          3, ((*request)[0] == '/' ? ((*request)[0] = 0, ++*request) : NULL), NULL) : ((!strncmp(*request, "Cons", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? (*request
          += 4, ((*request)[0] == '/' ? ++*request : NULL), 
          ({
          struct __uws_7* tmp = uw_malloc(ctx, sizeof(struct __uws_7));
          *tmp =
          ({
           uw_Basis_int uwr_1 = uw_Basis_unurlifyInt(ctx, request);
            struct __uws_7* uwr_2 = unurlify_list_7(ctx, request);
             struct __uws_7 tmp = { uwr_1, uwr_2 };
           tmp;
           });
          tmp;
          }))
          :
          (uw_error(ctx, FATAL, "Error unurlifying list: %s", *request), NULL))));
          }
         
         static void urlifyl_7(uw_context ctx, struct __uws_7
          *it0) {
          if (it0) {
           uw_Basis_int it1 =
           it0->__uwf_1;
           uw_write(ctx, "Cons/");
           uw_Basis_urlifyInt_w(ctx, it1);
            ;
           uw_write(ctx, "/");
           urlifyl_7(ctx, it0->__uwf_2);
           } else {
           uw_write(ctx, "Nil");
            }
           }
          
          static void urlify_3(uw_context ctx, struct __uwd_color_3*
           it0) {
           if
            (it0->tag==__uwc_Red_4) {
            uw_write(ctx, "Red");
             } else {
            if
             (it0->tag==__uwc_Green_5) {
             uw_write(ctx, "Green/");
              uw_Basis_int it1 =
              it0->data.uw_Green;
              uw_Basis_urlifyInt_w(ctx, it1);
               
              } else {
             if
              (it0->tag==__uwc_Blue_6) {
              uw_write(ctx, "Blue/");
               struct __uws_1 it1 =
               it0->data.uw_Blue;
               {
                uw_Basis_string it2 =
                it1.__uwf_1;
                uw_Basis_urlifyString_w(ctx, it2);
                 }
                {
                 uw_Basis_int* it2 =
                 it1.__uwf_2;
                 uw_write(ctx, "/");
                  if (it2) {uw_Basis_int it3 =
                             *it2;
                             uw_write(ctx, "Some/");
                             uw_Basis_urlifyInt_w(ctx, it3);
                              ;
                             } else {
                                      uw_write(ctx, "None");
                                      }
                   }
                 
               } else {
              uw_error(ctx, FATAL, "Error urlifying datatype color (%d)", it0->data);
               
               }
              
              }
             
             }
            
            
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
  
  
  if (!strcmp(request, "/app.FD34D9EFAA9930B6BEF6B48FFA70D3F4797A6194.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Rpc_types/main", 15) && (request[15] == 0 || request[15] == '/')) {
  request += 15;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.FD34D9EFAA9930B6BEF6B48FFA70D3F4797A6194.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_28(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Rpc_types/viaAlias", 19) && (request[19] == 0 || request[19] == '/')) {
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
      __uwn_viaAlias_7(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/wrapped", 18) && (request[18] == 0 || request[18] == '/')) {
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
      __uwn_wrapped_8(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/p", 12) && (request[12] == 0 || request[12] == '/')) {
   request += 12;
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
      __uwn_$p_27(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/six", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
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
     uw_Basis_string arg1 = uw_Basis_unurlifyString(ctx, &request);
      uw_Basis_float arg2 = uw_Basis_unurlifyFloat(ctx, &request);
       uw_Basis_int* arg3 =
        (request[0] == '/' ? ++request : request, ((!strncmp(request, "None", 4) && (request[4] == 0 || request[4] == '/')) ? (request += (request[4] == 0 ? 4 : 5), NULL) : ((!strncmp(request, "Some", 4) && request[4] == '/') ? (request += 5, 
         ({
          uw_Basis_int *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
          *tmp = uw_Basis_unurlifyInt(ctx, &request);
          tmp;
          })) :
         (uw_error(ctx, FATAL, "Error unurlifying option type"), NULL))));
        struct __uws_2* arg4 = unurlify_list_2(ctx, &request);
         struct __uwd_color_3* arg5 = unurlify_3(ctx, &request);
          uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_string it0 =
      __uwn_six_11(ctx, arg0, arg1, arg2, arg3, arg4, arg5, 0);
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
  
  if (!strncmp(request, "/Rpc_types/rec1", 15) && (request[15] == 0 || request[15] == '/')) {
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
    struct __uws_3 arg0 =
     ({
      uw_Basis_int uwr_A = uw_Basis_unurlifyInt(ctx, &request);
       uw_Basis_string uwr_B = uw_Basis_unurlifyString(ctx, &request);
        uw_Basis_float* uwr_C =
         (request[0] == '/' ? ++request : request, ((!strncmp(request, "None", 4) && (request[4] == 0 || request[4] == '/')) ? (request += (request[4] == 0 ? 4 : 5), NULL) : ((!strncmp(request, "Some", 4) && request[4] == '/') ? (request += 5, 
          ({
           uw_Basis_float *tmp = uw_malloc(ctx, sizeof(uw_Basis_float));
           *tmp = uw_Basis_unurlifyFloat(ctx, &request);
           tmp;
           })) :
          (uw_error(ctx, FATAL, "Error unurlifying option type"), NULL))));
         struct __uws_3 tmp = { uwr_A, uwr_B, uwr_C };
      tmp;
      });
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     struct __uws_3 it0 =
      __uwn_rec1_12(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      {
       uw_Basis_int it1 = it0.__uwf_A;
       uw_Basis_urlifyInt_w(ctx, it1);
        }
       {
        uw_Basis_string it1 =
        it0.__uwf_B;
        uw_write(ctx, "/");
         uw_Basis_urlifyString_w(ctx, it1);
          }
        {
         uw_Basis_float* it1 =
         it0.__uwf_C;
         uw_write(ctx, "/");
          if (it1) {uw_Basis_float it2 =
                     *it1;
                     uw_write(ctx, "Some/");
                     uw_Basis_urlifyFloat_w(ctx, it2);
                      ;
                     } else {
                              uw_write(ctx, "None");
                              }
           }
         return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Rpc_types/minmax", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
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
     uw_Basis_int arg1 = uw_Basis_unurlifyInt(ctx, &request);
      uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     struct __uws_4 it0 =
      __uwn_minmax_13(ctx, arg0, arg1, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      {
       uw_Basis_int it1 = it0.__uwf_1;
       uw_Basis_urlifyInt_w(ctx, it1);
        }
       {
        uw_Basis_int it1 =
        it0.__uwf_2;
        uw_write(ctx, "/");
         uw_Basis_urlifyInt_w(ctx, it1);
          }
        return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Rpc_types/strs", 15) && (request[15] == 0 || request[15] == '/')) {
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
    uw_Basis_string arg0 = uw_Basis_unurlifyString(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_string it0 =
      __uwn_strs_14(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/unitR", 16) && (request[16] == 0 || request[16] == '/')) {
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
     uw_unit it0 =
      __uwn_unitR_15(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      uw_Basis_urlifyString_w(ctx, "");
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Rpc_types/floats", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
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
    uw_Basis_float arg0 = uw_Basis_unurlifyFloat(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_float it0 =
      __uwn_floats_16(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      uw_Basis_urlifyFloat_w(ctx, it0);
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Rpc_types/opts", 15) && (request[15] == 0 || request[15] == '/')) {
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
     uw_Basis_int* it0 =
      __uwn_opts_17(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/lists", 16) && (request[16] == 0 || request[16] == '/')) {
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
    struct __uws_7* arg0 = unurlify_list_7(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     struct __uws_7* it0 =
      __uwn_lists_20(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      urlifyl_7(ctx, it0);
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Rpc_types/colors", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
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
    struct __uwd_color_3* arg0 = unurlify_3(ctx, &request);
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     struct __uwd_color_3* it0 =
      __uwn_colors_21(ctx, arg0, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, uw_get_real_script(ctx));
      uw_write(ctx, "\n");
      urlify_3(ctx, it0);
       return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
    }
    }
  
  if (!strncmp(request, "/Rpc_types/xb", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
     uw_Basis_string it0 =
      __uwn_xb_22(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/tryNone", 18) && (request[18] == 0 || request[18] == '/')) {
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
      __uwn_tryNone_23(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Rpc_types/summary", 18) && (request[18] == 0 || request[18] == '/')) {
   request += 18;
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
      __uwn_summary_26(ctx, arg0, 0);
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
 