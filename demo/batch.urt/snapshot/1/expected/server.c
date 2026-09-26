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
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_batch_t'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_batch_t'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_batch_t' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_batch_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_a' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_batch_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_a' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_batch_t' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_batch_t' AND LOWER(column_name) LIKE 'uw_%'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_batch_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_batch_t' has extra columns.");
      }
     
     PQclear(res);
     }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "SELECT T_T.uw_A, T_T.uw_Id FROM uw_Batch_t AS T_T", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_A, T_T.uw_Id FROM uw_Batch_t AS T_T\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "INSERT INTO uw_Batch_t (uw_A, uw_Id) VALUES ($1::text, $2::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Batch_t (uw_A, uw_Id) VALUES ($1::text, $2::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw2", "DELETE FROM uw_Batch_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Batch_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
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
   uw_Basis_int __uwf_1;
    uw_Basis_string __uwf_2;
     };
  struct __uws_2 {
   struct __uws_1 __uwf_1;
    struct __uws_2* __uwf_2;
     };
  
  struct __uws_3 {
   uw_Basis_string __uwf_A;
    uw_Basis_int __uwf_Id;
     };
  struct __uws_4 {
   struct __uws_3 __uwf_T;
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
  
  /* SQL table uw_Batch_t keys uw_Id constraints   */
   
  
  static struct __uws_2*
   __uwn_allRows_6(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_2* acc =
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
               uw_error(ctx, FATAL, "demo/batch.ur:7:4-7:9: Query failed:\n%s\n%s", 
               "SELECT T_T.uw_A, T_T.uw_Id FROM uw_Batch_t AS T_T", PQerrorMessage(conn));
               }
              
              if (PQnfields(res) != 2) {
              int nf = PQnfields(res);
               PQclear(res);
               uw_error(ctx, FATAL, "demo/batch.ur:7:4-7:9: Query returned %d columns instead of 2:\n%s\n%s", nf, 
               "SELECT T_T.uw_A, T_T.uw_Id FROM uw_Batch_t AS T_T", PQerrorMessage(conn));
               }
              
              uw_end_region(ctx);
              uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
              n = PQntuples(res);
              for (i = 0; i < n; ++i) {
              struct __uws_4 __uwr_r_2;
               struct __uws_2* __uwr_acc_3 =
               acc;
               
               __uwr_r_2.__uwf_T.__uwf_A =
                (PQgetisnull(res, i, 0) ? ({uw_Basis_string
                                           tmp;
                                           uw_error(ctx, FATAL, "demo/batch.ur:7:4-7:9: Unexpectedly NULL field #0");
                                           tmp;
                                           }) : uw_strdup(ctx, PQgetvalue(res, i, 0)));
                
                __uwr_r_2.__uwf_T.__uwf_Id =
                 (PQgetisnull(res, i, 1) ? ({uw_Basis_int
                                            tmp;
                                            uw_error(ctx, FATAL, "demo/batch.ur:7:4-7:9: Unexpectedly NULL field #1");
                                            tmp;
                                            }) : uw_Basis_stringToInt_error(ctx, 
                                                  PQgetvalue(res, i, 1)));
                 
               
               acc =
               ({
                struct __uws_2 *tmp =
                uw_malloc(ctx, sizeof(struct __uws_2));
                *tmp =
                ({ struct __uws_2 tmp =
                 {({ struct __uws_1 tmp =
                   {__uwr_r_2.__uwf_T.__uwf_Id, __uwr_r_2.__uwf_T.__uwf_A};
                   tmp; }), __uwr_acc_3}; tmp; });
                tmp;
                });
               }
              
              uw_pop_cleanup(ctx);
              
           acc;
           }));
   }
  
  static uw_unit __uwn_doBatch_7(uw_context, struct __uws_2*, uw_unit);
   static uw_unit
    __uwn_doBatch_7(uw_context ctx, struct __uws_2* __uwr_ls_0, 
                     uw_unit __uwr___1)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_0;
            
            disc == NULL ? 0
              :
             disc != NULL && 1 && 1 && 1 ?
              ({uw_Basis_int __uwr_id_2 = (*disc).__uwf_1.__uwf_1;
                 uw_Basis_string __uwr_a_3 = (*disc).__uwf_1.__uwf_2;
                  struct __uws_2* __uwr_lsPRIME_4 = (*disc).__uwf_2;
                   ({
                    uw_unit __uwr___5 =
                    (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                            uw_Basis_string arg1 = __uwr_a_3;
                                             uw_Basis_int arg2 = __uwr_id_2;
                                             
                                             uw_ensure_transaction(ctx);
                                             
                                             PGconn *conn = uw_get_db(ctx);
                                              static const int paramFormats[] = { 
                                               0, 0 };
                                               const int *paramLengths = paramFormats;
                                                const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
                                               paramValues[0] = arg1;
                                                
                                                paramValues[1] = uw_Basis_attrifyInt(ctx, 
                                                                  arg2);
                                                 
                                               
                                              PGresult *res;
                                              
                                              res = PQexecPrepared(conn, "uw1", 2, paramValues, paramLengths, paramFormats, 0);
                                              
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
                                                 uw_error(ctx, FATAL, "demo/batch.ur:15:8-16:19: DML failed:\n%s\n%s", 
                                                 "INSERT INTO uw_Batch_t (uw_A, uw_Id) VALUES ($1::text, $2::int8)", PQerrorMessage(conn));
                                                }
                                                  
                                                  PQclear(res);
                                                  
                                            
                                            uw_end_region(ctx);
                                            0;
                                            })));
                    uw_end_region(ctx);
                     ({
                      struct __uws_2* rearg0 = __uwr_lsPRIME_4;
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
               uw_error(ctx, FATAL, "demo/batch.ur:11:0-18:0: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_del_8(uw_context ctx, uw_Basis_int __uwr_id_0, uw_unit __uwr___1)
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
             
             res = PQexecPrepared(conn, "uw2", 1, paramValues, paramLengths, paramFormats, 0);
             
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
                "DELETE FROM uw_Batch_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
               }
                 
                 PQclear(res);
                 
           
           uw_end_region(ctx);
           0;
           })));
   }
  
  static uw_unit
   __uwn_show_22(uw_context ctx, uw_Basis_bool __uwr_withDel_0, 
                  uw_Basis_source __uwr_lss_1)
   {
   return(((uw_write(ctx, "<script type=\"text/javascript\">dyn(\"span\", execD({c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
           (({
             uw_Basis_bool disc =
             __uwr_withDel_0;
             
             disc == uw_Basis_True ? (uw_write(ctx, "true"), 0)
               :
              disc == uw_Basis_False ? (uw_write(ctx, "false"), 0)
                :
               ({
                uw_unit
                tmp;
                uw_error(ctx, FATAL, "demo/batch.ur:35:26-38:26: pattern match failure");
                tmp;
                });
             }),
            ((uw_write(ctx, "}},x:{c:\"c\",v:"), 0),
             (uw_begin_region(ctx), uw_Basis_htmlifySource_w(ctx, __uwr_lss_1),
              uw_end_region(ctx), (uw_write(ctx, "}},x:{c:\"c\",v:null}}))</script>"), 0))))));
   }
  
  static uw_Basis_string __uwn_jsify_18(uw_context, struct __uws_2*);
   static uw_Basis_string
    __uwn_jsify_18(uw_context ctx, struct __uws_2* __uwr_x_0)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_x_0;
            
            disc == NULL ? "null"
              :
             disc != NULL && 1 ?
              ({struct __uws_2 __uwr_x_1 = (*disc);
                 ({
                  uw_Basis_string arg0 = "{_1:{_1:";
                   
                   uw_Basis_string arg1 =
                    uw_Basis_htmlifyInt(ctx, __uwr_x_1.__uwf_1.__uwf_1);
                    uw_Basis_string arg2 = ",_2:";
                     
                   uw_Basis_string arg3 =
                    uw_Basis_jsifyString(ctx, __uwr_x_1.__uwf_1.__uwf_2);
                    uw_Basis_string arg4 = "},_2:";
                     
                   uw_Basis_string arg5 =
                    __uwn_jsify_18(ctx, __uwr_x_1.__uwf_2);
                    uw_Basis_string arg6 = "}";
                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, arg5, 
                                            arg6, NULL);
                  });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "demo/batch.ur:67:74-67:77: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_wrap_main_17(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_lss_2 =
           uw_Basis_new_client_source(ctx,
            ({
             uw_Basis_string arg0 = "{c:\"c\",v:";
              uw_Basis_string arg1 = __uwn_jsify_18(ctx, NULL);
               uw_Basis_string arg2 = "}";
                uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
             }));
           ({
            uw_Basis_source __uwr_batched_3 =
            uw_Basis_new_client_source(ctx,
             ({
              uw_Basis_string arg0 = "{c:\"c\",v:";
               uw_Basis_string arg1 = __uwn_jsify_18(ctx, NULL);
                uw_Basis_string arg2 = "}";
                 uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
              }));
            ({
             uw_Basis_source __uwr_id_4 =
             uw_Basis_new_client_source(ctx,
              ({
               uw_Basis_string arg0 = "{c:\"c\",v:";
                uw_Basis_string arg1 = uw_Basis_jsifyString(ctx, "");
                 uw_Basis_string arg2 = "}";
                  uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
               }));
             ({
              uw_Basis_source __uwr_a_5 =
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
                                     uw_end_region(ctx), ((uw_write(ctx, ">\n<h2>Rows</h2>\n"), 0),
                                                          (uw_begin_region(ctx),
                                                            ({
                                                             uw_Basis_bool arg0
                                                              = uw_Basis_True;
                                                              
                                                              uw_Basis_source
                                                               arg1 =
                                                               __uwr_lss_2;
                                                             __uwn_show_22(ctx,
                                                             arg0, arg1);
                                                             }),
                                                           uw_end_region(ctx), ((uw_write(ctx, 
                                                                               "\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:4},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_lss_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>Update</button><br />\n<br />\n<h2>Batch new rows to add</h2>\n<table>\n<tr> <th>Id:</th> <td><script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_id_4
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "));</script></td> </tr>\n<tr> <th>A:</th> <td><script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_a_5
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "));</script></td> </tr>\n<tr> <th></th> <td><button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:5},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_batched_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_id_4
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_a_5
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))))))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>Batch it</button></td> </tr>\n</table>\n<h2>Already batched:</h2>\n"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ({
                                                                               uw_Basis_bool
                                                                               arg0
                                                                               =
                                                                               uw_Basis_False;
                                                                               
                                                                               uw_Basis_source
                                                                               arg1
                                                                               =
                                                                               __uwr_batched_3;
                                                                               __uwn_show_22(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "\n<button onclick='uw_event=event;exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:6},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_batched_3
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               ")'>Execute</button>\n</body>"), 0))))))))))))))))));
              });
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
 
 static void urlify_3(uw_context, struct __uws_2 *);
  static struct __uws_2 *unurlify_3(uw_context, char **);
   static void urlify_3(uw_context ctx, struct __uws_2
    *it0) {
    if (it0) {
     struct __uws_2 it1 =
      *it0;
      uw_write(ctx, "Cons/");
      {
       struct __uws_1 it2 =
       it1.__uwf_1;
       {
        uw_Basis_int it3 = it2.__uwf_1;
        uw_Basis_urlifyInt_w(ctx, it3);
         }
        {
         uw_Basis_string it3 =
         it2.__uwf_2;
         uw_write(ctx, "/");
          uw_Basis_urlifyString_w(ctx, it3);
           }
         }
       {
        struct __uws_2* it2 =
        it1.__uwf_2;
        uw_write(ctx, "/");
         urlify_3(ctx, it2);
          }
        ;
      } else {
               uw_write(ctx, "Nil");
               }
     }
    
    static struct __uws_2
     *unurlify_3(uw_context ctx, char **request) {
     return ((*request)[0] == '/' ? ++*request : *request,
      ((!strncmp(*request, "Nil", 3) && ((*request)[3] == 0 || (*request)[3] == '/')) ? (*request
      +=
      3, NULL) : ((!strncmp(*request, "Cons", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? (*request
      +=
      4, ((*request)[0] == '/' ? ++*request : NULL), 
      ({
       struct __uws_2 *tmp = uw_malloc(ctx, sizeof(struct __uws_2));
       *tmp =
       ({
        struct __uws_1 uwr_1 =
         ({
          uw_Basis_int uwr_1 = uw_Basis_unurlifyInt(ctx, request);
           uw_Basis_string uwr_2 = uw_Basis_unurlifyString(ctx, request);
            struct __uws_1 tmp = { uwr_1, uwr_2 };
          tmp;
          });
         struct __uws_2* uwr_2 = unurlify_3(ctx, request);
          struct __uws_2 tmp = { uwr_1, uwr_2 };
        tmp;
        });
       tmp;
       }))
      : (uw_error(ctx, FATAL, "Error unurlifying datatype list_s"), NULL))));
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
  
  
  if (!strcmp(request, "/app.1F80A6C9DEC649921FAD4AE75DA652A98E06FD71.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 
 if (!strncmp(request, "/Batch/main", 11) && (request[11] == 0 || request[11] == '/')) {
  request += 11;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.1F80A6C9DEC649921FAD4AE75DA652A98E06FD71.js\"></script>\n");
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
  
  if (!strncmp(request, "/Batch/allRows", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
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
     struct __uws_2* it0 = __uwn_allRows_6(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     urlify_3(ctx, it0);
      return;
    }
    }
  
  if (!strncmp(request, "/Batch/doBatch", 14) && (request[14] == 0 || request[14] == '/')) {
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
    struct __uws_2* arg0 = unurlify_3(ctx, &request);
     uw_unit it0 = __uwn_doBatch_7(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     uw_Basis_urlifyString_w(ctx, "");
      return;
    }
    }
  
  if (!strncmp(request, "/Batch/del", 10) && (request[10] == 0 || request[10] == '/')) {
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
     uw_unit it0 = __uwn_del_8(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     uw_Basis_urlifyString_w(ctx, "");
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
 