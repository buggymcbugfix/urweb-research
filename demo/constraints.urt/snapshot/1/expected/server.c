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
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_constraints_t'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_constraints_t'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_constraints_t' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_constraints_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_nam' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_parent' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'YES'))");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_constraints_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_nam' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_parent' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'YES'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "3")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_constraints_t' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_constraints_t' AND LOWER(column_name) LIKE 'uw_%'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_constraints_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "3")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_constraints_t' has extra columns.");
      }
     
     PQclear(res);
     }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "SELECT T_T.uw_Id, T_T.uw_Nam, T_T.uw_Parent FROM uw_Constraints_t AS T_T", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Id, T_T.uw_Nam, T_T.uw_Parent FROM uw_Constraints_t AS T_T\n%s", msg);
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
  
 
  
  
  struct __uws_1
   {
   uw_Basis_int __uwf_Id;
    uw_Basis_string __uwf_Nam;
     uw_Basis_int* __uwf_Parent;
      };
  struct __uws_2 {
   struct __uws_1 __uwf_T;
    };
  
  struct __uws_3
   {
   uw_Basis_string __uwf_Id;
    uw_Basis_string __uwf_Nam;
     uw_Basis_string __uwf_Parent;
      };
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  
  /* SQL table uw_Constraints_t keys uw_Id constraints
   Nam : UNIQUE (uw_Nam), Id : CHECK (uw_Id >= 0::int8), 
    Parent : FOREIGN KEY (uw_Parent) REFERENCES uw_Constraints_t (uw_Id)  */
   
  
  static uw_unit
   __uwn_main_6(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<table>\n<tr> <th>Id</th> <th>Name</th> <th>Parent</th> </tr>\n"), 0),
                                                      (uw_begin_region(ctx), (uw_begin_region(ctx), ({
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
                                                                               uw_error(ctx, FATAL, "demo/constraints.ur:18:29-18:33: Query failed:\n%s\n%s", 
                                                                               "SELECT T_T.uw_Id, T_T.uw_Nam, T_T.uw_Parent FROM uw_Constraints_t AS T_T", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               if (PQnfields(res) != 3) {
                                                                               int nf = PQnfields(res);
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "demo/constraints.ur:18:29-18:33: Query returned %d columns instead of 3:\n%s\n%s", nf, 
                                                                               "SELECT T_T.uw_Id, T_T.uw_Nam, T_T.uw_Parent FROM uw_Constraints_t AS T_T", PQerrorMessage(conn));
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
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               =
                                                                               (PQgetisnull(res, i, 0) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/constraints.ur:18:29-18:33: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 0)));
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Nam
                                                                               =
                                                                               (PQgetisnull(res, i, 1) ? 
                                                                               ({uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/constraints.ur:18:29-18:33: Unexpectedly NULL field #1");
                                                                               tmp;
                                                                               }) : 
                                                                               PQgetvalue(res, i, 1));
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Parent
                                                                               =
                                                                               (PQgetisnull(res, i, 2) ? NULL : 
                                                                               ({
                                                                               uw_Basis_int
                                                                               *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
                                                                               *tmp = 
                                                                               (PQgetisnull(res, i, 2) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/constraints.ur:18:29-18:33: Unexpectedly NULL field #2");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 2)));
                                                                               tmp;
                                                                               }));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               ((uw_write(ctx, 
                                                                               "<tr>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_Nam
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ({
                                                                               uw_Basis_int*
                                                                               disc
                                                                               =
                                                                               __uwr_r_2.__uwf_T.__uwf_Parent;
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               (uw_write(ctx, 
                                                                               "NULL"), 0)
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_int
                                                                               __uwr_id_4
                                                                               =
                                                                               (*disc);
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_id_4
                                                                               );
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_unit
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/constraints.ur:12:18-12:22: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               }),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</td>\n</tr>"), 0)))))));
                                                                               }
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                              uw_end_region(ctx);
                                                                               acc;
                                                                              })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\n</table>\n<form method=\"post\" action=\"/Constraints/add\">\n<table>\n<tr> <th>Id:</th> <td><input type=\"text\" name=\"Id\" /></td> </tr>\n<tr> <th>Name:</th> <td><input type=\"text\" name=\"Nam\" /></td> </tr>\n<tr> <th>Parent:</th> <td><input type=\"text\" name=\"Parent\" /></td> </tr>\n<tr> <th></th> <td><input type=\"submit\" /></td> </tr>\n</table>\n</form>\n</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_add_4(uw_context ctx, struct __uws_3 __uwr_x0_0, 
                     uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   char *dml = ({
                                                uw_Basis_string arg0 =
                                                 "INSERT INTO uw_Constraints_t (uw_Id, uw_Nam, uw_Parent) VALUES (";
                                                 
                                                 uw_Basis_string arg1 =
                                                  uw_Basis_sqlifyInt(ctx,
                                                   uw_Basis_stringToInt_error(ctx,
                                                    __uwr_x0_0.__uwf_Id));
                                                  uw_Basis_string arg2 = ", ";
                                                   
                                                 uw_Basis_string arg3 =
                                                  uw_Basis_sqlifyString(ctx,
                                                   __uwr_x0_0.__uwf_Nam);
                                                  uw_Basis_string arg4 = ", ";
                                                   
                                                 uw_Basis_string arg5 =
                                                  ({
                                                   uw_Basis_int* disc =
                                                   ({
                                                    uw_Basis_string disc =
                                                    __uwr_x0_0.__uwf_Parent;
                                                    
                                                    !strcmp(disc, "") ? NULL
                                                      :
                                                     1 ?
                                                      ({uw_Basis_string
                                                         __uwr_s_2 = disc;
                                                         ({
                                                          uw_Basis_int *tmp =
                                                          uw_malloc(ctx, sizeof(
                                                          uw_Basis_int));
                                                          *tmp =
                                                          uw_Basis_stringToInt_error(ctx,
                                                           __uwr_s_2);
                                                          tmp;
                                                          });
                                                       })
                                                       :
                                                      ({
                                                       uw_Basis_int*
                                                       tmp;
                                                       uw_error(ctx, FATAL, "$/basis.urs:545:20-545:36: pattern match failure");
                                                       tmp;
                                                       });
                                                    });
                                                   
                                                   disc == NULL ? "NULL::int8"
                                                     :
                                                    disc != NULL && 1 ?
                                                     ({uw_Basis_int __uwr_y_2 =
                                                        (*disc);
                                                        uw_Basis_sqlifyInt(ctx,
                                                         __uwr_y_2);
                                                      })
                                                      :
                                                     ({
                                                      uw_Basis_string
                                                      tmp;
                                                      uw_error(ctx, FATAL, "demo/constraints.ur:33:8-37:46: pattern match failure");
                                                      tmp;
                                                      });
                                                   });
                                                  uw_Basis_string arg6 = ")";
                                                   uw_Basis_mstrcat(ctx, arg0, 
                                                                          arg1, 
                                                                          arg2, 
                                                                          arg3, 
                                                                          arg4, 
                                                                          arg5, 
                                                                          arg6,
                                                                               NULL);
                                                });
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     PGresult *res;
                                     res = PQexecParams(conn, dml, 0, NULL, NULL, NULL, NULL, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/constraints.ur:33:4-38:11: DML failed:\n%s\n%s", dml, PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ({
             uw_unit arg0 = 0;
              uw_unit arg1 = 0;
             __uwn_main_6(ctx, arg0, arg1);
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_5(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit arg0 = __uwr_x0_0;
            uw_unit arg1 = 0;
           __uwn_main_6(ctx, arg0, arg1);
           }));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'I':
   return 0;
   case 'N':
    return 1;
    case 'P':
     return 2;
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
 
 
 
 if (!strncmp(request, "/Constraints/main", 17) && (request[17] == 0 || request[17] == '/')) {
  request += 17;
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
    __uwn_wrap_main_5(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Constraints/add", 16) && (request[16] == 0 || request[16] == '/')) {
   request += 16;
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
    uw_Basis_string uw_input_Id;
     uw_Basis_string uw_input_Nam;
      uw_Basis_string uw_input_Parent;
       
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input Id");
      uw_input_Id = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input Nam");
       uw_input_Nam = uw_Basis_unurlifyString_fromClient(ctx, &request);
       request = uw_get_input(ctx, 2);
        if (request == NULL)
        uw_error(ctx, FATAL, "Missing input Parent");
        uw_input_Parent = uw_Basis_unurlifyString_fromClient(ctx, &request);
        struct __uws_3 uw_inputs = {
         uw_input_Id,
          uw_input_Nam,
           uw_input_Parent,
            };
     __uwn_wrap_add_4(ctx, uw_inputs, 0);
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
 uw_app uw_application = {3,
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
 