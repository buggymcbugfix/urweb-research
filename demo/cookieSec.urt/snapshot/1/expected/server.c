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
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_cookiesec_lastvisit'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_cookiesec_lastvisit'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_cookiesec_lastvisit' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_cookiesec_lastvisit' AND ((LOWER(column_name) = 'uw_user' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_when' AND data_type = 'timestamp without time zone' AND is_nullable = 'NO'))");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_cookiesec_lastvisit' AND ((LOWER(column_name) = 'uw_user' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_when' AND data_type = 'timestamp without time zone' AND is_nullable = 'NO'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_cookiesec_lastvisit' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_cookiesec_lastvisit' AND LOWER(column_name) LIKE 'uw_%'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_cookiesec_lastvisit' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "2")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_cookiesec_lastvisit' has extra columns.");
      }
     
     PQclear(res);
     }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "SELECT T_LastVisit.uw_User, T_LastVisit.uw_When FROM uw_CookieSec_lastVisit AS T_LastVisit", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_LastVisit.uw_User, T_LastVisit.uw_When FROM uw_CookieSec_lastVisit AS T_LastVisit\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "DELETE FROM uw_CookieSec_lastVisit AS T_T WHERE (T_T.uw_User = $1::text)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_CookieSec_lastVisit AS T_T WHERE (T_T.uw_User = $1::text)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw2", "INSERT INTO uw_CookieSec_lastVisit (uw_User, uw_When) VALUES ($1::text, CURRENT_TIMESTAMP)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_CookieSec_lastVisit (uw_User, uw_When) VALUES ($1::text, CURRENT_TIMESTAMP)\n%s", msg);
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
   uw_Basis_string __uwf_User;
    uw_Basis_time __uwf_When;
     };
  struct __uws_2 {
   struct __uws_1 __uwf_LastVisit;
    };
  struct __uws_3 {
   uw_Basis_string __uwf_User;
    };
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* cookie CookieSec/username */
  /* SQL table uw_CookieSec_lastVisit keys uw_User constraints   */
   
  
  static uw_unit
   __uwn_main_3(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\nCookie: "), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                                                              ({
                                                                               uw_Basis_string
                                                                               disc
                                                                               =
                                                                               ({
                                                                               uw_Basis_string request = uw_maybe_strdup(ctx, 
                                                                               uw_Basis_get_cookie(ctx,
                                                                               "CookieSec/username"
                                                                               ));
                                                                               
                                                                               (request ? uw_Basis_unurlifyString(ctx, &request) : NULL);
                                                                               });
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               ""
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_string
                                                                               __uwr_x_2
                                                                               =
                                                                               disc;
                                                                               __uwr_x_2;
                                                                               })
                                                                               
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
                                                                             "<br />\n<table>\n<tr><th>User</th> <th>Last Visit</th></tr>\n"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              (uw_begin_region(ctx), ({
                                                                               uw_unit
                                                                               acc
                                                                               =
                                                                               0;
                                                                               int dummy = (uw_begin_region(ctx), 0);
                                                                               uw_ensure_transaction(ctx);
                                                                               
                                                                               
                                                                               PGconn *conn = uw_get_db(ctx);
                                                                               static const int paramFormats[] = {  };
                                                                               const int *paramLengths = paramFormats;
                                                                               const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
                                                                               
                                                                               
                                                                               PGresult *res = 
                                                                               PQexecPrepared(conn, "uw0", 0, paramValues, paramLengths, paramFormats, 0);
                                                                               
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
                                                                               uw_error(ctx, FATAL, "demo/cookieSec.ur:16:16-16:20: Query failed:\n%s\n%s", 
                                                                               "SELECT T_LastVisit.uw_User, T_LastVisit.uw_When FROM uw_CookieSec_lastVisit AS T_LastVisit", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               if (PQnfields(res) != 2) {
                                                                               int nf = PQnfields(res);
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "demo/cookieSec.ur:16:16-16:20: Query returned %d columns instead of 2:\n%s\n%s", nf, 
                                                                               "SELECT T_LastVisit.uw_User, T_LastVisit.uw_When FROM uw_CookieSec_lastVisit AS T_LastVisit", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               uw_end_region(ctx);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                                                               n = PQntuples(res);
                                                                               for (i = 0; i < n; ++i) {
                                                                               struct __uws_2 __uwr_r_2;
                                                                               uw_unit
                                                                               __uwr_acc_3
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_2.__uwf_LastVisit.__uwf_User
                                                                               =
                                                                               (PQgetisnull(res, i, 0) ? 
                                                                               ({uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/cookieSec.ur:16:16-16:20: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               PQgetvalue(res, i, 0));
                                                                               
                                                                               __uwr_r_2.__uwf_LastVisit.__uwf_When
                                                                               =
                                                                               (PQgetisnull(res, i, 1) ? 
                                                                               ({uw_Basis_time
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/cookieSec.ur:16:16-16:20: Unexpectedly NULL field #1");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_unsqlTime(ctx, 
                                                                               PQgetvalue(res, i, 1)));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               ((uw_write(ctx, 
                                                                               "<tr><td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_r_2.__uwf_LastVisit.__uwf_User
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td> <td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyTime_w(ctx,
                                                                               __uwr_r_2.__uwf_LastVisit.__uwf_When
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</td></tr>"), 0)))));
                                                                               }
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               uw_end_region(ctx);
                                                                               acc;
                                                                               })),
                                                                             uw_end_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "\n</table>\n<h2>Set cookie value</h2>\n<form method=\"post\" action=\"/CookieSec/set\"><input type=\"text\" name=\"User\" /> <input type=\"submit\" /></form>\n<h2>Record your visit</h2>\n<form method=\"post\" action=\"/CookieSec/imHere\"><input type=\"hidden\" name=\"Sig\" value=\""), 0),
                                                                               (uw_begin_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               uw_Basis_sigString(ctx,
                                                                               0)), 0),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\" /><input type=\"submit\" /></form>\n</body>"), 0)))))))))));
   }
  
  static uw_unit
   __uwn_wrap_imHere_4(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_string disc =
           ({
            uw_Basis_string request = uw_maybe_strdup(ctx, uw_Basis_get_cookie(ctx,
                                                            "CookieSec/username"
                                                            ));
            
            (request ? uw_Basis_unurlifyString(ctx, &request) : NULL);
            });
           
           disc == NULL ? (uw_write(ctx, "You don't have a cookie set!"), 0)
             :
            disc != NULL && 1 ?
             ({uw_Basis_string __uwr_user_2 = disc;
                ({
                 uw_unit __uwr___3 =
                 (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                         uw_Basis_string arg1 = __uwr_user_2;
                                          
                                          uw_ensure_transaction(ctx);
                                          
                                          PGconn *conn = uw_get_db(ctx);
                                           static const int paramFormats[] = { 0 };
                                            const int *paramLengths = paramFormats;
                                             const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                            paramValues[0] = arg1;
                                             
                                            
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
                                              uw_error(ctx, FATAL, "demo/cookieSec.ur:36:8-38:15: DML failed:\n%s\n%s", 
                                              "DELETE FROM uw_CookieSec_lastVisit AS T_T WHERE (T_T.uw_User = $1::text)", PQerrorMessage(conn));
                                             }
                                               
                                               PQclear(res);
                                               
                                         
                                         uw_end_region(ctx);
                                         0;
                                         })));
                 uw_end_region(ctx);
                  ({
                   uw_unit __uwr___4 =
                   (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                           uw_Basis_string arg1 = __uwr_user_2;
                                            
                                            uw_ensure_transaction(ctx);
                                            
                                            PGconn *conn = uw_get_db(ctx);
                                             static const int paramFormats[] = { 0 };
                                              const int *paramLengths = paramFormats;
                                               const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                              paramValues[0] = arg1;
                                               
                                              
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
                                                uw_error(ctx, FATAL, "demo/cookieSec.ur:37:8-38:15: DML failed:\n%s\n%s", 
                                                "INSERT INTO uw_CookieSec_lastVisit (uw_User, uw_When) VALUES ($1::text, CURRENT_TIMESTAMP)", PQerrorMessage(conn));
                                               }
                                                 
                                                 PQclear(res);
                                                 
                                           
                                           uw_end_region(ctx);
                                           0;
                                           })));
                   uw_end_region(ctx);
                    ({
                     uw_unit arg0 = 0;
                      uw_unit arg1 = 0;
                     __uwn_main_3(ctx, arg0, arg1);
                     });
                   });
                 });
              })
              :
             ({
              uw_unit
              tmp;
              uw_error(ctx, FATAL, "demo/cookieSec.ur:6:0-40:0: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_unit
   __uwn_wrap_set_5(uw_context ctx, struct __uws_3 __uwr_x0_0, 
                     uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 = "/";
                                    
                                    uw_Basis_string arg1 =
                                     "CookieSec/username";
                                     
                                    uw_Basis_string arg2 =
                                     uw_Basis_urlifyString(ctx,
                                      __uwr_x0_0.__uwf_User);
                                     uw_Basis_time* arg3 = NULL;
                                      uw_Basis_bool arg4 = uw_Basis_False;
                                       uw_Basis_bool arg5 = uw_Basis_True;
                                        uw_Basis_set_cookie(ctx, arg0, arg1, 
                                                                  arg2, arg3, 
                                                                  arg4, arg5);
                                   }));
           uw_end_region(ctx);
            ({
             uw_unit arg0 = 0;
              uw_unit arg1 = 0;
             __uwn_main_3(ctx, arg0, arg1);
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_6(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit arg0 = __uwr_x0_0;
            uw_unit arg1 = 0;
           __uwn_main_3(ctx, arg0, arg1);
           }));
   }
 
 static int uw_input_num(const char *name) {
 return 0;}
 
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
  uw_sign(uw_unnull(uw_Basis_get_cookie(ctx, "CookieSec/username")), r);
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
 
 if (!strncmp(request, "/CookieSec/main", 15) && (request[15] == 0 || request[15] == '/')) {
  request += 15;
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
    __uwn_wrap_main_6(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/CookieSec/set", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
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
    uw_Basis_string uw_input_User;
     
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input User");
      uw_input_User = uw_Basis_unurlifyString_fromClient(ctx, &request);
      struct __uws_3 uw_inputs = {
       uw_input_User,
        };
     __uwn_wrap_set_5(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/CookieSec/imHere", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
   if (*request == '/') ++request;
   {
    uw_Basis_string sig = uw_get_input(ctx, 0);
    if (sig == NULL) uw_error(ctx, FATAL, "Missing cookie signature");
    if (!uw_streq(sig, uw_cookie_sig(ctx)))
    uw_error(ctx, FATAL, "Wrong cookie signature");
     }
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
    
     uw_unit uw_inputs;
     __uwn_wrap_imHere_4(ctx, uw_inputs, 0);
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
                                                                       
                           uw_served_files};
 