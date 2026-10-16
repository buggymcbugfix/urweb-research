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
   static int uw_db_check(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_validate_db_t'");
     
     if (res == NULL) {
     uw_set_error_message(ctx, "Out of memory allocating query result.");
      return 1;
      }
     
     if (PQresultStatus(res) != PGRES_TUPLES_OK) {
     uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_validate_db_t'", PQerrorMessage(conn));
      PQclear(res);
      return 1;
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      uw_set_error_message(ctx, "%s", "Table 'uw_validate_db_t' does not exist.");
      return 1;
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_t' AND ((LOWER(column_name) = 'uw_n' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))");
      
      if (res == NULL) {
      uw_set_error_message(ctx, "Out of memory allocating query result.");
       return 1;
       }
      
      if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_t' AND ((LOWER(column_name) = 'uw_n' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))", PQerrorMessage(conn));
       PQclear(res);
       return 1;
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "1")) {
      PQclear(res);
       uw_set_error_message(ctx, "%s", "Table 'uw_validate_db_t' has the wrong column types.");
       return 1;
       }
      
      PQclear(res);
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_t' AND LOWER(column_name) LIKE 'uw_%'");
       
       if (res == NULL) {
       uw_set_error_message(ctx, "Out of memory allocating query result.");
        return 1;
        }
       
       if (PQresultStatus(res) != PGRES_TUPLES_OK) {
       uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_t' AND LOWER(column_name) LIKE 'uw_%'", PQerrorMessage(conn));
        PQclear(res);
        return 1;
        }
       
       if (strcmp(PQgetvalue(res, 0, 0), "1")) {
       PQclear(res);
        uw_set_error_message(ctx, "%s", "Table 'uw_validate_db_t' has extra columns.");
        return 1;
        }
       
       PQclear(res);
       res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.views WHERE table_name = 'uw_validate_db_v'");
        
        if (res == NULL) {
        uw_set_error_message(ctx, "Out of memory allocating query result.");
         return 1;
         }
        
        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.views WHERE table_name = 'uw_validate_db_v'", PQerrorMessage(conn));
         PQclear(res);
         return 1;
         }
        
        if (strcmp(PQgetvalue(res, 0, 0), "1")) {
        PQclear(res);
         uw_set_error_message(ctx, "%s", "Table 'uw_validate_db_v' does not exist.");
         return 1;
         }
        
        PQclear(res);
        res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_v' AND ((LOWER(column_name) = 'uw_n' AND data_type IN ('bigint', 'numeric', 'integer')))");
         
         if (res == NULL) {
         uw_set_error_message(ctx, "Out of memory allocating query result.");
          return 1;
          }
         
         if (PQresultStatus(res) != PGRES_TUPLES_OK) {
         uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_v' AND ((LOWER(column_name) = 'uw_n' AND data_type IN ('bigint', 'numeric', 'integer')))", PQerrorMessage(conn));
          PQclear(res);
          return 1;
          }
         
         if (strcmp(PQgetvalue(res, 0, 0), "1")) {
         PQclear(res);
          uw_set_error_message(ctx, "%s", "Table 'uw_validate_db_v' has the wrong column types.");
          return 1;
          }
         
         PQclear(res);
         res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_v' AND LOWER(column_name) LIKE 'uw_%'");
          
          if (res == NULL) {
          uw_set_error_message(ctx, "Out of memory allocating query result.");
           return 1;
           }
          
          if (PQresultStatus(res) != PGRES_TUPLES_OK) {
          uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_validate_db_v' AND LOWER(column_name) LIKE 'uw_%'", PQerrorMessage(conn));
           PQclear(res);
           return 1;
           }
          
          if (strcmp(PQgetvalue(res, 0, 0), "1")) {
          PQclear(res);
           uw_set_error_message(ctx, "%s", "Table 'uw_validate_db_v' has extra columns.");
           return 1;
           }
          
          PQclear(res);
          res = PQexec(conn, "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_validate_db_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)");
           
           if (res == NULL) {
           uw_set_error_message(ctx, "Out of memory allocating query result.");
            return 1;
            }
           
           if (PQresultStatus(res) != PGRES_TUPLES_OK) {
           uw_set_error_message(ctx, "Query failed:\n%s\n%s", "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_validate_db_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)", PQerrorMessage(conn));
            PQclear(res);
            return 1;
            }
           
           if (strcmp(PQgetvalue(res, 0, 0), "1")) {
           PQclear(res);
            uw_set_error_message(ctx, "%s", "Sequence 'uw_Validate_db_s' does not exist.");
            return 1;
            }
           
           PQclear(res);
           
    return 0;
    }
    
    static int uw_db_validate(uw_context ctx) {
     static int validated;
     
     if (validated)
     return 0;
      if (uw_db_check(ctx))
     return 1;
      validated = 1;
     return 0;
     }
     
     static int uw_db_prepare(uw_context ctx) {
     PGconn *conn = uw_get_db(ctx);
     PGresult *res;
     
     
     return 0;
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
    char failure[ERROR_BUF_LEN];
    char *env_db_str = getenv("URWEB_PQ_CON");
    PGconn *conn = PQconnectdb(env_db_str == NULL ? "dbname=validate_db.db" : env_db_str);
    if (conn == NULL) uw_error(ctx, FATAL, "libpq can't allocate a connection.");
    if (PQstatus(conn) != CONNECTION_OK) {
    char msg[1024];
     strncpy(msg, PQerrorMessage(conn), 1024);
     msg[1023] = 0;
     PQfinish(conn);
     uw_error(ctx, BOUNDED_RETRY, "Connection to Postgres server failed: %s", msg);
    }
    uw_set_db(ctx, conn);
    
    if (uw_db_validate(ctx) || uw_db_prepare(ctx)) {
    strcpy(failure, uw_error_message(ctx));
     PQfinish(conn);
     uw_set_db(ctx, NULL);
     uw_error(ctx, FATAL, "%s", failure);
     }
    }
 
 /* No global setup for LRU cache. */
  
 
  
  
  static uw_unit __uwn_initializer_1(uw_context ctx, uw_unit __uwr___0)
   {
   return(0);
   }
  
  static uw_unit __uwn_expunger_2(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return(0);
   }
  
  /* SQL table uw_Validate_db_t constraints   */
   
  /* SQL sequence uw_Validate_db_s */
   
  
  /* SQL view uw_Validate_db_v AS
   SELECT T_T.uw_N AS uw_N FROM uw_Validate_db_t AS T_T  */
   
  
  static uw_unit
   __uwn_wrap_check_3(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                                                              ({
                                                                               uw_Basis_string
                                                                               disc
                                                                               =
                                                                               uw_Basis_validateDb(ctx
                                                                               );
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               "ok"
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_string
                                                                               __uwr_m_2
                                                                               =
                                                                               disc;
                                                                               __uwr_m_2;
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
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
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
 
 if (!strncmp(request, "/Validate_db/check", 18) && (request[18] == 0 || request[18] == '/')) {
  request += 18;
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
    __uwn_wrap_check_3(ctx, arg0, 0);
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
                                                       0};
 