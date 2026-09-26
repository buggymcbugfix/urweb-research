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
    
    res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_chat_t'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_chat_t'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "1")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_chat_t' does not exist.");
      }
     
     PQclear(res);
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_title' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_room' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_title' AND data_type IN ('text', 'character varying') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_room' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "3")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_chat_t' has the wrong column types.");
      }
     
     PQclear(res);
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_t' AND LOWER(column_name) LIKE 'uw_%'");
     
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
      uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
      }
     
     if (strcmp(PQgetvalue(res, 0, 0), "3")) {
     PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Table 'uw_chat_t' has extra columns.");
      }
     
     PQclear(res);
     
     
     res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_chat_room_t'");
      
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
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.tables WHERE table_name = 'uw_chat_room_t'\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "1")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Table 'uw_chat_room_t' does not exist.");
       }
      
      PQclear(res);
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_room_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_client' AND data_type = 'integer' AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_channel' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))");
      
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
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_room_t' AND ((LOWER(column_name) = 'uw_id' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_client' AND data_type = 'integer' AND is_nullable = 'NO') OR (LOWER(column_name) = 'uw_channel' AND data_type IN ('bigint', 'numeric', 'integer') AND is_nullable = 'NO'))\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "3")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Table 'uw_chat_room_t' has the wrong column types.");
       }
      
      PQclear(res);
      
      res = PQexec(conn, "SELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_room_t' AND LOWER(column_name) LIKE 'uw_%'");
      
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
       uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM information_schema.columns WHERE table_name = 'uw_chat_room_t' AND LOWER(column_name) LIKE 'uw_%'\n%s", msg);
       }
      
      if (strcmp(PQgetvalue(res, 0, 0), "3")) {
      PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Table 'uw_chat_room_t' has extra columns.");
       }
      
      PQclear(res);
      res = PQexec(conn, "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_chat_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)");
       
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
        uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM pg_class WHERE relname = 'uw_chat_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)\n%s", msg);
        }
       
       if (strcmp(PQgetvalue(res, 0, 0), "1")) {
       PQclear(res);
        PQfinish(conn);
        uw_error(ctx, FATAL, "Sequence 'uw_Chat_s' does not exist.");
        }
       
       PQclear(res);
       
       
       res = PQexec(conn, "SELECT COUNT(*) FROM pg_class WHERE relname = 'uw_chat_room_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)");
        
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
         uw_error(ctx, FATAL, "Query failed:\nSELECT COUNT(*) FROM pg_class WHERE relname = 'uw_chat_room_s' AND relkind = 'S' AND pg_catalog.pg_table_is_visible(oid)\n%s", msg);
         }
        
        if (strcmp(PQgetvalue(res, 0, 0), "1")) {
        PQclear(res);
         PQfinish(conn);
         uw_error(ctx, FATAL, "Sequence 'uw_Chat_Room_s' does not exist.");
         }
        
        PQclear(res);
        }static void uw_db_prepare(uw_context ctx) {
    PGconn *conn = uw_get_db(ctx);
    PGresult *res;
    
    res = PQprepare(conn, "uw0", "DELETE FROM uw_Chat_Room_t", 0, NULL);
     if (PQresultStatus(res) != PGRES_COMMAND_OK) {
     char msg[1024];
      strncpy(msg, PQerrorMessage(conn), 1024);
      msg[1023] = 0;
      PQclear(res);
      PQfinish(conn);
      uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Chat_Room_t\n%s", msg);
      }
     PQclear(res);
     
     
     res = PQprepare(conn, "uw1", "DELETE FROM uw_Chat_Room_t WHERE (((uw_Channel >> 32) = $1::int4) OR ((uw_Client) = $2::int4))", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Chat_Room_t WHERE (((uw_Channel >> 32) = $1::int4) OR ((uw_Client) = $2::int4))\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw2", "SELECT T_T.uw_Room FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Room FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw3", "SELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw4", "SELECT T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw5", "SELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE ((T_T.uw_Id = $1::int8) AND (T_T.uw_Client = $2::int4))", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE ((T_T.uw_Id = $1::int8) AND (T_T.uw_Client = $2::int4))\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw6", "INSERT INTO uw_Chat_Room_t (uw_Channel, uw_Client, uw_Id) VALUES ($1::int8, $2::int4, $3::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Chat_Room_t (uw_Channel, uw_Client, uw_Id) VALUES ($1::int8, $2::int4, $3::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw7", "SELECT COUNT(*) AS uw_N FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT COUNT(*) AS uw_N FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw8", "SELECT T_T.uw_Id, T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nSELECT T_T.uw_Id, T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw9", "INSERT INTO uw_Chat_t (uw_Id, uw_Room, uw_Title) VALUES ($1::int8, $2::int8, $3::text)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nINSERT INTO uw_Chat_t (uw_Id, uw_Room, uw_Title) VALUES ($1::int8, $2::int8, $3::text)\n%s", msg);
       }
      PQclear(res);
      
     
     res = PQprepare(conn, "uw10", "DELETE FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", 0, NULL);
      if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      char msg[1024];
       strncpy(msg, PQerrorMessage(conn), 1024);
       msg[1023] = 0;
       PQclear(res);
       PQfinish(conn);
       uw_error(ctx, FATAL, "Unable to create prepared statement:\nDELETE FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)\n%s", msg);
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
   uw_Basis_int __uwf_Room;
    };
  struct __uws_2 {
   struct __uws_1 __uwf_T;
    };
  struct __uws_3 {
   uw_Basis_channel __uwf_Channel;
    };
  struct __uws_4 {
   struct __uws_3 __uwf_T;
    };
  struct __uws_5 {
   uw_Basis_int __uwf_Room;
    uw_Basis_string __uwf_Title;
     };
  struct __uws_6 {
   struct __uws_5 __uwf_T;
    };
  struct __uws_7 {
   uw_Basis_source __uwf_Head;
    uw_Basis_source __uwf_Tail;
     };
  
  struct __uws_8
   {
   uw_Basis_int __uwf_Id;
    uw_Basis_int __uwf_Room;
     uw_Basis_string __uwf_Title;
      };
  struct __uws_9 {
   struct __uws_8 __uwf_T;
    };
  struct __uws_10 {
   uw_Basis_int __uwf_N;
    };
  struct __uws_11 {
   uw_Basis_string __uwf_Title;
    };
  
  static char jslib[] = "*runtime elided*";
   static char jsapp[] = "*script elided*";
  
  static uw_unit __uwn_initializer_1758(uw_context ctx, uw_unit __uwr___0)
   {
   return(((uw_begin_region(ctx), ({
            
             
             uw_ensure_transaction(ctx);
             
             PGconn *conn = uw_get_db(ctx);
              static const int paramFormats[] = {  };
               const int *paramLengths = paramFormats;
                const char **paramValues = uw_malloc(ctx, 0 * sizeof(char*));
               
               
              PGresult *res;
              
              res = PQexecPrepared(conn, "uw0", 0, paramValues, paramLengths, paramFormats, 0);
              
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
                 uw_error(ctx, FATAL, ":0:0-0:0: DML failed:\n%s\n%s", "DELETE FROM uw_Chat_Room_t", PQerrorMessage(conn));
                }
                  
                  PQclear(res);
                  
            
            uw_end_region(ctx);
            0;
            })), 0));
   }
  
  static uw_unit
   __uwn_expunger_1757(uw_context ctx, uw_Basis_client __uwr_cli_0)
   {
   return((uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                  uw_Basis_client arg1 = __uwr_cli_0;
                                   uw_Basis_client arg2 = __uwr_cli_0;
                                   
                                   uw_ensure_transaction(ctx);
                                   
                                   PGconn *conn = uw_get_db(ctx);
                                    static const int paramFormats[] = { 0, 0 };
                                     const int *paramLengths = paramFormats;
                                      const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
                                     paramValues[0] = uw_Basis_attrifyClient(ctx, 
                                                       arg1);
                                      
                                      paramValues[1] = uw_Basis_attrifyClient(ctx, 
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
                                       uw_error(ctx, FATAL, ":0:0-0:0: DML failed:\n%s\n%s", 
                                       "DELETE FROM uw_Chat_Room_t WHERE (((uw_Channel >> 32) = $1::int4) OR ((uw_Client) = $2::int4))", PQerrorMessage(conn));
                                      }
                                        
                                        PQclear(res);
                                        
                                  
                                  uw_end_region(ctx);
                                  0;
                                  })), uw_end_region(ctx), 0));
   }
  
  /* SQL sequence uw_Chat_Room_s */
   
  /* SQL table uw_Chat_Room_t keys uw_Client, uw_Id constraints   */
   
  /* SQL sequence uw_Chat_s */
   
  /* SQL table uw_Chat_t keys uw_Id constraints   */
   
  
  static uw_unit
   __uwn_$speak_1745(uw_context ctx, uw_Basis_int __uwr_id_0, 
                      uw_Basis_string __uwr_line_1, uw_unit __uwr___2)
   {
   return(({
           struct __uws_2 __uwr_r_3 =
           (uw_begin_region(ctx), ({
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
                                       paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                         arg1);
                                        
                                       
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
                                        uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Query failed:\n%s\n%s", 
                                        "SELECT T_T.uw_Room FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                        }
                                       
                                       if (PQnfields(res) != 1) {
                                       int nf = PQnfields(res);
                                        PQclear(res);
                                        uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                        "SELECT T_T.uw_Room FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                        }
                                       
                                       uw_end_region(ctx);
                                       uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                       n = PQntuples(res);
                                       for (i = 0; i < n; ++i) {
                                       struct __uws_2 __uwr_r_3;
                                        struct __uws_2* __uwr_acc_4 =
                                        acc;
                                        
                                        __uwr_r_3.__uwf_T.__uwf_Room =
                                         (PQgetisnull(res, i, 0) ? ({uw_Basis_int
                                                                    tmp;
                                                                    uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Unexpectedly NULL field #0");
                                                                    tmp;
                                                                    }) : uw_Basis_stringToInt_error(ctx, 
                                                                          PQgetvalue(res, i, 0)));
                                         
                                        
                                        acc =
                                        ({
                                         struct __uws_2 *tmp =
                                         uw_malloc(ctx, sizeof(struct __uws_2));
                                         *tmp = __uwr_r_3;
                                         tmp;
                                         });
                                        }
                                       
                                       uw_pop_cleanup(ctx);
                                       
                                    acc;
                                    });
                                   
                                   disc == NULL ?
                                    ({
                                     struct __uws_2
                                     tmp;
                                     uw_error(ctx, FATAL, "$/top.ur:399:24-400:3: %s", 
                                     "Query returned no rows");
                                     tmp;
                                     })
                                     :
                                    disc != NULL && 1 ?
                                     ({struct __uws_2 __uwr_r_3 = (*disc);
                                        __uwr_r_3;
                                      })
                                      :
                                     ({
                                      struct __uws_2
                                      tmp;
                                      uw_error(ctx, FATAL, "demo/chat.ur:28:12-29:27: pattern match failure");
                                      tmp;
                                      });
                                   }));
           uw_end_region(ctx);
            (uw_begin_region(ctx), ({
             uw_unit acc =
             0;
             int dummy = (uw_begin_region(ctx), 0);
             uw_ensure_transaction(ctx);
             uw_Basis_int arg1 = __uwr_r_3.__uwf_T.__uwf_Room;
              
              PGconn *conn = uw_get_db(ctx);
               static const int paramFormats[] = { 0 };
                const int *paramLengths = paramFormats;
                 const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                paramValues[0] = uw_Basis_attrifyInt(ctx, arg1);
                 
                
               PGresult *res = PQexecPrepared(conn, "uw3", 1, paramValues, paramLengths, paramFormats, 0);
               
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
                 uw_error(ctx, FATAL, "demo/chat.ur:32:12-33:31: Query failed:\n%s\n%s", 
                 "SELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                 }
                
                if (PQnfields(res) != 1) {
                int nf = PQnfields(res);
                 PQclear(res);
                 uw_error(ctx, FATAL, "demo/chat.ur:32:12-33:31: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                 "SELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                 }
                
                uw_end_region(ctx);
                uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                n = PQntuples(res);
                for (i = 0; i < n; ++i) {
                struct __uws_4 __uwr_r_4;
                 uw_unit __uwr_acc_5 =
                 acc;
                 
                 __uwr_r_4.__uwf_T.__uwf_Channel =
                  (PQgetisnull(res, i, 0) ? ({uw_Basis_channel
                                             tmp;
                                             uw_error(ctx, FATAL, "demo/chat.ur:32:12-33:31: Unexpectedly NULL field #0");
                                             tmp;
                                             }) : uw_Basis_stringToChannel_error(ctx, 
                                                   PQgetvalue(res, i, 0)));
                  
                 
                 acc =
                 ({
                  uw_Basis_channel arg0 = __uwr_r_4.__uwf_T.__uwf_Channel;
                   
                   uw_Basis_string arg1 =
                    uw_Basis_urlifyString(ctx, __uwr_line_1);
                    uw_Basis_send(ctx, arg0, arg1);
                  });
                 }
                
                uw_pop_cleanup(ctx);
                
             uw_end_region(ctx);
              acc;
             }));
           }));
   }
  
  static uw_unit
   __uwn_wrap_chat_1754(uw_context ctx, uw_Basis_int __uwr_x1_0, 
                         uw_unit __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           struct __uws_6 __uwr_r_3 =
           ({
            struct __uws_6* disc =
            ({
             struct __uws_6* acc =
             NULL;
             int dummy = (uw_begin_region(ctx), 0);
             uw_ensure_transaction(ctx);
             uw_Basis_int arg1 = __uwr_x1_0;
              
              PGconn *conn = uw_get_db(ctx);
               static const int paramFormats[] = { 0 };
                const int *paramLengths = paramFormats;
                 const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                paramValues[0] = uw_Basis_attrifyInt(ctx, arg1);
                 
                
               PGresult *res = PQexecPrepared(conn, "uw4", 1, paramValues, paramLengths, paramFormats, 0);
               
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
                 uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Query failed:\n%s\n%s", 
                 "SELECT T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                 }
                
                if (PQnfields(res) != 2) {
                int nf = PQnfields(res);
                 PQclear(res);
                 uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Query returned %d columns instead of 2:\n%s\n%s", nf, 
                 "SELECT T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                 }
                
                uw_end_region(ctx);
                uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                n = PQntuples(res);
                for (i = 0; i < n; ++i) {
                struct __uws_6 __uwr_r_3;
                 struct __uws_6* __uwr_acc_4 =
                 acc;
                 
                 __uwr_r_3.__uwf_T.__uwf_Room =
                  (PQgetisnull(res, i, 0) ? ({uw_Basis_int
                                             tmp;
                                             uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Unexpectedly NULL field #0");
                                             tmp;
                                             }) : uw_Basis_stringToInt_error(ctx, 
                                                   PQgetvalue(res, i, 0)));
                  
                  __uwr_r_3.__uwf_T.__uwf_Title =
                   (PQgetisnull(res, i, 1) ? ({uw_Basis_string
                                              tmp;
                                              uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Unexpectedly NULL field #1");
                                              tmp;
                                              }) : uw_strdup(ctx, PQgetvalue(res, i, 1)));
                   
                 
                 acc =
                 ({
                  struct __uws_6 *tmp =
                  uw_malloc(ctx, sizeof(struct __uws_6));
                  *tmp = __uwr_r_3;
                  tmp;
                  });
                 }
                
                uw_pop_cleanup(ctx);
                
             acc;
             });
            
            disc == NULL ?
             ({
              struct __uws_6
              tmp;
              uw_error(ctx, FATAL, "$/top.ur:399:24-400:3: %s", "Query returned no rows");
              tmp;
              })
              :
             disc != NULL && 1 ?
              ({struct __uws_6 __uwr_r_3 = (*disc);
                 __uwr_r_3;
               })
               :
              ({
               struct __uws_6
               tmp;
               uw_error(ctx, FATAL, "demo/chat.ur:10:4-50:7: pattern match failure");
               tmp;
               });
            });
           ({
            uw_Basis_client __uwr_cli_4 =
            (uw_begin_region(ctx), uw_Basis_self(ctx));
            uw_end_region(ctx);
             ({
              uw_Basis_channel __uwr_ch_5 =
              (uw_begin_region(ctx), ({
                                      struct __uws_4* disc =
                                      ({
                                       struct __uws_4* acc =
                                       NULL;
                                       int dummy = (uw_begin_region(ctx), 0);
                                       uw_ensure_transaction(ctx);
                                       uw_Basis_int arg1 =
                                        __uwr_r_3.__uwf_T.__uwf_Room;
                                        uw_Basis_client arg2 = __uwr_cli_4;
                                        
                                        PGconn *conn = uw_get_db(ctx);
                                         static const int paramFormats[] = { 0, 
                                                                              0 };
                                          const int *paramLengths = paramFormats;
                                           const char **paramValues = uw_malloc(ctx, 2 * sizeof(char*));
                                          paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                            arg1);
                                           
                                           paramValues[1] = uw_Basis_attrifyClient(ctx, 
                                                             arg2);
                                            
                                          
                                         PGresult *res = PQexecPrepared(conn, "uw5", 2, paramValues, paramLengths, paramFormats, 0);
                                         
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
                                           uw_error(ctx, FATAL, "demo/broadcast.ur:15:13-15:15: Query failed:\n%s\n%s", 
                                           "SELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE ((T_T.uw_Id = $1::int8) AND (T_T.uw_Client = $2::int4))", PQerrorMessage(conn));
                                           }
                                          
                                          if (PQnfields(res) != 1) {
                                          int nf = PQnfields(res);
                                           PQclear(res);
                                           uw_error(ctx, FATAL, "demo/broadcast.ur:15:13-15:15: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                           "SELECT T_T.uw_Channel FROM uw_Chat_Room_t AS T_T WHERE ((T_T.uw_Id = $1::int8) AND (T_T.uw_Client = $2::int4))", PQerrorMessage(conn));
                                           }
                                          
                                          uw_end_region(ctx);
                                          uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                          n = PQntuples(res);
                                          for (i = 0; i < n; ++i) {
                                          struct __uws_4 __uwr_r_5;
                                           struct __uws_4* __uwr_acc_6 =
                                           acc;
                                           
                                           __uwr_r_5.__uwf_T.__uwf_Channel =
                                            (PQgetisnull(res, i, 0) ? ({uw_Basis_channel
                                                                       tmp;
                                                                       uw_error(ctx, FATAL, "demo/broadcast.ur:15:13-15:15: Unexpectedly NULL field #0");
                                                                       tmp;
                                                                       }) : uw_Basis_stringToChannel_error(ctx, 
                                                                             PQgetvalue(res, i, 0)));
                                            
                                           
                                           acc =
                                           ({
                                            struct __uws_4 *tmp =
                                            uw_malloc(ctx, sizeof(struct
                                                                   __uws_4));
                                            *tmp = __uwr_r_5;
                                            tmp;
                                            });
                                           }
                                          
                                          uw_pop_cleanup(ctx);
                                          
                                       acc;
                                       });
                                      
                                      disc == NULL ?
                                       ({
                                        uw_Basis_channel __uwr_ch_5 =
                                        (uw_begin_region(ctx), uw_Basis_new_channel(ctx,
                                                                0));
                                        uw_end_region(ctx);
                                         ({
                                          uw_unit __uwr___6 =
                                          (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                                                  uw_Basis_channel
                                                                   arg1 =
                                                                   __uwr_ch_5;
                                                                   
                                                                   uw_Basis_client
                                                                    arg2 =
                                                                    __uwr_cli_4;
                                                                   
                                                                   uw_Basis_int
                                                                    arg3 =
                                                                    __uwr_r_3.__uwf_T.__uwf_Room;
                                                                   
                                                                   uw_ensure_transaction(ctx);
                                                                   
                                                                   PGconn *conn = uw_get_db(ctx);
                                                                    static const int paramFormats[] = { 
                                                                     0, 0, 0 };
                                                                     const int *paramLengths = paramFormats;
                                                                      const char **paramValues = uw_malloc(ctx, 3 * sizeof(char*));
                                                                     paramValues[0] = 
                                                                      uw_Basis_attrifyChannel(ctx, 
                                                                       arg1);
                                                                      
                                                                      paramValues[1] = 
                                                                       uw_Basis_attrifyClient(ctx, 
                                                                        arg2);
                                                                       
                                                                      paramValues[2] = 
                                                                       uw_Basis_attrifyInt(ctx, 
                                                                        arg3);
                                                                       
                                                                     
                                                                    PGresult *res;
                                                                    
                                                                    res = PQexecPrepared(conn, "uw6", 3, paramValues, paramLengths, paramFormats, 0);
                                                                    
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
                                                                       uw_error(ctx, FATAL, "demo/broadcast.ur:18:12-19:21: DML failed:\n%s\n%s", 
                                                                       "INSERT INTO uw_Chat_Room_t (uw_Channel, uw_Client, uw_Id) VALUES ($1::int8, $2::int4, $3::int8)", PQerrorMessage(conn));
                                                                      }
                                                                        
                                                                        PQclear(res);
                                                                        
                                                                  
                                                                  uw_end_region(ctx);
                                                                  0;
                                                                  })));
                                          uw_end_region(ctx);
                                           __uwr_ch_5;
                                          });
                                        })
                                        :
                                       disc != NULL && 1 ?
                                        ({struct __uws_4 __uwr_r_5 = (*disc);
                                           __uwr_r_5.__uwf_T.__uwf_Channel;
                                         })
                                         :
                                        ({
                                         uw_Basis_channel
                                         tmp;
                                         uw_error(ctx, FATAL, "demo/broadcast.ur:13:8-22:4: pattern match failure");
                                         tmp;
                                         });
                                      }));
              uw_end_region(ctx);
               ({
                uw_Basis_source __uwr_newLine_6 =
                uw_Basis_new_client_source(ctx,
                 ({
                  uw_Basis_string arg0 = "{c:\"c\",v:";
                   uw_Basis_string arg1 = uw_Basis_jsifyString(ctx, "");
                    uw_Basis_string arg2 = "}";
                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                  }));
                ({
                 uw_Basis_source __uwr_head_7 =
                 uw_Basis_new_client_source(ctx, "{c:\"c\",v:null}");
                 ({
                  struct __uws_7 __uwr_buf_8 =
                  ({ struct __uws_7 tmp =
                   {__uwr_head_7, 
                     uw_Basis_new_client_source(ctx,
                      ({
                       uw_Basis_string arg0 = "{c:\"c\",v:";
                        
                        uw_Basis_string arg1 =
                         uw_Basis_htmlifySource(ctx, __uwr_head_7);
                         uw_Basis_string arg2 = "}";
                          uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                       }))}; tmp; });
                  ((uw_write(ctx, "<body"), 0),
                   (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                          ({
                                                           uw_Basis_string arg0
                                                            =
                                                            uw_Basis_get_settings(ctx,
                                                             0);
                                                            
                                                            uw_Basis_string
                                                             arg1 =
                                                             "exec({c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:3},x:{c:\"c\",v:";
                                                             
                                                            uw_Basis_string
                                                             arg2 =
                                                             uw_Basis_jsifyChannel(ctx,
                                                              __uwr_ch_5);
                                                             
                                                            uw_Basis_string
                                                             arg3 =
                                                             "}},x:{c:\"c\",v:{_Head:";
                                                             
                                                            uw_Basis_string
                                                             arg4 =
                                                             uw_Basis_htmlifySource(ctx,
                                                              __uwr_buf_8.__uwf_Head
                                                              );
                                                             
                                                            uw_Basis_string
                                                             arg5 = ",_Tail:";
                                                             
                                                            uw_Basis_string
                                                             arg6 =
                                                             uw_Basis_htmlifySource(ctx,
                                                              __uwr_buf_8.__uwf_Tail
                                                              );
                                                             
                                                            uw_Basis_string
                                                             arg7 =
                                                             "}}},x:{c:\"c\",v:null}},x:{c:\"c\",v:null}})";
                                                             uw_Basis_mstrcat(ctx, 
                                                           arg0, arg1, arg2, 
                                                            arg3, arg4, arg5, 
                                                            arg6, arg7, NULL);
                                                           }))), 0),
                    uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                               ""
                                                                               )), 0),
                                         uw_end_region(ctx), ((uw_write(ctx, ">\n<h1>"), 0),
                                                              (uw_begin_region(ctx),
                                                                uw_Basis_htmlifyString_w(ctx,
                                                                 __uwr_r_3.__uwf_T.__uwf_Title
                                                                 ),
                                                               uw_end_region(ctx),
                                                                ((uw_write(ctx, 
                                                                  "</h1>\n<button onclick='uw_event=event;exec("), 0),
                                                                 (uw_begin_region(ctx),
                                                                   ((uw_write(ctx, 
                                                                     "{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:4},x:{c:\"c\",v:"), 0),
                                                                    (uw_begin_region(ctx),
                                                                      uw_Basis_htmlifyInt_w(ctx,
                                                                       __uwr_x1_0
                                                                       ),
                                                                     uw_end_region(ctx),
                                                                      ((uw_write(ctx, 
                                                                        "}},x:{c:\"c\",v:"), 0),
                                                                       (uw_begin_region(ctx),
                                                                         uw_Basis_htmlifySource_w(ctx,
                                                                          __uwr_newLine_6
                                                                          ),
                                                                        uw_end_region(ctx),
                                                                         (uw_write(ctx, 
                                                                          "}},x:{c:\"c\",v:null}}"), 0))))),
                                                                  uw_end_region(ctx),
                                                                   ((uw_write(ctx, 
                                                                     ")'>Send:</button> <script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                    (uw_begin_region(ctx),
                                                                      ((uw_write(ctx, 
                                                                        "{c:\"c\",v:"), 0),
                                                                       (uw_begin_region(ctx),
                                                                         uw_Basis_htmlifySource_w(ctx,
                                                                          __uwr_newLine_6
                                                                          ),
                                                                        uw_end_region(ctx),
                                                                         (uw_write(ctx, 
                                                                          "}"), 0))),
                                                                     uw_end_region(ctx),
                                                                      ((uw_write(ctx, 
                                                                        "));</script>\n<h2>Messages</h2>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                       (uw_begin_region(ctx),
                                                                         ((uw_write(ctx, 
                                                                           "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:5},x:{c:\"c\",v:{_Head:"), 0),
                                                                          (uw_begin_region(ctx),
                                                                            uw_Basis_htmlifySource_w(ctx,
                                                                             __uwr_buf_8.__uwf_Head
                                                                             ),
                                                                           uw_end_region(ctx),
                                                                            ((uw_write(ctx, 
                                                                              ",_Tail:"), 0),
                                                                             (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_buf_8.__uwf_Tail
                                                                               ),
                                                                              uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}}},x:{c:\"c\",v:null}}"), 0))))),
                                                                        uw_end_region(ctx),
                                                                         (uw_write(ctx, 
                                                                          "))</script>\n</body>"), 0))))))))))));
                  });
                 });
                });
              });
            });
           }));
   }
  
  static uw_unit
   __uwn_main_1766(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<h1>Current Channels</h1>\n<table>\n<tr> <th>ID</th> <th>Title</th> <th>#Subscribers</th> </tr>\n"), 0),
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
                                                                               PQexecPrepared(conn, "uw8", 0, paramValues, paramLengths, paramFormats, 0);
                                                                               
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
                                                                               uw_error(ctx, FATAL, "demo/chat.ur:81:33-81:37: Query failed:\n%s\n%s", 
                                                                               "SELECT T_T.uw_Id, T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               if (PQnfields(res) != 3) {
                                                                               int nf = PQnfields(res);
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "demo/chat.ur:81:33-81:37: Query returned %d columns instead of 3:\n%s\n%s", nf, 
                                                                               "SELECT T_T.uw_Id, T_T.uw_Room, T_T.uw_Title FROM uw_Chat_t AS T_T", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               uw_end_region(ctx);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                                                               n = PQntuples(res);
                                                                               for (i = 0; i < n; ++i) {
                                                                               struct __uws_9 __uwr_r_2;
                                                                               uw_unit
                                                                               __uwr_acc_3
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               =
                                                                               (PQgetisnull(res, i, 0) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/chat.ur:81:33-81:37: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 0)));
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Room
                                                                               =
                                                                               (PQgetisnull(res, i, 1) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/chat.ur:81:33-81:37: Unexpectedly NULL field #1");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 1)));
                                                                               
                                                                               __uwr_r_2.__uwf_T.__uwf_Title
                                                                               =
                                                                               (PQgetisnull(res, i, 2) ? 
                                                                               ({uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/chat.ur:81:33-81:37: Unexpectedly NULL field #2");
                                                                               tmp;
                                                                               }) : 
                                                                               PQgetvalue(res, i, 2));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               ({
                                                                               struct
                                                                               __uws_10
                                                                               __uwr_r_4
                                                                               =
                                                                               (uw_begin_region(ctx),
                                                                               ({
                                                                               struct
                                                                               __uws_10*
                                                                               disc
                                                                               =
                                                                               ({
                                                                               struct
                                                                               __uws_10*
                                                                               acc
                                                                               =
                                                                               NULL;
                                                                               int dummy = (uw_begin_region(ctx), 0);
                                                                               uw_ensure_transaction(ctx);
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               __uwr_r_2.__uwf_T.__uwf_Room;
                                                                               
                                                                               PGconn *conn = uw_get_db(ctx);
                                                                               static const int paramFormats[] = { 0 };
                                                                               const int *paramLengths = paramFormats;
                                                                               const char **paramValues = uw_malloc(ctx, 1 * sizeof(char*));
                                                                               paramValues[0] = 
                                                                               uw_Basis_attrifyInt(ctx, 
                                                                               arg1);
                                                                               
                                                                               
                                                                               PGresult *res = 
                                                                               PQexecPrepared(conn, "uw7", 1, paramValues, paramLengths, paramFormats, 0);
                                                                               
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
                                                                               uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Query failed:\n%s\n%s", 
                                                                               "SELECT COUNT(*) AS uw_N FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               if (PQnfields(res) != 1) {
                                                                               int nf = PQnfields(res);
                                                                               PQclear(res);
                                                                               uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Query returned %d columns instead of 1:\n%s\n%s", nf, 
                                                                               "SELECT COUNT(*) AS uw_N FROM uw_Chat_Room_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                                                               }
                                                                               
                                                                               uw_end_region(ctx);
                                                                               uw_push_cleanup(ctx, (void (*)(void *))PQclear, res);
                                                                               n = PQntuples(res);
                                                                               for (i = 0; i < n; ++i) {
                                                                               struct __uws_10 __uwr_r_4;
                                                                               struct
                                                                               __uws_10*
                                                                               __uwr_acc_5
                                                                               =
                                                                               acc;
                                                                               
                                                                               __uwr_r_4.__uwf_N
                                                                               =
                                                                               (PQgetisnull(res, i, 0) ? 
                                                                               ({uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:398:17-398:18: Unexpectedly NULL field #0");
                                                                               tmp;
                                                                               }) : 
                                                                               uw_Basis_stringToInt_error(ctx, 
                                                                               PQgetvalue(res, i, 0)));
                                                                               
                                                                               
                                                                               acc
                                                                               =
                                                                               ({
                                                                               struct
                                                                               __uws_10
                                                                               *tmp
                                                                               =
                                                                               uw_malloc(ctx, sizeof(
                                                                               struct
                                                                               __uws_10));
                                                                               *tmp
                                                                               =
                                                                               __uwr_r_4;
                                                                               tmp;
                                                                               });
                                                                               }
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                               acc;
                                                                               })
                                                                               ;
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               ({
                                                                               struct
                                                                               __uws_10
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:399:24-400:3: %s", 
                                                                               "Query returned no rows");
                                                                               tmp;
                                                                               })
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               struct
                                                                               __uws_10
                                                                               __uwr_r_4
                                                                               =
                                                                               (*disc);
                                                                               __uwr_r_4;
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               struct
                                                                               __uws_10
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/broadcast.ur:27:8-28:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               }));
                                                                               uw_end_region(ctx);
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
                                                                               __uwr_r_2.__uwf_T.__uwf_Title
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_r_4.__uwf_N
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</td>\n<td><form method=\"post\" action=\"/Chat/chat/"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_urlifyInt_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "\"><input type=\"submit\" value=\"Enter\" /></form></td>\n<td><form method=\"post\" action=\"/Chat/delete/"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_urlifyInt_w(ctx,
                                                                               __uwr_r_2.__uwf_T.__uwf_Id
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\"><input type=\"submit\" value=\"Delete\" /></form></td>\n</tr>"), 0)))))))))));
                                                                               });
                                                                               }
                                                                               
                                                                               uw_pop_cleanup(ctx);
                                                                               
                                                                              uw_end_region(ctx);
                                                                               acc;
                                                                              })),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\n</table>\n<h1>New Channel</h1>\n<form method=\"post\" action=\"/Chat/create\">\nTitle: <input type=\"text\" name=\"Title\" /><br />\n<input type=\"submit\" />\n</form>\n</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_$create_1756(uw_context ctx, struct __uws_11 __uwr_x0_0, 
                            uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), (uw_begin_region(ctx), ({
                                   uw_Basis_int arg1 =
                                    ({
                                     uw_Basis_int n;
                                     uw_ensure_transaction(ctx);
                                     char *query = "SELECT NEXTVAL('uw_Chat_s')";
                                      PGconn *conn = uw_get_db(ctx);
                                      PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);
                                      
                                      if (res == NULL) {
                                                         uw_try_reconnecting_and_restarting(ctx);
                                                         uw_error(ctx, FATAL, "Can't allocate NEXTVAL result; database server may be down.");
                                                         }
                                       
                                       if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                                       PQclear(res);
                                        uw_error(ctx, FATAL, ":0:0-0:0: Query failed:\n%s\n%s", query, PQerrorMessage(conn));
                                        }
                                       
                                       n = PQntuples(res);
                                       if (n != 1) {
                                       PQclear(res);
                                        uw_error(ctx, FATAL, ":0:0-0:0: Wrong number of result rows:\n%s\n%s", query, PQerrorMessage(conn));
                                        }
                                       
                                       n = uw_Basis_stringToInt_error(ctx, PQgetvalue(res, 0, 0));
                                       PQclear(res);
                                       
                                     
                                     n;
                                     });
                                    
                                    uw_Basis_int arg2 =
                                     ({
                                      uw_Basis_int n;
                                      uw_ensure_transaction(ctx);
                                      char *query = "SELECT NEXTVAL('uw_Chat_Room_s')";
                                       PGconn *conn = uw_get_db(ctx);
                                       PGresult *res = PQexecParams(conn, query, 0, NULL, NULL, NULL, NULL, 0);
                                       
                                       if (res == NULL) {
                                                          uw_try_reconnecting_and_restarting(ctx);
                                                          uw_error(ctx, FATAL, "Can't allocate NEXTVAL result; database server may be down.");
                                                          }
                                        
                                        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                                        PQclear(res);
                                         uw_error(ctx, FATAL, ":0:0-0:0: Query failed:\n%s\n%s", query, PQerrorMessage(conn));
                                         }
                                        
                                        n = PQntuples(res);
                                        if (n != 1) {
                                        PQclear(res);
                                         uw_error(ctx, FATAL, ":0:0-0:0: Wrong number of result rows:\n%s\n%s", query, PQerrorMessage(conn));
                                         }
                                        
                                        n = uw_Basis_stringToInt_error(ctx, PQgetvalue(res, 0, 0));
                                        PQclear(res);
                                        
                                      
                                      n;
                                      });
                                    
                                    uw_Basis_string arg3 =
                                     __uwr_x0_0.__uwf_Title;
                                    
                                    uw_ensure_transaction(ctx);
                                    
                                    PGconn *conn = uw_get_db(ctx);
                                     static const int paramFormats[] = { 0, 0, 
                                                                          0 };
                                      const int *paramLengths = paramFormats;
                                       const char **paramValues = uw_malloc(ctx, 3 * sizeof(char*));
                                      paramValues[0] = uw_Basis_attrifyInt(ctx, 
                                                        arg1);
                                       
                                       paramValues[1] = uw_Basis_attrifyInt(ctx, 
                                                         arg2);
                                        paramValues[2] = arg3;
                                         
                                      
                                     PGresult *res;
                                     
                                     res = PQexecPrepared(conn, "uw9", 3, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/chat.ur:73:12-74:19: DML failed:\n%s\n%s", 
                                        "INSERT INTO uw_Chat_t (uw_Id, uw_Room, uw_Title) VALUES ($1::int8, $2::int8, $3::text)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ({
             uw_unit arg0 = 0;
              uw_unit arg1 = 0;
             __uwn_main_1766(ctx, arg0, arg1);
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_delete_1755(uw_context ctx, uw_Basis_int __uwr_x1_0, 
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
                                     
                                     res = PQexecPrepared(conn, "uw10", 1, paramValues, paramLengths, paramFormats, 0);
                                     
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
                                        uw_error(ctx, FATAL, "demo/chat.ur:65:4-66:11: DML failed:\n%s\n%s", 
                                        "DELETE FROM uw_Chat_t AS T_T WHERE (T_T.uw_Id = $1::int8)", PQerrorMessage(conn));
                                       }
                                         
                                         PQclear(res);
                                         
                                   
                                   uw_end_region(ctx);
                                   0;
                                   })));
           uw_end_region(ctx);
            ({
             uw_unit arg0 = 0;
              uw_unit arg1 = 0;
             __uwn_main_1766(ctx, arg0, arg1);
             });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_1753(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit arg0 = __uwr_x0_0;
            uw_unit arg1 = 0;
           __uwn_main_1766(ctx, arg0, arg1);
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
 
 if (!strcmp(request, "/runtime.5176DF5A2DB34FEB6231025A1D7D8D4A059FC48D.js")) {
  uw_write_header(ctx, "Content-Type: text/javascript\r\n");
   uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
   uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
   uw_write(ctx, jslib);
   return;
   }
  
  
  if (!strcmp(request, "/app.D2DD3C69341A912E90692B56DDB3A95F6A939AB5.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 
 if (!strncmp(request, "/Chat/main", 10) && (request[10] == 0 || request[10] == '/')) {
  request += 10;
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
    __uwn_wrap_main_1753(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Chat/chat", 10) && (request[10] == 0 || request[10] == '/')) {
   request += 10;
   if (*request == '/') ++request;
   uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
    uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
     uw_write(ctx, uw_begin_html5);
    uw_mayReturnIndirectly(ctx);
    uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.5176DF5A2DB34FEB6231025A1D7D8D4A059FC48D.js\"></script>\n<script type=\"text/javascript\" src=\"/app.D2DD3C69341A912E90692B56DDB3A95F6A939AB5.js\"></script>\n");
    uw_set_could_write_db(ctx, 1);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 1);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_Basis_int arg0 = uw_Basis_unurlifyInt(ctx, &request);
     
      uw_unit uw_inputs;
      __uwn_wrap_chat_1754(ctx, arg0, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Chat/delete", 12) && (request[12] == 0 || request[12] == '/')) {
   request += 12;
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
      __uwn_wrap_delete_1755(ctx, arg0, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Chat/create", 12) && (request[12] == 0 || request[12] == '/')) {
   request += 12;
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
    uw_Basis_string uw_input_Title;
     
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input Title");
      uw_input_Title = uw_Basis_unurlifyString_fromClient(ctx, &request);
      struct __uws_11 uw_inputs = {
       uw_input_Title,
        };
     __uwn_wrap_$create_1756(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Chat/speak", 11) && (request[11] == 0 || request[11] == '/')) {
   request += 11;
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
      uw_unit it0 = __uwn_$speak_1745(ctx, arg0, arg1, 0);
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
  __uwn_expunger_1757(ctx, cli);
   }
 static void uw_initializer(uw_context ctx) {
 uw_begin_initializing(ctx);
  uw_end_initializing(ctx);
  __uwn_initializer_1758(ctx, 0);
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
 