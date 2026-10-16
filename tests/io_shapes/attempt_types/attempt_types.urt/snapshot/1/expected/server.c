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
     
     if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Attempt_types_t' COLLATE NOCASE", -1, &stmt, NULL) != SQLITE_OK) {
      sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Query preparation failed:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Attempt_types_t' COLLATE NOCASE");
       }
      
      while ((res = sqlite3_step(stmt)) == SQLITE_BUSY)
      sleep(1);
       
      if (res == SQLITE_DONE) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "No row returned:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Attempt_types_t' COLLATE NOCASE");
       }
      
      if (res != SQLITE_ROW) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Error getting row:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Attempt_types_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_count(stmt) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Bad column count:<br />SELECT COUNT(*) FROM sqlite_schema WHERE name = 'uw_Attempt_types_t' COLLATE NOCASE");
       }
      
      if (sqlite3_column_int(stmt, 0) != 1) {
      sqlite3_finalize(stmt);
       sqlite3_close(conn->conn);
       uw_error(ctx, FATAL, "Table 'uw_Attempt_types_t' does not exist.");
       }
      
      sqlite3_finalize(stmt);
      }
     
     static void uw_db_prepare(uw_context ctx) {
     uw_conn *conn = uw_get_db(ctx);
     
     if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", -1, &conn->p0, NULL) != SQLITE_OK) {
      char msg[1024];
       strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
       msg[1023] = 0;
       sqlite3_close(conn->conn);
        uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)<br />%s"
        , msg);
        }
      
      
      if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T", -1, &conn->p1, NULL) != SQLITE_OK) {
       char msg[1024];
        strncpy(msg, sqlite3_errmsg(conn->conn), 1024);
        msg[1023] = 0;
        sqlite3_finalize(conn->p0);
         sqlite3_close(conn->conn);
         uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T<br />%s"
         , msg);
         }
       }
    
    static void uw_db_init(uw_context ctx) {
    sqlite3 *sqlite;
    sqlite3_stmt *stmt;
    uw_conn *conn;
    
    char *env_db_str = getenv("URWEB_SQLITE_DB_PATH");
    const char *sqlite_db_path = env_db_str == NULL ? "attempt_types.db" : env_db_str;
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
  
 enum __uwe_enum_7 { __uwc_A_8, __uwc_B_9, __uwc_C_10 };
  
  
  struct __uws_1 {
   uw_Basis_float __uwf_1;
    uw_Basis_string __uwf_2;
     };
  
  enum __uwe_shape_3 { __uwc_Dot_4, __uwc_Circle_5, __uwc_Rect_6 };
   
   struct __uwd_shape_3 {
   enum __uwe_shape_3
   tag;
   union {
    uw_Basis_int uw_Circle;
     struct __uws_1 uw_Rect;
    } data;
    };
  
  struct __uws_2 {
   uw_Basis_int __uwf_1;
    struct __uws_2* __uwf_2;
     };
  struct __uws_3 {
   uw_Basis_int __uwf_X;
    uw_Basis_string __uwf_Y;
     };
  struct __uws_4 {
   uw_Basis_int __uwf_1;
    uw_Basis_string __uwf_2;
     };
  struct __uws_5 {
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
  
  /* SQL table uw_Attempt_types_t constraints   */
   
  
  static uw_Basis_string
   __uwn_showShape_11(uw_context ctx, struct __uwd_shape_3* __uwr_s_0)
   {
   return(({
           struct __uwd_shape_3* disc =
           __uwr_s_0;
           
           disc->tag == __uwc_Dot_4 ? "Dot"
             :
            disc->tag == __uwc_Circle_5 && 1 ?
             ({uw_Basis_int __uwr_n_1 = disc->data.uw_Circle;
                ({
                 uw_Basis_string arg0 = "Circle ";
                  uw_Basis_string arg1 = uw_Basis_intToString(ctx, __uwr_n_1);
                   uw_Basis_strcat(ctx, arg0, arg1);
                 });
              })
              :
             disc->tag == __uwc_Rect_6 && 1 && 1 ?
              ({uw_Basis_float __uwr_f_1 = disc->data.uw_Rect.__uwf_1;
                 uw_Basis_string __uwr_s_2 = disc->data.uw_Rect.__uwf_2;
                  ({
                   uw_Basis_string arg0 = "Rect ";
                    
                    uw_Basis_string arg1 =
                     uw_Basis_floatToString(ctx, __uwr_f_1);
                     uw_Basis_string arg2 = " ";
                      uw_Basis_string arg3 = __uwr_s_2;
                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, NULL);
                   });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:8:28-9:0: pattern match failure");
               tmp;
               });
           }));
   }
  
  static uw_Basis_string
   __uwn_showEnum_12(uw_context ctx, enum __uwe_enum_7 __uwr_e_0)
   {
   return(({
           enum __uwe_enum_7 disc =
           __uwr_e_0;
           
           disc == __uwc_A_8 ? "A"
             :
            disc == __uwc_B_9 ? "B"
              :
             disc == __uwc_C_10 ? "C"
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:9:26-10:0: pattern match failure");
               tmp;
               });
           }));
   }
  
  static uw_Basis_int __uwn_$lengthPRIME_unpoly_13(uw_context,
   uw_Basis_int, struct __uws_2*);
   static uw_Basis_int
    __uwn_$lengthPRIME_unpoly_13(uw_context ctx, uw_Basis_int __uwr_acc_0, 
                                  struct __uws_2* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr___2 = (*disc).__uwf_1;
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
   
  
  static uw_Basis_string
   __uwn_ok_14(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int* __uwr_i_2 =
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
              __uwr_n_0;
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
            uw_Basis_float* __uwr_f_3 =
            ({
             uw_io_transaction __uwt;
             uw_Basis_float
             *__uwr = NULL;
             uw_set_could_write_db(ctx, 0);
             uw_io_transaction_begin(ctx, &__uwt);
             while (1) {
             int __uwfk = setjmp(*uw_jmp_buf(ctx));
              if (__uwfk == 0) {
              uw_Basis_float __uwv =
               uw_Basis_floatFromInt(ctx, __uwr_n_0) / 4;
               __uwr = uw_malloc(ctx, sizeof(uw_Basis_float));
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
             uw_Basis_string __uwr_s_4 =
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
                 uw_Basis_string arg0 = "s";
                  uw_Basis_string arg1 = uw_Basis_intToString(ctx, __uwr_n_0);
                   uw_Basis_strcat(ctx, arg0, arg1);
                 });
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
              uw_Basis_bool* __uwr_b_5 =
              ({
               uw_io_transaction __uwt;
               uw_Basis_bool
               *__uwr = NULL;
               uw_set_could_write_db(ctx, 0);
               uw_io_transaction_begin(ctx, &__uwt);
               while (1) {
               int __uwfk = setjmp(*uw_jmp_buf(ctx));
                if (__uwfk == 0) {
                uw_Basis_bool __uwv =
                 ! (__uwr_n_0 <= 0LL);
                 __uwr = uw_malloc(ctx, sizeof(uw_Basis_bool));
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
               uw_unit* __uwr_u_6 =
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
                  0;
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
                struct __uws_3* __uwr_r_7 =
                ({
                 uw_io_transaction __uwt;
                 struct __uws_3
                 *__uwr = NULL;
                 uw_set_could_write_db(ctx, 0);
                 uw_io_transaction_begin(ctx, &__uwt);
                 while (1) {
                 int __uwfk = setjmp(*uw_jmp_buf(ctx));
                  if (__uwfk == 0) {
                  struct __uws_3 __uwv =
                   ({ struct __uws_3 tmp = {__uwr_n_0, "y"}; tmp; });
                   __uwr = uw_malloc(ctx, sizeof(struct __uws_3));
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
                 struct __uws_4* __uwr_p_8 =
                 ({
                  uw_io_transaction __uwt;
                  struct __uws_4
                  *__uwr = NULL;
                  uw_set_could_write_db(ctx, 0);
                  uw_io_transaction_begin(ctx, &__uwt);
                  while (1) {
                  int __uwfk = setjmp(*uw_jmp_buf(ctx));
                   if (__uwfk == 0) {
                   struct __uws_4 __uwv =
                    ({ struct __uws_4 tmp =
                     {__uwr_n_0, uw_Basis_intToString(ctx, __uwr_n_0)}; tmp; });
                    __uwr = uw_malloc(ctx, sizeof(struct __uws_4));
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
                  uw_Basis_int** __uwr_oo_9 =
                  ({
                   uw_io_transaction __uwt;
                   uw_Basis_int*
                   *__uwr = NULL;
                   uw_set_could_write_db(ctx, 0);
                   uw_io_transaction_begin(ctx, &__uwt);
                   while (1) {
                   int __uwfk = setjmp(*uw_jmp_buf(ctx));
                    if (__uwfk == 0) {
                    uw_Basis_int* __uwv =
                     ({
                      uw_Basis_int *tmp =
                      uw_malloc(ctx, sizeof(uw_Basis_int));
                      *tmp = __uwr_n_0;
                      tmp;
                      });
                     __uwr = uw_malloc(ctx, sizeof(uw_Basis_int*));
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
                   uw_Basis_int** __uwr_on_10 =
                   ({
                    uw_io_transaction __uwt;
                    uw_Basis_int*
                    *__uwr = NULL;
                    uw_set_could_write_db(ctx, 0);
                    uw_io_transaction_begin(ctx, &__uwt);
                    while (1) {
                    int __uwfk = setjmp(*uw_jmp_buf(ctx));
                     if (__uwfk == 0) {
                     uw_Basis_int* __uwv =
                      NULL;
                      __uwr = uw_malloc(ctx, sizeof(uw_Basis_int*));
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
                    struct __uws_2** __uwr_l_11 =
                    ({
                     uw_io_transaction __uwt;
                     struct __uws_2*
                     *__uwr = NULL;
                     uw_set_could_write_db(ctx, 0);
                     uw_io_transaction_begin(ctx, &__uwt);
                     while (1) {
                     int __uwfk = setjmp(*uw_jmp_buf(ctx));
                      if (__uwfk == 0) {
                      struct __uws_2* __uwv =
                       ({
                        struct __uws_2 *tmp =
                        uw_malloc(ctx, sizeof(struct __uws_2));
                        *tmp =
                        ({ struct __uws_2 tmp =
                         {__uwr_n_0, 
                           ({
                            struct __uws_2 *tmp =
                            uw_malloc(ctx, sizeof(struct __uws_2));
                            *tmp =
                            ({ struct __uws_2 tmp = {__uwr_n_0 + 1LL, NULL};
                             tmp; });
                            tmp;
                            })}; tmp; });
                        tmp;
                        });
                       __uwr = uw_malloc(ctx, sizeof(struct __uws_2*));
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
                     struct __uwd_shape_3* __uwr_d1_12 =
                     ({
                      uw_io_transaction __uwt;
                      struct __uwd_shape_3*
                      __uwr;
                      uw_set_could_write_db(ctx, 0);
                      uw_io_transaction_begin(ctx, &__uwt);
                      while (1) {
                      int __uwfk = setjmp(*uw_jmp_buf(ctx));
                       if (__uwfk == 0) {
                       __uwr =
                        ({
                         struct __uwd_shape_3 *tmp =
                         uw_malloc(ctx, sizeof(struct __uwd_shape_3));
                         tmp->tag =
                         __uwc_Rect_6;
                         tmp->data.uw_Rect =
                          ({ struct __uws_1 tmp = {1.5, "r"}; tmp; });
                          tmp;
                         });
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
                      struct __uwd_shape_3* __uwr_d2_13 =
                      ({
                       uw_io_transaction __uwt;
                       struct __uwd_shape_3*
                       __uwr;
                       uw_set_could_write_db(ctx, 0);
                       uw_io_transaction_begin(ctx, &__uwt);
                       while (1) {
                       int __uwfk = setjmp(*uw_jmp_buf(ctx));
                        if (__uwfk == 0) {
                        __uwr =
                         ({
                          struct __uwd_shape_3 *tmp =
                          uw_malloc(ctx, sizeof(struct __uwd_shape_3));
                          tmp->tag = __uwc_Dot_4;
                          tmp;
                          });
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
                       enum __uwe_enum_7* __uwr_e_14 =
                       ({
                        uw_io_transaction __uwt;
                        enum __uwe_enum_7
                        *__uwr = NULL;
                        uw_set_could_write_db(ctx, 0);
                        uw_io_transaction_begin(ctx, &__uwt);
                        while (1) {
                        int __uwfk = setjmp(*uw_jmp_buf(ctx));
                         if (__uwfk == 0) {
                         enum __uwe_enum_7 __uwv =
                          __uwc_B_9;
                          __uwr = uw_malloc(ctx, sizeof(enum __uwe_enum_7));
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
                        uw_Basis_time* __uwr_ti_15 =
                        ({
                         uw_io_transaction __uwt;
                         uw_Basis_time
                         *__uwr = NULL;
                         uw_set_could_write_db(ctx, 0);
                         uw_io_transaction_begin(ctx, &__uwt);
                         while (1) {
                         int __uwfk = setjmp(*uw_jmp_buf(ctx));
                          if (__uwfk == 0) {
                          uw_Basis_time __uwv =
                           uw_Basis_now(ctx);
                           __uwr = uw_malloc(ctx, sizeof(uw_Basis_time));
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
                         uw_unit __uwr___16 =
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
                                                     __uwr_n_0;
                                                     
                                                     uw_ensure_transaction(ctx);
                                                     
                                                     uw_conn *conn = uw_get_db(ctx);
                                                      sqlite3_stmt *stmt = conn->p0;
                                                      
                                                      if (stmt == NULL) {
                                                      if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                                       conn->p0 = stmt;
                                                       }
                                                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                      if (sqlite3_bind_int64(stmt, 1, 
                                                           arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:27:4-27:59: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                       
                                                      int r;
                                                       if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                       sleep(1);
                                                        uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                        }
                                                       
                                                       if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:27:4-27:59: DML step failed: %s<br />%s", 
                                                                              "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                              else if (uw_dml_log_enabled()) {
                                                       char *expanded = sqlite3_expanded_sql(stmt);
                                                        uw_log_dml(ctx, "tests/io_shapes/attempt_types/attempt_types.ur:27:4-27:59", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                           uw_Basis_string arg0 =
                            ({
                             uw_Basis_int* disc =
                             __uwr_i_2;
                             
                             disc == NULL ? "None"
                               :
                              disc != NULL && 1 ?
                               ({uw_Basis_int __uwr_x_17 = (*disc);
                                  ({
                                   uw_Basis_string arg0 = "Some(";
                                    
                                    uw_Basis_string arg1 =
                                     uw_Basis_intToString(ctx, __uwr_x_17);
                                     uw_Basis_string arg2 = ")";
                                      uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                             NULL);
                                   });
                                })
                                :
                               ({
                                uw_Basis_string
                                tmp;
                                uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                tmp;
                                });
                             });
                            uw_Basis_string arg1 = " ";
                             
                            uw_Basis_string arg2 =
                             ({
                              uw_Basis_float* disc =
                              __uwr_f_3;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_Basis_float __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      uw_Basis_floatToString(ctx, __uwr_x_17);
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg3 = " ";
                              
                            uw_Basis_string arg4 =
                             ({
                              uw_Basis_string disc =
                              __uwr_s_4;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_Basis_string __uwr_x_17 = disc;
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     uw_Basis_string arg1 = __uwr_x_17;
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg5 = " ";
                              
                            uw_Basis_string arg6 =
                             ({
                              uw_Basis_bool* disc =
                              __uwr_b_5;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_Basis_bool __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      uw_Basis_boolToString(ctx, __uwr_x_17);
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg7 = " ";
                              
                            uw_Basis_string arg8 =
                             ({
                              uw_unit* disc =
                              __uwr_u_6;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_unit __uwr_x_17 = (*disc);
                                   "Some(())";
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg9 = " ";
                              
                            uw_Basis_string arg10 =
                             ({
                              struct __uws_3* disc =
                              __uwr_r_7;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({struct __uws_3 __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      uw_Basis_intToString(ctx,
                                       __uwr_x_17.__uwf_X);
                                      
                                     uw_Basis_string arg2 = __uwr_x_17.__uwf_Y;
                                      uw_Basis_string arg3 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2, 
                                                              arg3, NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg11 = " ";
                              
                            uw_Basis_string arg12 =
                             ({
                              struct __uws_4* disc =
                              __uwr_p_8;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({struct __uws_4 __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      ({
                                       struct __uws_4 disc =
                                       __uwr_x_17;
                                       
                                       1 && 1 ?
                                        ({uw_Basis_int __uwr_a_18 =
                                           disc.__uwf_1;
                                           
                                           uw_Basis_string __uwr_b_19 =
                                            disc.__uwf_2;
                                            ({
                                             uw_Basis_string arg0 =
                                              uw_Basis_intToString(ctx,
                                               __uwr_a_18);
                                              
                                              uw_Basis_string arg1 =
                                               __uwr_b_19;
                                               uw_Basis_strcat(ctx, arg0, arg1);
                                             });
                                         })
                                         :
                                        ({
                                         uw_Basis_string
                                         tmp;
                                         uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:29:59-29:65: pattern match failure");
                                         tmp;
                                         });
                                       });
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg13 = " ";
                              
                            uw_Basis_string arg14 =
                             ({
                              uw_Basis_int** disc =
                              __uwr_oo_9;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_Basis_int* __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      ({
                                       uw_Basis_int* disc =
                                       __uwr_x_17;
                                       
                                       disc == NULL ? "None"
                                         :
                                        disc != NULL && 1 ?
                                         ({uw_Basis_int __uwr_x_18 = (*disc);
                                            ({
                                             uw_Basis_string arg0 = "Some(";
                                              
                                              uw_Basis_string arg1 =
                                               uw_Basis_intToString(ctx,
                                                __uwr_x_18);
                                               uw_Basis_string arg2 = ")";
                                                uw_Basis_mstrcat(ctx, arg0, 
                                                                       arg1, 
                                                                       arg2,
                                                                            NULL);
                                             });
                                          })
                                          :
                                         ({
                                          uw_Basis_string
                                          tmp;
                                          uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                          tmp;
                                          });
                                       });
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg15 = " ";
                              
                            uw_Basis_string arg16 =
                             ({
                              uw_Basis_int** disc =
                              __uwr_on_10;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_Basis_int* __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      ({
                                       uw_Basis_int* disc =
                                       __uwr_x_17;
                                       
                                       disc == NULL ? "None"
                                         :
                                        disc != NULL && 1 ?
                                         ({uw_Basis_int __uwr_x_18 = (*disc);
                                            ({
                                             uw_Basis_string arg0 = "Some(";
                                              
                                              uw_Basis_string arg1 =
                                               uw_Basis_intToString(ctx,
                                                __uwr_x_18);
                                               uw_Basis_string arg2 = ")";
                                                uw_Basis_mstrcat(ctx, arg0, 
                                                                       arg1, 
                                                                       arg2,
                                                                            NULL);
                                             });
                                          })
                                          :
                                         ({
                                          uw_Basis_string
                                          tmp;
                                          uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                          tmp;
                                          });
                                       });
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg17 = " ";
                              
                            uw_Basis_string arg18 =
                             ({
                              struct __uws_2** disc =
                              __uwr_l_11;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({struct __uws_2* __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      uw_Basis_intToString(ctx,
                                       ({
                                        uw_Basis_int arg0 = 0LL;
                                         struct __uws_2* arg1 = __uwr_x_17;
                                        __uwn_$lengthPRIME_unpoly_13(ctx,
                                        arg0, arg1);
                                        }));
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg19 = " ";
                              
                            uw_Basis_string arg20 =
                             ({
                              struct __uwd_shape_3* disc =
                              __uwr_d1_12;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({struct __uwd_shape_3* __uwr_x_17 = disc;
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      __uwn_showShape_11(ctx, __uwr_x_17);
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg21 = " ";
                              
                            uw_Basis_string arg22 =
                             ({
                              struct __uwd_shape_3* disc =
                              __uwr_d2_13;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({struct __uwd_shape_3* __uwr_x_17 = disc;
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      __uwn_showShape_11(ctx, __uwr_x_17);
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg23 = " ";
                              
                            uw_Basis_string arg24 =
                             ({
                              enum __uwe_enum_7* disc =
                              __uwr_e_14;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({enum __uwe_enum_7 __uwr_x_17 = (*disc);
                                   ({
                                    uw_Basis_string arg0 = "Some(";
                                     
                                     uw_Basis_string arg1 =
                                      __uwn_showEnum_12(ctx, __uwr_x_17);
                                      uw_Basis_string arg2 = ")";
                                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                              NULL);
                                    });
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_string arg25 = " ";
                              
                            uw_Basis_string arg26 =
                             ({
                              uw_Basis_time* disc =
                              __uwr_ti_15;
                              
                              disc == NULL ? "None"
                                :
                               disc != NULL && 1 ?
                                ({uw_Basis_time __uwr_x_17 = (*disc);
                                   "Some(time)";
                                 })
                                 :
                                ({
                                 uw_Basis_string
                                 tmp;
                                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                 tmp;
                                 });
                              });
                             uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, 
                                                    arg4, arg5, arg6, arg7, 
                                                    arg8, arg9, arg10, arg11, 
                                                    arg12, arg13, arg14, arg15, 
                                                    arg16, arg17, arg18, arg19, 
                                                    arg20, arg21, arg22, arg23, 
                                                    arg24, arg25, arg26, NULL);
                           });
                         });
                        });
                       });
                      });
                     });
                    });
                   });
                  });
                 });
                });
               });
              });
             });
            });
           }));
   }
  
  static uw_Basis_string
   __uwn_bad_15(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int* __uwr_i_2 =
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
               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:33:29-33:51: %s", 
               "i");
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
           ({
            uw_Basis_float* __uwr_f_3 =
            ({
             uw_io_transaction __uwt;
             uw_Basis_float
             *__uwr = NULL;
             uw_set_could_write_db(ctx, 0);
             uw_io_transaction_begin(ctx, &__uwt);
             while (1) {
             int __uwfk = setjmp(*uw_jmp_buf(ctx));
              if (__uwfk == 0) {
              uw_Basis_float __uwv =
               ({
                uw_Basis_float
                tmp;
                uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:34:29-34:51: %s", 
                "f");
                tmp;
                });
               __uwr = uw_malloc(ctx, sizeof(uw_Basis_float));
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
             uw_Basis_string __uwr_s_4 =
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
                 uw_Basis_string
                 tmp;
                 uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:35:29-35:51: %s", 
                 "s");
                 tmp;
                 });
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
              uw_Basis_bool* __uwr_b_5 =
              ({
               uw_io_transaction __uwt;
               uw_Basis_bool
               *__uwr = NULL;
               uw_set_could_write_db(ctx, 0);
               uw_io_transaction_begin(ctx, &__uwt);
               while (1) {
               int __uwfk = setjmp(*uw_jmp_buf(ctx));
                if (__uwfk == 0) {
                uw_Basis_bool __uwv =
                 ({
                  uw_Basis_bool
                  tmp;
                  uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:36:29-36:51: %s", 
                  "b");
                  tmp;
                  });
                 __uwr = uw_malloc(ctx, sizeof(uw_Basis_bool));
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
               uw_unit* __uwr_u_6 =
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
                   uw_unit
                   tmp;
                   uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:37:29-37:51: %s", 
                   "u");
                   tmp;
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
                struct __uws_3* __uwr_r_7 =
                ({
                 uw_io_transaction __uwt;
                 struct __uws_3
                 *__uwr = NULL;
                 uw_set_could_write_db(ctx, 0);
                 uw_io_transaction_begin(ctx, &__uwt);
                 while (1) {
                 int __uwfk = setjmp(*uw_jmp_buf(ctx));
                  if (__uwfk == 0) {
                  struct __uws_3 __uwv =
                   ({
                    struct __uws_3
                    tmp;
                    uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:38:29-38:51: %s", 
                    "r");
                    tmp;
                    });
                   __uwr = uw_malloc(ctx, sizeof(struct __uws_3));
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
                 struct __uws_4* __uwr_p_8 =
                 ({
                  uw_io_transaction __uwt;
                  struct __uws_4
                  *__uwr = NULL;
                  uw_set_could_write_db(ctx, 0);
                  uw_io_transaction_begin(ctx, &__uwt);
                  while (1) {
                  int __uwfk = setjmp(*uw_jmp_buf(ctx));
                   if (__uwfk == 0) {
                   struct __uws_4 __uwv =
                    ({
                     struct __uws_4
                     tmp;
                     uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:39:29-39:51: %s", 
                     "p");
                     tmp;
                     });
                    __uwr = uw_malloc(ctx, sizeof(struct __uws_4));
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
                  uw_Basis_int** __uwr_oo_9 =
                  ({
                   uw_io_transaction __uwt;
                   uw_Basis_int*
                   *__uwr = NULL;
                   uw_set_could_write_db(ctx, 0);
                   uw_io_transaction_begin(ctx, &__uwt);
                   while (1) {
                   int __uwfk = setjmp(*uw_jmp_buf(ctx));
                    if (__uwfk == 0) {
                    uw_Basis_int* __uwv =
                     ({
                      uw_Basis_int*
                      tmp;
                      uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:40:30-40:53: %s", 
                      "oo");
                      tmp;
                      });
                     __uwr = uw_malloc(ctx, sizeof(uw_Basis_int*));
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
                   struct __uws_2** __uwr_l_10 =
                   ({
                    uw_io_transaction __uwt;
                    struct __uws_2*
                    *__uwr = NULL;
                    uw_set_could_write_db(ctx, 0);
                    uw_io_transaction_begin(ctx, &__uwt);
                    while (1) {
                    int __uwfk = setjmp(*uw_jmp_buf(ctx));
                     if (__uwfk == 0) {
                     struct __uws_2* __uwv =
                      ({
                       struct __uws_2*
                       tmp;
                       uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:41:29-41:51: %s", 
                       "l");
                       tmp;
                       });
                      __uwr = uw_malloc(ctx, sizeof(struct __uws_2*));
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
                    struct __uwd_shape_3* __uwr_d1_11 =
                    ({
                     uw_io_transaction __uwt;
                     struct __uwd_shape_3*
                     __uwr;
                     uw_set_could_write_db(ctx, 0);
                     uw_io_transaction_begin(ctx, &__uwt);
                     while (1) {
                     int __uwfk = setjmp(*uw_jmp_buf(ctx));
                      if (__uwfk == 0) {
                      __uwr =
                       ({
                        struct __uwd_shape_3*
                        tmp;
                        uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:42:30-42:53: %s", 
                        "d1");
                        tmp;
                        });
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
                     enum __uwe_enum_7* __uwr_e_12 =
                     ({
                      uw_io_transaction __uwt;
                      enum __uwe_enum_7
                      *__uwr = NULL;
                      uw_set_could_write_db(ctx, 0);
                      uw_io_transaction_begin(ctx, &__uwt);
                      while (1) {
                      int __uwfk = setjmp(*uw_jmp_buf(ctx));
                       if (__uwfk == 0) {
                       enum __uwe_enum_7 __uwv =
                        ({
                         enum __uwe_enum_7
                         tmp;
                         uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:43:29-43:51: %s", 
                         "e");
                         tmp;
                         });
                        __uwr = uw_malloc(ctx, sizeof(enum __uwe_enum_7));
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
                      uw_Basis_time* __uwr_ti_13 =
                      ({
                       uw_io_transaction __uwt;
                       uw_Basis_time
                       *__uwr = NULL;
                       uw_set_could_write_db(ctx, 0);
                       uw_io_transaction_begin(ctx, &__uwt);
                       while (1) {
                       int __uwfk = setjmp(*uw_jmp_buf(ctx));
                        if (__uwfk == 0) {
                        uw_Basis_time __uwv =
                         ({
                          uw_Basis_time
                          tmp;
                          uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:44:30-44:53: %s", 
                          "ti");
                          tmp;
                          });
                         __uwr = uw_malloc(ctx, sizeof(uw_Basis_time));
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
                       uw_unit __uwr___14 =
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
                                                   __uwr_n_0;
                                                   
                                                   uw_ensure_transaction(ctx);
                                                   
                                                   uw_conn *conn = uw_get_db(ctx);
                                                    sqlite3_stmt *stmt = conn->p0;
                                                    
                                                    if (stmt == NULL) {
                                                    if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                                     conn->p0 = stmt;
                                                     }
                                                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                                    uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                                    if (sqlite3_bind_int64(stmt, 1, 
                                                         arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:45:4-45:59: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                                     
                                                    int r;
                                                     if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                                     sleep(1);
                                                      uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                      }
                                                     
                                                     if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:45:4-45:59: DML step failed: %s<br />%s", 
                                                                            "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                            else if (uw_dml_log_enabled()) {
                                                     char *expanded = sqlite3_expanded_sql(stmt);
                                                      uw_log_dml(ctx, "tests/io_shapes/attempt_types/attempt_types.ur:45:4-45:59", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
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
                         uw_Basis_string arg0 =
                          ({
                           uw_Basis_int* disc =
                           __uwr_i_2;
                           
                           disc == NULL ? "None"
                             :
                            disc != NULL && 1 ?
                             ({uw_Basis_int __uwr_x_15 = (*disc);
                                ({
                                 uw_Basis_string arg0 = "Some(";
                                  
                                  uw_Basis_string arg1 =
                                   uw_Basis_intToString(ctx, __uwr_x_15);
                                   uw_Basis_string arg2 = ")";
                                    uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                           NULL);
                                 });
                              })
                              :
                             ({
                              uw_Basis_string
                              tmp;
                              uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                              tmp;
                              });
                           });
                          uw_Basis_string arg1 = " ";
                           
                          uw_Basis_string arg2 =
                           ({
                            uw_Basis_float* disc =
                            __uwr_f_3;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({uw_Basis_float __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    uw_Basis_floatToString(ctx, __uwr_x_15);
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg3 = " ";
                            
                          uw_Basis_string arg4 =
                           ({
                            uw_Basis_string disc =
                            __uwr_s_4;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({uw_Basis_string __uwr_x_15 = disc;
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   uw_Basis_string arg1 = __uwr_x_15;
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg5 = " ";
                            
                          uw_Basis_string arg6 =
                           ({
                            uw_Basis_bool* disc =
                            __uwr_b_5;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({uw_Basis_bool __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    uw_Basis_boolToString(ctx, __uwr_x_15);
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg7 = " ";
                            
                          uw_Basis_string arg8 =
                           ({
                            uw_unit* disc =
                            __uwr_u_6;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({uw_unit __uwr_x_15 = (*disc);
                                 "Some(())";
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg9 = " ";
                            
                          uw_Basis_string arg10 =
                           ({
                            struct __uws_3* disc =
                            __uwr_r_7;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({struct __uws_3 __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    uw_Basis_intToString(ctx,
                                     __uwr_x_15.__uwf_X);
                                    uw_Basis_string arg2 = __uwr_x_15.__uwf_Y;
                                     uw_Basis_string arg3 = ")";
                                      uw_Basis_mstrcat(ctx, arg0, arg1, arg2, 
                                                             arg3, NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg11 = " ";
                            
                          uw_Basis_string arg12 =
                           ({
                            struct __uws_4* disc =
                            __uwr_p_8;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({struct __uws_4 __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    ({
                                     struct __uws_4 disc =
                                     __uwr_x_15;
                                     
                                     1 && 1 ?
                                      ({uw_Basis_int __uwr_a_16 = disc.__uwf_1;
                                         
                                         uw_Basis_string __uwr_b_17 =
                                          disc.__uwf_2;
                                          ({
                                           uw_Basis_string arg0 =
                                            uw_Basis_intToString(ctx,
                                             __uwr_a_16);
                                            uw_Basis_string arg1 = __uwr_b_17;
                                             uw_Basis_strcat(ctx, arg0, arg1);
                                           });
                                       })
                                       :
                                      ({
                                       uw_Basis_string
                                       tmp;
                                       uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:47:59-47:65: pattern match failure");
                                       tmp;
                                       });
                                     });
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg13 = " ";
                            
                          uw_Basis_string arg14 =
                           ({
                            uw_Basis_int** disc =
                            __uwr_oo_9;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({uw_Basis_int* __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    ({
                                     uw_Basis_int* disc =
                                     __uwr_x_15;
                                     
                                     disc == NULL ? "None"
                                       :
                                      disc != NULL && 1 ?
                                       ({uw_Basis_int __uwr_x_16 = (*disc);
                                          ({
                                           uw_Basis_string arg0 = "Some(";
                                            
                                            uw_Basis_string arg1 =
                                             uw_Basis_intToString(ctx,
                                              __uwr_x_16);
                                             uw_Basis_string arg2 = ")";
                                              uw_Basis_mstrcat(ctx, arg0, arg1, 
                                                                     arg2, NULL);
                                           });
                                        })
                                        :
                                       ({
                                        uw_Basis_string
                                        tmp;
                                        uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                                        tmp;
                                        });
                                     });
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg15 = " ";
                            
                          uw_Basis_string arg16 =
                           ({
                            struct __uws_2** disc =
                            __uwr_l_10;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({struct __uws_2* __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    uw_Basis_intToString(ctx,
                                     ({
                                      uw_Basis_int arg0 = 0LL;
                                       struct __uws_2* arg1 = __uwr_x_15;
                                      __uwn_$lengthPRIME_unpoly_13(ctx,
                                      arg0, arg1);
                                      }));
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg17 = " ";
                            
                          uw_Basis_string arg18 =
                           ({
                            struct __uwd_shape_3* disc =
                            __uwr_d1_11;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({struct __uwd_shape_3* __uwr_x_15 = disc;
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    __uwn_showShape_11(ctx, __uwr_x_15);
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg19 = " ";
                            
                          uw_Basis_string arg20 =
                           ({
                            enum __uwe_enum_7* disc =
                            __uwr_e_12;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({enum __uwe_enum_7 __uwr_x_15 = (*disc);
                                 ({
                                  uw_Basis_string arg0 = "Some(";
                                   
                                   uw_Basis_string arg1 =
                                    __uwn_showEnum_12(ctx, __uwr_x_15);
                                    uw_Basis_string arg2 = ")";
                                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2,
                                                                            NULL);
                                  });
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_string arg21 = " ";
                            
                          uw_Basis_string arg22 =
                           ({
                            uw_Basis_time* disc =
                            __uwr_ti_13;
                            
                            disc == NULL ? "None"
                              :
                             disc != NULL && 1 ?
                              ({uw_Basis_time __uwr_x_15 = (*disc);
                                 "Some(time)";
                               })
                               :
                              ({
                               uw_Basis_string
                               tmp;
                               uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                               tmp;
                               });
                            });
                           uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, 
                                                  arg5, arg6, arg7, arg8, arg9, 
                                                  arg10, arg11, arg12, arg13, 
                                                  arg14, arg15, arg16, arg17, 
                                                  arg18, arg19, arg20, arg21, 
                                                  arg22, NULL);
                         });
                       });
                      });
                     });
                    });
                   });
                  });
                 });
                });
               });
              });
             });
            });
           }));
   }
  
  static uw_Basis_string
   __uwn_late_16(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_string arg0 =
            ({
             struct __uws_3* disc =
             ({
              uw_io_transaction __uwt;
              struct __uws_3
              *__uwr = NULL;
              uw_set_could_write_db(ctx, 1);
              uw_io_transaction_begin(ctx, &__uwt);
              while (1) {
              int __uwfk = setjmp(*uw_jmp_buf(ctx));
               if (__uwfk == 0) {
               struct __uws_3 __uwv =
                ({
                 uw_Basis_int __uwr_c_2 =
                 (uw_begin_region(ctx), ({
                                         struct __uws_5* disc =
                                         ({
                                          struct __uws_5* acc =
                                          NULL;
                                          int dummy = (uw_begin_region(ctx), 0);
                                          uw_ensure_transaction(ctx);
                                          
                                           
                                           uw_conn *conn = uw_get_db(ctx);
                                            sqlite3_stmt *stmt = conn->p1;
                                             
                                             if (stmt == NULL) {
                                             if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T<br />%s", sqlite3_errmsg(conn->conn));
                                            conn->p1 = stmt;
                                             }
                                             
                                             uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                             uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                             
                                            
                                            int r;
                                             sqlite3_reset(stmt);
                                             uw_end_region(ctx);
                                             while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                                             struct __uws_5 __uwr_r_2;
                                              struct __uws_5* __uwr_acc_3 =
                                              acc;
                                              
                                              __uwr_r_2.__uwf_1 =
                                               (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? 
                                                ({uw_Basis_int
                                                 tmp;
                                                 uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                 tmp;
                                                 }) : sqlite3_column_int64(stmt, 0));
                                               
                                              
                                              acc =
                                              ({
                                               struct __uws_5 *tmp =
                                               uw_malloc(ctx, sizeof(struct
                                                                      __uws_5));
                                               *tmp = __uwr_r_2;
                                               tmp;
                                               });
                                              }
                                             
                                             if (r == SQLITE_BUSY) {sleep(1);
                                                                     uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                                                     }
                                             
                                             if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: query step failed: %s<br />%s", 
                                             "SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T", sqlite3_errmsg(conn->conn));
                                             
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
                                           ({struct __uws_5 __uwr_r_2 =
                                              (*disc);
                                              __uwr_r_2.__uwf_1;
                                            })
                                            :
                                           ({
                                            uw_Basis_int
                                            tmp;
                                            uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:53:29-53:179: pattern match failure");
                                            tmp;
                                            });
                                         }));
                 uw_end_region(ctx);
                  ({
                   uw_unit __uwr___3 =
                   (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                           uw_Basis_int arg1 = __uwr_n_0;
                                            
                                            uw_ensure_transaction(ctx);
                                            
                                            uw_conn *conn = uw_get_db(ctx);
                                             sqlite3_stmt *stmt = conn->p0;
                                             
                                             if (stmt == NULL) {
                                             if (sqlite3_prepare_v2(conn->conn, "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)<br />%s", sqlite3_errmsg(conn->conn));
                                              conn->p0 = stmt;
                                              }
                                             uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                                             uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                                             if (sqlite3_bind_int64(stmt, 1, arg1) != SQLITE_OK) uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:53:71-53:179: Error binding parameter #1: %s", sqlite3_errmsg(conn->conn));
                                              
                                             int r;
                                              if ((r = sqlite3_step(stmt)) == SQLITE_BUSY) {
                                              sleep(1);
                                               uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                               }
                                              
                                              if (r != SQLITE_DONE) uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:53:71-53:179: DML step failed: %s<br />%s", 
                                                                     "INSERT INTO uw_Attempt_types_t (uw_N) VALUES (\?)", sqlite3_errmsg(conn->conn));
                                                                     else if (uw_dml_log_enabled()) {
                                              char *expanded = sqlite3_expanded_sql(stmt);
                                               uw_log_dml(ctx, "tests/io_shapes/attempt_types/attempt_types.ur:53:71-53:179", expanded ? expanded : sqlite3_sql(stmt), sqlite3_changes(conn->conn));
                                               sqlite3_free(expanded);
                                               }
                                              uw_pop_cleanup(ctx);
                                             uw_pop_cleanup(ctx);
                                             
                                           
                                           uw_end_region(ctx);
                                           0;
                                           })));
                   uw_end_region(ctx);
                    ({
                     uw_Basis_bool disc =
                     ! (__uwr_c_2 < 0LL);
                     
                     disc == uw_Basis_True ?
                      ({
                       struct __uws_3
                       tmp;
                       uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:53:126-53:151: %s", 
                       "late");
                       tmp;
                       })
                       :
                      disc == uw_Basis_False ?
                       ({ struct __uws_3 tmp = {__uwr_c_2, "late"}; tmp; })
                        :
                       ({
                        struct __uws_3
                        tmp;
                        uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:53:71-53:179: pattern match failure");
                        tmp;
                        });
                     });
                   });
                 });
                __uwr = uw_malloc(ctx, sizeof(struct __uws_3));
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
              disc != NULL && 1 ?
               ({struct __uws_3 __uwr_x_2 = (*disc);
                  ({
                   uw_Basis_string arg0 = "Some(";
                    
                    uw_Basis_string arg1 =
                     uw_Basis_intToString(ctx, __uwr_x_2.__uwf_X);
                     uw_Basis_string arg2 = __uwr_x_2.__uwf_Y;
                      uw_Basis_string arg3 = ")";
                       uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, NULL);
                   });
                })
                :
               ({
                uw_Basis_string
                tmp;
                uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:10:47-12:0: pattern match failure");
                tmp;
                });
             });
            uw_Basis_string arg1 = " rows ";
             
            uw_Basis_string arg2 =
             uw_Basis_intToString(ctx,
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
                  struct __uws_5* disc =
                  ({
                   struct __uws_5* acc =
                   NULL;
                   int dummy = (uw_begin_region(ctx), 0);
                   uw_ensure_transaction(ctx);
                   
                    
                    uw_conn *conn = uw_get_db(ctx);
                     sqlite3_stmt *stmt = conn->p1;
                      
                      if (stmt == NULL) {
                      if (sqlite3_prepare_v2(conn->conn, "SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T", -1, &stmt, NULL) != SQLITE_OK) uw_error(ctx, FATAL, "Error preparing statement: SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T<br />%s", sqlite3_errmsg(conn->conn));
                     conn->p1 = stmt;
                      }
                      
                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_clear_bindings, stmt);
                      uw_push_cleanup(ctx, (void (*)(void *))sqlite3_reset, stmt);
                      
                     
                     int r;
                      sqlite3_reset(stmt);
                      uw_end_region(ctx);
                      while ((r = sqlite3_step(stmt)) == SQLITE_ROW) {
                      struct __uws_5 __uwr_r_2;
                       struct __uws_5* __uwr_acc_3 =
                       acc;
                       
                       __uwr_r_2.__uwf_1 =
                        (sqlite3_column_type(stmt, 0) == SQLITE_NULL ? ({uw_Basis_int
                                                                        tmp;
                                                                        uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: Unexpectedly NULL field #0");
                                                                        tmp;
                                                                        }) : sqlite3_column_int64(stmt, 0));
                        
                       
                       acc =
                       ({
                        struct __uws_5 *tmp =
                        uw_malloc(ctx, sizeof(struct __uws_5));
                        *tmp = __uwr_r_2;
                        tmp;
                        });
                       }
                      
                      if (r == SQLITE_BUSY) {sleep(1);
                                              uw_error(ctx, UNLIMITED_RETRY, "Database is busy");
                                              }
                      
                      if (r != SQLITE_DONE) uw_error(ctx, FATAL, "$/top.ur:437:17-437:18: query step failed: %s<br />%s", 
                      "SELECT COUNT(*) AS uw_1 FROM uw_Attempt_types_t AS T_T", sqlite3_errmsg(conn->conn));
                      
                      uw_pop_cleanup(ctx);
                     uw_pop_cleanup(ctx);
                      
                   acc;
                   });
                  
                  disc == NULL ?
                   ({
                    uw_Basis_int
                    tmp;
                    uw_error(ctx, FATAL, "$/top.ur:438:24-439:3: %s", "Query returned no rows");
                    tmp;
                    })
                    :
                   disc != NULL && 1 ?
                    ({struct __uws_5 __uwr_r_2 = (*disc);
                       __uwr_r_2.__uwf_1;
                     })
                     :
                    ({
                     uw_Basis_int
                     tmp;
                     uw_error(ctx, FATAL, "tests/io_shapes/attempt_types/attempt_types.ur:54:9-54:61: pattern match failure");
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
             uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_17(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_s_2 =
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
                                                                              ")'>ok</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>bad</button>\n<button onclick='uw_event=event;exec("), 0),
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
                                                                               ")'>late</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
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
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Attempt_types/main")) return 1;
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
  
  
  if (!strcmp(request, "/app.5C65B8F6E7A50E48B5DE272294DA2D90A8A4AEB3.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Attempt_types/main", 19) && (request[19] == 0 || request[19] == '/')) {
  request += 19;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.5C65B8F6E7A50E48B5DE272294DA2D90A8A4AEB3.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_17(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Attempt_types/ok", 17) && (request[17] == 0 || request[17] == '/')) {
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
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     uw_Basis_string it0 =
      __uwn_ok_14(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Attempt_types/bad", 18) && (request[18] == 0 || request[18] == '/')) {
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
     uw_Basis_string it0 =
      __uwn_bad_15(ctx, arg0, 0);
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
  
  if (!strncmp(request, "/Attempt_types/late", 19) && (request[19] == 0 || request[19] == '/')) {
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
     uw_Basis_string it0 =
      __uwn_late_16(ctx, arg0, 0);
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
 