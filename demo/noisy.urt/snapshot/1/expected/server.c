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
   static void uw_db_validate(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_noisy_t'");
     
     if (res == NULL) {
     PQfinish(conn);
      uw_error(ctx, FATAL, "Out of memory allocating query result.");
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_noisy_t'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_noisy_t' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_noisy_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_a' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))");
     
     if (res == NULL) {
     PQfinish(conn);
      uw_error(ctx, FATAL, "Out of memory allocating query result.");
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_noisy_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_a' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_noisy_t' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_noisy_t' AND LOWER(column_name) LIKE 'uw_%'");
     
     if (res == NULL) {
     PQfinish(conn);
      uw_error(ctx, FATAL, "Out of memory allocating query result.");
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_noisy_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_noisy_t' has extra columns.");
      }
     
     PQclear(res);
     }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "INSERT INTO uw_Noisy_t (uw_A, uw_Id) VALUES ($1::text, $2::int8)", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Noisy_t (uw_A, uw_Id) VALUES ($1::text, $2::int8)\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "DELETE FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw2", "SELECT T_T.uw_A FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_A FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
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
    PGconn *conn = PQconnectdb(env_db_str == NULL ? "dbname=test" : env_db_str);
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
   uw_Basis_string __uwf_A;
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
  
  /* SQL table uw_Noisy_t keys uw_Id constraints   */
   
  
  static uw_unit
   __uwn_add_3(uw_context ctx, uw_Basis_int __uwr_id_0, 
                uw_Basis_string __uwr_s_1, uw_unit __uwr___2)
   {
   return((uw_begin_region(ctx), ({
           uw_Basis_string arg1 = __uwr_s_1;
            uw_Basis_int arg2 = __uwr_id_0;
            
            uw_ensure_transaction(ctx);
            
            PGconn *conn = uw_get_db(ctx);
             static const int paramFormats[] = { 0, 0 };
              const int *paramLengths = paramFormats;
               const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
              paramValues[0] = arg1;
               paramValues[1] = uw_Basis_attrifyInt(ctx, arg2);
                
              
             PGresult *res;
             
             res = PQexecPrepared(conn, "uw0", 2, paramValues, paramLengths, paramFormats, 0);
             
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
                uw_error(ctx, FATAL, "$/basis.urs:720:0-720:33: DML failed:\n%s\n%s", 
                "INSERT INTO uw_Noisy_t (uw_A, uw_Id) VALUES ($1::text, $2::int8)", PQerrorMessage(conn));
               }
                 
                 PQclear(res);
                 
           
           uw_end_region(ctx);
           0;
           })));
   }
  
  static uw_unit
   __uwn_del_4(uw_context ctx, uw_Basis_int __uwr_id_0, uw_unit __uwr___1)
   {
   return((uw_begin_region(ctx), ({
           uw_Basis_int arg1 = __uwr_id_0;
            
            uw_ensure_transaction(ctx);
            
            PGconn *conn = uw_get_db(ctx);
             static const int paramFormats[] = { 0 };
              const int *paramLengths = paramFormats;
               const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
              paramValues[0] = uw_Basis_attrifyInt(ctx, arg1);
               
              
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
                uw_error(ctx, FATAL, "$/basis.urs:720:0-720:33: DML failed:\n%s\n%s", 
                "DELETE FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
               }
                 
                 PQclear(res);
                 
           
           uw_end_region(ctx);
           0;
           })));
   }
  
  static uw_Basis_string
   __uwn_lookup_5(uw_context ctx, uw_Basis_int __uwr_id_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_2* disc =
           ({
            struct __uws_2* acc =
            NULL;
            int dummy = (uw_begin_region(ctx), 0);
            uw_ensure_transaction(ctx);
            uw_Basis_int arg1 = __uwr_id_0;
             
             PGconn *conn = uw_get_db(ctx);
              static const int paramFormats[] = { 0 };
               const int *paramLengths = paramFormats;
                const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
               paramValues[0] = uw_Basis_attrifyInt(ctx, arg1);
                
               
              PGresult *res = PQexecPrepared(conn, "uw2", 1, paramValues, paramLengths, paramFormats, 0);
              
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
                uw_error(ctx, FATAL, "demo/noisy.ur:14:9-14:11: Query failed:\n%s\n%s", 
                "SELECT T_T.uw_A FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                }
               
               if (PQnfields(res) != 1) {
               int nf = PQnfields(res);
                PQclear(res);
                uw_error(ctx, FATAL, "demo/noisy.ur:14:9-14:11: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                "SELECT T_T.uw_A FROM uw_Noisy_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                }
               
               uw_end_region(ctx);
               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
               n = PQntuples(res);
               for (i = 0; i < n; ++i) {
               struct __uws_2 __uwr_r_2;
                struct __uws_2* __uwr_acc_3 =
                acc;
                
                __uwr_r_2.__uwf_T.__uwf_A =
                 (PQgetisnull(res, i, 0) ? ({uw_Basis_string
                                            tmp;
                                            uw_error(ctx, FATAL, "demo/noisy.ur:14:9-14:11: Unexpectedly NULL field #0");
                                            tmp;
                                            }) : uw_strdup(ctx, PQgetvalue(res, i, 0)));
                 
                
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
           
           disc == NULL ? NULL
             :
            disc != NULL && 1 ?
             ({struct __uws_2 __uwr_r_2 = (*disc);
                __uwr_r_2.__uwf_T.__uwf_A;
              })
              :
             ({
              uw_Basis_string
              tmp;
              uw_error(ctx, FATAL, "demo/noisy.ur:13:4-18:0: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_13(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_idAdd_2 =
           uw_Basis_new_client_source(ctx,
            ({
             uw_Basis_string arg0 = "{c:\"c\",v:";
              uw_Basis_string arg1 = uw_Basis_jsifyString(ctx, "");
               uw_Basis_string arg2 = "}";
                uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
             }));
           ({
            uw_Basis_source __uwr_aAdd_3 =
            uw_Basis_new_client_source(ctx,
             ({
              uw_Basis_string arg0 = "{c:\"c\",v:";
               uw_Basis_string arg1 = uw_Basis_jsifyString(ctx, "");
                uw_Basis_string arg2 = "}";
                 uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
              }));
            ({
             uw_Basis_source __uwr_idDel_4 =
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
                                    uw_end_region(ctx), ((uw_write(ctx, ">\n<button onclick='uw_event=event;exec({c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:null}})'>Check values of 1, 2, and 3</button><br />\n<br />\n<button onclick='uw_event=event;exec("), 0),
                                                         (uw_begin_region(ctx), 
                                                          ((uw_write(ctx, "{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:3},x:{c:\"c\",v:"), 0),
                                                           (uw_begin_region(ctx),
                                                             uw_Basis_htmlifySource_w(ctx,
                                                              __uwr_idAdd_2),
                                                            uw_end_region(ctx), 
                                                            ((uw_write(ctx, "}},x:{c:\"c\",v:"), 0),
                                                             (uw_begin_region(ctx),
                                                               uw_Basis_htmlifySource_w(ctx,
                                                                __uwr_aAdd_3),
                                                              uw_end_region(ctx),
                                                               (uw_write(ctx, "}},x:{c:\"c\",v:null}}"), 0))))),
                                                          uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ")'>Add</button>\n <script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_idAdd_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "));</script>\n <script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_aAdd_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "));</script><br />\n<br />\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:4},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_idDel_4
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>Delete</button>\n <script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_idDel_4
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "));</script>\n</body>"), 0))))))))))))));
             });
            });
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
  
  
  if (!strcmp(request, "/app.475E1BB8B0A29ED8EF2B7F228564B1DC25FA52E3.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 
 if (!strncmp(request, "/Noisy/main", 11) && (request[11] == 0 || request[11] == '/')) {
  request += 11;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.475E1BB8B0A29ED8EF2B7F228564B1DC25FA52E3.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_13(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Noisy/add", 10) && (request[10] == 0 || request[10] == '/')) {
   request += 10;
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
      uw_unit it0 = __uwn_add_3(ctx, arg0, arg1, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     uw_Basis_urlifyString_w(ctx, "");
      return;
    }
    }
  
  if (!strncmp(request, "/Noisy/del", 10) && (request[10] == 0 || request[10] == '/')) {
   request += 10;
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
     uw_unit it0 = __uwn_del_4(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     uw_Basis_urlifyString_w(ctx, "");
      return;
    }
    }
  
  if (!strncmp(request, "/Noisy/lookup", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     uw_Basis_string it0 = __uwn_lookup_5(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     if (it0) {uw_write(ctx, "Some/");
                uw_Basis_urlifyString_w(ctx, it0);
                 } else {
                          uw_write(ctx, "None");
                          }
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
                                                                  NULL};
 