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
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_crud3_t'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_crud3_t'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_crud3_t' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_crud3_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_text' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_crud3_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_text' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_crud3_t' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_crud3_t' AND LOWER(column_name) LIKE 'uw_%'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_crud3_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_crud3_t' has extra columns.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_crud3_seq' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)");
      
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
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM pg_class WHERE relname = 'uw_crud3_seq' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "1")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Sequence 'uw_Crud3_seq' does not exist.");
       }
      
      PQclear(res);
      }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "SELECT T_Tab.uw_Text FROM uw_Crud3_t AS T_Tab WHERE (T_Tab.uw_Id = $1::int8)", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_Tab.uw_Text FROM uw_Crud3_t AS T_Tab WHERE (T_Tab.uw_Id = $1::int8)\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "SELECT T_T.uw_Id, T_T.uw_Text FROM uw_Crud3_t AS T_T", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Id, T_T.uw_Text FROM uw_Crud3_t AS T_T\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw2", "UPDATE uw_Crud3_t AS T_T SET uw_Text = $1::text WHERE (T_T.uw_Id = $2::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nUPDATE uw_Crud3_t AS T_T SET uw_Text = $1::text WHERE (T_T.uw_Id = $2::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw3", "DELETE FROM uw_Crud3_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Crud3_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw4", "SELECT NEXTVAL('uw_Crud3_seq')", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT NEXTVAL('uw_Crud3_seq')\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw5", "INSERT INTO uw_Crud3_t (uw_Id, uw_Text) VALUES ($1::int8, $2::text)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Crud3_t (uw_Id, uw_Text) VALUES ($1::int8, $2::text)\n%s", msg);
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
   uw_Basis_string __uwf_Text;
    };
  struct __uws_2 {
   struct __uws_1 __uwf_Tab;
    };
  struct __uws_3 {
   uw_Basis_int __uwf_Id;
    uw_Basis_string __uwf_Text;
     };
  struct __uws_4 {
   struct __uws_3 __uwf_T;
    };
  struct __uws_5 {
   uw_Basis_string __uwf_A;
    uw_Basis_string __uwf_B;
     };
  struct __uws_6 {
   struct __uws_5 __uwf_Text;
    };
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Crud3_t keys uw_Id constraints   */
   
  /* SQL sequence uw_Crud3_seq */
   
  
  static uw_unit
   __uwn_wrap_confirm_3(uw_context ctx, uw_Basis_int __uwr_x0_0, 
                         uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<p>Are you sure you want to delete ID #"), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                              __uwr_x0_0
                                                                              ),
                                                       uw_end_region(ctx), ((uw_write(ctx, 
                                                                             "\?</p>\n<form method=\"post\" action=\"/Crud3/delete/"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              uw_Basis_urlifyInt_w(ctx,
                                                                               __uwr_x0_0
                                                                               ),
                                                                             uw_end_region(ctx),
                                                                              (uw_write(ctx, 
                                                                               "\"><input type=\"submit\" value=\"I was born sure!\" /></form>\n</body>"), 0)))))))));
   }
  
  static uw_unit
   __uwn_wrap_upd_4(uw_context ctx, uw_Basis_int __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_2* disc =
           ({
            struct __uws_2* acc =
            NULL;
            int dummy = (uw_begin_region(ctx), 0);
            uw_ensure_transaction(ctx);
            uw_Basis_int arg1 = __uwr_x0_0;
             
             PGconn *conn = uw_get_db(ctx);
              static const int paramFormats[] = { 0 };
               const int *paramLengths = paramFormats;
                const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
               paramValues[0] = uw_Basis_attrifyInt(ctx, arg1);
                
               
              PGresult *res = PQexecPrepared(conn, "uw0", 1, paramValues, paramLengths, paramFormats, 0);
              
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
                uw_error(ctx, FATAL, "demo/crud.ur:127:17-127:20: Query failed:\n%s\n%s", 
                "SELECT T_Tab.uw_Text FROM uw_Crud3_t AS T_Tab WHERE (T_Tab.uw_Id = $1::int8)", PQerrorMessage(conn));
                }
               
               if (PQnfields(res) != 1) {
               int nf = PQnfields(res);
                PQclear(res);
                uw_error(ctx, FATAL, "demo/crud.ur:127:17-127:20: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                "SELECT T_Tab.uw_Text FROM uw_Crud3_t AS T_Tab WHERE (T_Tab.uw_Id = $1::int8)", PQerrorMessage(conn));
                }
               
               uw_end_region(ctx);
               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
               n = PQntuples(res);
               for (i = 0; i < n; ++i) {
               struct __uws_2 __uwr_r_2;
                struct __uws_2* __uwr_acc_3 =
                acc;
                
                __uwr_r_2.__uwf_Tab.__uwf_Text =
                 (PQgetisnull(res, i, 0) ? ({uw_Basis_string
                                            tmp;
                                            uw_error(ctx, FATAL, "demo/crud.ur:127:17-127:20: Unexpectedly NULL field #0");
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
           
           disc == NULL ?
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), (uw_write(ctx, ">Not found!</body>"), 0))))
             :
            disc != NULL && 1 ?
             ({struct __uws_2 __uwr_fs_2 = (*disc);
                ((uw_write(ctx, "<body"), 0),
                 (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                        uw_Basis_get_settings(ctx,
                                                         0))), 0),
                  uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                             "")), 0),
                                       uw_end_region(ctx), ((uw_write(ctx, "><form method=\"post\" action=\"/Crud3/save/"), 0),
                                                            (uw_begin_region(ctx),
                                                              uw_Basis_urlifyInt_w(ctx,
                                                               __uwr_x0_0),
                                                             uw_end_region(ctx),
                                                              ((uw_write(ctx, "\">\n<p> Text: <input type=\"hidden\" name=\".b\" value=\"Text\" />\n<input value=\""), 0),
                                                               (uw_begin_region(ctx),
                                                                 uw_Basis_attrifyString_w(ctx,
                                                                  __uwr_fs_2.__uwf_Tab.__uwf_Text
                                                                  ),
                                                                uw_end_region(ctx),
                                                                 (uw_write(ctx, 
                                                                  "\" type=\"text\" name=\"A\" />\n<input type=\"text\" name=\"B\" />\n<input type=\"hidden\" name=\".e\" value=\"1\" />\n</p>\n<input type=\"submit\" />\n</form></body>"), 0))))))));
              })
              :
             ({
              uw_unit
              tmp;
              uw_error(ctx, FATAL, "demo/crud.ur:48:4-173:2: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_unit
   __uwn_list_5(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "\n<table border=\"1\">\n<tr>\n<th>ID</th>\n<th>Text</th>\n</tr>\n"), 0),
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_unit acc =
                                   0;
                                   int dummy = (uw_begin_region(ctx), 0);
                                   uw_ensure_transaction(ctx);
                                   
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = {  };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
                                      
                                      
                                     PGresult *res = PQexecPrepared(conn, "uw1", 0, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                       uw_error(ctx, FATAL, "demo/crud.ur:67:18-67:22: Query failed:\n%s\n%s", 
                                       "SELECT T_T.uw_Id, T_T.uw_Text FROM uw_Crud3_t AS T_T", PQerrorMessage(conn));
                                       }
                                      
                                      if (PQnfields(res) != 2) {
                                      int nf = PQnfields(res);
                                       PQclear(res);
                                       uw_error(ctx, FATAL, "demo/crud.ur:67:18-67:22: Query returned %d columns instead of 2:\n%s\n%s", nf, 
                                       "SELECT T_T.uw_Id, T_T.uw_Text FROM uw_Crud3_t AS T_T", PQerrorMessage(conn));
                                       }
                                      
                                      uw_end_region(ctx);
                                      uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                      n = PQntuples(res);
                                      for (i = 0; i < n; ++i) {
                                      struct __uws_4 __uwr_r_2;
                                       uw_unit __uwr_acc_3 =
                                       acc;
                                       
                                       __uwr_r_2.__uwf_T.__uwf_Id =
                                        (PQgetisnull(res, i, 0) ? ({uw_Basis_int
                                                                   tmp;
                                                                   uw_error(ctx, FATAL, "demo/crud.ur:67:18-67:22: Unexpectedly NULL field #0");
                                                                   tmp;
                                                                   }) : uw_Basis_stringToInt_error(ctx, 
                                                                         PQgetvalue(res, i, 0)));
                                        
                                        __uwr_r_2.__uwf_T.__uwf_Text =
                                         (PQgetisnull(res, i, 1) ? ({uw_Basis_string
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "demo/crud.ur:67:18-67:22: Unexpectedly NULL field #1");
                                                                    tmp;
                                                                    }) : PQgetvalue(res, i, 1));
                                         
                                       
                                       acc =
                                       ((uw_write(ctx, "\n<tr>\n<td>"), 0),
                                        (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                __uwr_r_2.__uwf_T.__uwf_Id
                                                                ),
                                         uw_end_region(ctx), ((uw_write(ctx, "</td>\n<td>"), 0),
                                                              (uw_begin_region(ctx),
                                                                uw_Basis_htmlifyString_w(ctx,
                                                                 __uwr_r_2.__uwf_T.__uwf_Text
                                                                 ),
                                                               uw_end_region(ctx),
                                                                ((uw_write(ctx, 
                                                                  "</td>\n<td>\n<a href=\"/Crud3/upd/"), 0),
                                                                 (uw_begin_region(ctx),
                                                                   uw_Basis_urlifyInt_w(ctx,
                                                                    __uwr_r_2.__uwf_T.__uwf_Id
                                                                    ),
                                                                  uw_end_region(ctx),
                                                                   ((uw_write(ctx, 
                                                                     "\">[Update]</a>\n<a href=\"/Crud3/confirm/"), 0),
                                                                    (uw_begin_region(ctx),
                                                                      uw_Basis_urlifyInt_w(ctx,
                                                                       __uwr_r_2.__uwf_T.__uwf_Id
                                                                       ),
                                                                     uw_end_region(ctx),
                                                                      (uw_write(ctx, 
                                                                       "\">[Delete]</a>\n</td>\n</tr>\n"), 0)))))))));
                                       }
                                      
                                      uw_pop_cleanup(ctx);
                                      
                                   uw_end_region(ctx);
                                    acc;
                                   })),
            uw_end_region(ctx), (uw_write(ctx, "\n</table>\n<br /><hr /><br />\n<form method=\"post\" action=\"/Crud3/create\">\n<p> Text: <input type=\"hidden\" name=\".b\" value=\"Text\" />\n<input type=\"text\" name=\"A\" />\n<input type=\"text\" name=\"B\" />\n<input type=\"hidden\" name=\".e\" value=\"1\" />\n</p>\n<input type=\"submit\" />\n</form>\n"), 0))));
   }
  
  static uw_unit
   __uwn_wrap_$save_6(uw_context ctx, uw_Basis_int __uwr_x1_0, 
                       struct __uws_6 __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           uw_unit __uwr___3 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_string arg1 =
                                    ({
                                     uw_Basis_string arg0 =
                                      __uwr_x0_1.__uwf_Text.__uwf_A;
                                      
                                      uw_Basis_string arg1 =
                                       __uwr_x0_1.__uwf_Text.__uwf_B;
                                       uw_Basis_strcat(ctx, arg0, arg1);
                                     });
                                    uw_Basis_int arg2 = __uwr_x1_0;
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0, 0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
                                      paramValues[0] = arg1;
                                       
                                       paramValues[1] = uw_Basis_attrifyInt(ctx, 
                                                         arg2);
                                        
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw2", 2, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/crud.ur:111:16-125:3: DML failed:\n%s\n%s", 
                                        "UPDATE uw_Crud3_t AS T_T SET uw_Text = $1::text WHERE (T_T.uw_Id = $2::int8)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">\n<p>Saved!</p>\n"), 0),
                                                        (uw_begin_region(ctx), ({
                                                                               uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                               __uwn_list_5(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "\n</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_$delete_7(uw_context ctx, uw_Basis_int __uwr_x1_0, 
                         uw_unit __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           uw_unit __uwr___3 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_int arg1 = __uwr_x1_0;
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                      paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                        arg1);
                                       
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw3", 1, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/crud.ur:147:16-154:3: DML failed:\n%s\n%s", 
                                        "DELETE FROM uw_Crud3_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), ((uw_write(ctx, ">\n<p>The deed is done.</p>\n"), 0),
                                                        (uw_begin_region(ctx), ({
                                                                               uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                               __uwn_list_5(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                         uw_end_region(ctx), (uw_write(ctx, 
                                                                              "\n</body>"), 0))))));
           }));
   }
  
  static uw_unit
   __uwn_wrap_create_8(uw_context ctx, struct __uws_6 __uwr_x0_0, 
                        uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int __uwr_id_2 =
           ({
            uw_Basis_int n;
            uw_ensure_transaction(ctx);
            PGconn *conn = uw_get_db(ctx);
             
             PGresult *res = PQexecPrepared(conn, "uw4", 0, NULL, NULL, NULL, 0);
             
             if (res == NULL) {
                                uw_try_reconnecting_and_restarting(ctx);
                                uw_error(ctx, FATAL, "Can't allocate NEXTVAL result; database server may be down.");
                                }
              
              if (PQresultStatus(res) != PGRES_TUPLES_OK) {
              PQclear(res);
               uw_error(ctx, FATAL, "demo/crud.ur:93:8-108:2: Query failed:\n%s\n%s", 
               "SELECT NEXTVAL('uw_Crud3_seq')", PQerrorMessage(conn));
               }
              
              n = PQntuples(res);
              if (n != 1) {
              PQclear(res);
               uw_error(ctx, FATAL, "demo/crud.ur:93:8-108:2: Wrong number of result rows:\n%s\n%s", 
               "SELECT NEXTVAL('uw_Crud3_seq')", PQerrorMessage(conn));
               }
              
              n = uw_Basis_stringToInt_error(ctx, PQgetvalue(res, 0, 0));
              PQclear(res);
              
            
            n;
            });
           ({
            uw_unit __uwr___3 =
            (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                    uw_Basis_int arg1 = __uwr_id_2;
                                     
                                     uw_Basis_string arg2 =
                                      ({
                                       uw_Basis_string arg0 =
                                        __uwr_x0_0.__uwf_Text.__uwf_A;
                                        
                                        uw_Basis_string arg1 =
                                         __uwr_x0_0.__uwf_Text.__uwf_B;
                                         uw_Basis_strcat(ctx, arg0, arg1);
                                       });
                                     
                                     uw_ensure_transaction(ctx);
                                     
                                     PGconn *conn = uw_get_db(ctx);
                                      static const int paramFormats[] = { 0, 0 };
                                       const int *paramLengths = paramFormats;
                                        const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
                                       paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                         arg1);
                                        paramValues[1] = arg2;
                                         
                                       
                                      PGresult *res;
                                      
                                      res = PQexecPrepared(conn, "uw5", 2, paramValues, paramLengths, paramFormats, 0);
                                      
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
                                         uw_error(ctx, FATAL, "demo/crud.ur:94:8-108:2: DML failed:\n%s\n%s", 
                                         "INSERT INTO uw_Crud3_t (uw_Id, uw_Text) VALUES ($1::int8, $2::text)", PQerrorMessage(conn));
                                        }
                                          
                                          PQclear(res);
                                          
                                    
                                    uw_end_region(ctx);
                                    0;
                                    })));
            uw_end_region(ctx);
             ((uw_write(ctx, "<body"), 0),
              (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                     uw_Basis_get_settings(ctx,
                                                      0))), 0),
               uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                          "")), 0),
                                    uw_end_region(ctx), ((uw_write(ctx, ">\n<p>Inserted with ID "), 0),
                                                         (uw_begin_region(ctx), 
                                                          uw_Basis_htmlifyInt_w(ctx,
                                                           __uwr_id_2),
                                                          uw_end_region(ctx), ((uw_write(ctx, 
                                                                               ".</p>\n"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ({
                                                                               uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                               __uwn_list_5(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\n</body>"), 0))))))));
            });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_9(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<head>\n<title>Crud3</title>\n</head><body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<h1>Crud3</h1>\n"), 0),
                                                      (uw_begin_region(ctx), ({
                                                                              uw_unit
                                                                               arg0
                                                                               =
                                                                               0;
                                                                               
                                                                               uw_unit
                                                                               arg1
                                                                               =
                                                                               0;
                                                                              __uwn_list_5(ctx,
                                                                              arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                              }),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\n</body>"), 0)))))));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'A':
   return 0;
   case 'B':
    return 1;
    case 'T':
     return 0;
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
 
 
 
 if (!strncmp(request, "/Crud3/main", 11) && (request[11] == 0 || request[11] == '/')) {
  request += 11;
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
    __uwn_wrap_main_9(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Crud3/upd", 10) && (request[10] == 0 || request[10] == '/')) {
   request += 10;
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     __uwn_wrap_upd_4(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Crud3/confirm", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     __uwn_wrap_confirm_3(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Crud3/create", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
    struct __uws_5 uw_input_Text;
     
     uw_enter_subform(ctx, 0);
      uw_input_Text =
      ({struct __uws_5
         result;
         uw_Basis_string uw_input_A;
          uw_Basis_string uw_input_B;
           
         request = uw_get_input(ctx, 0);
          if (request == NULL)
          uw_error(ctx, FATAL, "Missing input A");
          uw_input_A = uw_Basis_unurlifyString_fromClient(ctx, &request);
          result.__uwf_A = uw_input_A;
          
          request = uw_get_input(ctx, 1);
           if (request == NULL)
           uw_error(ctx, FATAL, "Missing input B");
           uw_input_B = uw_Basis_unurlifyString_fromClient(ctx, &request);
           result.__uwf_B = uw_input_B;
           
         result;
         });
      uw_leave_subform(ctx);struct __uws_6 uw_inputs = {
                             uw_input_Text,
                              };
     __uwn_wrap_create_8(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Crud3/save", 11) && (request[11] == 0 || request[11] == '/')) {
   request += 11;
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     struct __uws_5 uw_input_Text;
      
      uw_enter_subform(ctx, 0);
       uw_input_Text =
       ({struct __uws_5
          result;
          uw_Basis_string uw_input_A;
           uw_Basis_string uw_input_B;
            
          request = uw_get_input(ctx, 0);
           if (request == NULL)
           uw_error(ctx, FATAL, "Missing input A");
           uw_input_A = uw_Basis_unurlifyString_fromClient(ctx, &request);
           result.__uwf_A = uw_input_A;
           
           request = uw_get_input(ctx, 1);
            if (request == NULL)
            uw_error(ctx, FATAL, "Missing input B");
            uw_input_B = uw_Basis_unurlifyString_fromClient(ctx, &request);
            result.__uwf_B = uw_input_B;
            
          result;
          });
       uw_leave_subform(ctx);struct __uws_6 uw_inputs = {
                              uw_input_Text,
                               };
      __uwn_wrap_$save_6(ctx, arg0, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Crud3/delete", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     
      uw_unit uw_inputs;
      __uwn_wrap_$delete_7(ctx, arg0, uw_inputs, 0);
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
 