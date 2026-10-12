#include "include/urweb/config.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <time.h>
 #include "include/urweb/urweb.h"
 
 static void uw_setup_limits() {
  }
  
  void uw_global_custom() {
   uw_setup_limits();
   }
   static void uw_client_init(void) { };
    static void uw_db_init(uw_context ctx) { };
    static int uw_db_begin(uw_context ctx, int could_write) { return 0; };
    static void uw_db_close(uw_context ctx) { };
    static int uw_db_commit(uw_context ctx) { return 0; };
    static int uw_db_rollback(uw_context ctx) { return 0; };
 
 /* No global setup for LRU cache. */
  
 
  
  
  struct __uws_1
   {
   uw_Basis_string __uwf_A;
    uw_Basis_string __uwf_B;
     uw_Basis_string __uwf_C;
      };
  
  struct __uws_2
   {
   uw_Basis_string __uwf_A;
    uw_Basis_float __uwf_B;
     uw_Basis_int __uwf_C;
      };
  /* cookie Cookie/c */
  
  static uw_unit
   __uwn_wrap_setExp_1(uw_context ctx, struct __uws_1 __uwr_x0_0, 
                        uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 = "/";
                                    uw_Basis_string arg1 = "Cookie/c";
                                     
                                    uw_Basis_string arg2 =
                                     ({
                                      uw_Basis_string arg0 =
                                       uw_Basis_urlifyString(ctx,
                                        __uwr_x0_0.__uwf_A);
                                       uw_Basis_string arg1 = "/";
                                        
                                       uw_Basis_string arg2 =
                                        uw_Basis_urlifyFloat(ctx,
                                         uw_Basis_stringToFloat_error(ctx,
                                          __uwr_x0_0.__uwf_B));
                                        uw_Basis_string arg3 = "/";
                                         
                                       uw_Basis_string arg4 =
                                        uw_Basis_urlifyInt(ctx,
                                         uw_Basis_stringToInt_error(ctx,
                                          __uwr_x0_0.__uwf_C));
                                        uw_Basis_mstrcat(ctx, arg0, arg1, arg2, 
                                                               arg3, arg4, NULL);
                                      });
                                     
                                    uw_Basis_time* arg3 =
                                     ({
                                      uw_Basis_time *tmp =
                                      uw_malloc(ctx, sizeof(uw_Basis_time));
                                      *tmp =
                                      uw_Basis_stringToTime_error(ctx,
                                       "2012-11-6 00:00:00");
                                      tmp;
                                      });
                                     uw_Basis_bool arg4 = uw_Basis_False;
                                      uw_Basis_bool arg5 = uw_Basis_True;
                                       uw_Basis_set_cookie(ctx, arg0, arg1, 
                                                                 arg2, arg3, 
                                                                 arg4, arg5);
                                   }));
           uw_end_region(ctx);
            (uw_write(ctx, "Cookie set robustly."), 0);
           }));
   }
  
  static uw_unit
   __uwn_wrap_set_2(uw_context ctx, struct __uws_1 __uwr_x0_0, 
                     uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 = "/";
                                    uw_Basis_string arg1 = "Cookie/c";
                                     
                                    uw_Basis_string arg2 =
                                     ({
                                      uw_Basis_string arg0 =
                                       uw_Basis_urlifyString(ctx,
                                        __uwr_x0_0.__uwf_A);
                                       uw_Basis_string arg1 = "/";
                                        
                                       uw_Basis_string arg2 =
                                        uw_Basis_urlifyFloat(ctx,
                                         uw_Basis_stringToFloat_error(ctx,
                                          __uwr_x0_0.__uwf_B));
                                        uw_Basis_string arg3 = "/";
                                         
                                       uw_Basis_string arg4 =
                                        uw_Basis_urlifyInt(ctx,
                                         uw_Basis_stringToInt_error(ctx,
                                          __uwr_x0_0.__uwf_C));
                                        uw_Basis_mstrcat(ctx, arg0, arg1, arg2, 
                                                               arg3, arg4, NULL);
                                      });
                                     uw_Basis_time* arg3 = NULL;
                                      uw_Basis_bool arg4 = uw_Basis_False;
                                       uw_Basis_bool arg5 = uw_Basis_True;
                                        uw_Basis_set_cookie(ctx, arg0, arg1, 
                                                                  arg2, arg3, 
                                                                  arg4, arg5);
                                   }));
           uw_end_region(ctx);
            (uw_write(ctx, "Cookie set."), 0);
           }));
   }
  
  static uw_unit
   __uwn_wrap_delete_3(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), ({
                                   uw_Basis_string arg0 = "/";
                                    uw_Basis_string arg1 = "Cookie/c";
                                     uw_Basis_clear_cookie(ctx, arg0, arg1);
                                   }));
           uw_end_region(ctx);
            (uw_write(ctx, "Cookie cleared."), 0);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_4(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n"), 0),
                                                      (uw_begin_region(ctx), ({
                                                                              struct
                                                                               __uws_2*
                                                                              disc
                                                                              =
                                                                              ({
                                                                               uw_Basis_string request = uw_maybe_strdup(ctx, 
                                                                               uw_Basis_get_cookie(ctx,
                                                                               "Cookie/c"
                                                                               ));
                                                                               
                                                                               (request ? 
                                                                               ({
                                                                               struct
                                                                               __uws_2 *tmp = uw_malloc(ctx, sizeof(
                                                                               struct
                                                                               __uws_2));
                                                                               *tmp = 
                                                                               ({
                                                                               uw_Basis_string
                                                                               uwr_A
                                                                               =
                                                                               uw_Basis_unurlifyString(ctx, &request);
                                                                               uw_Basis_float
                                                                               uwr_B
                                                                               =
                                                                               uw_Basis_unurlifyFloat(ctx, &request);
                                                                               uw_Basis_int
                                                                               uwr_C
                                                                               =
                                                                               uw_Basis_unurlifyInt(ctx, &request);
                                                                               struct
                                                                               __uws_2
                                                                               tmp
                                                                               =
                                                                               {
                                                                               uwr_A
                                                                               ,
                                                                               
                                                                               uwr_B
                                                                               ,
                                                                               
                                                                               uwr_C
                                                                               };
                                                                               tmp;
                                                                               });
                                                                               tmp;
                                                                               }) : NULL);
                                                                               });
                                                                              
                                                                              disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               (uw_write(ctx, 
                                                                               "No cookie set."), 0)
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               struct
                                                                               __uws_2
                                                                               __uwr_v_2
                                                                               =
                                                                               (*disc);
                                                                               ((uw_write(ctx, 
                                                                               "\nCookie: A = "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_v_2.__uwf_A
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ", B = "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyFloat_w(ctx,
                                                                               __uwr_v_2.__uwf_B
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ", C = "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               __uwr_v_2.__uwf_C
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "<br />\n<form method=\"post\" action=\"/Cookie/delete\"><input type=\"submit\" value=\"Delete\" /></form>\n"), 0)))))));
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_unit
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/cookie.ur:24:3-46:8: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                              }),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\n<br /><br />\n<form method=\"post\" action=\"/Cookie/set\">\nA: <input type=\"text\" name=\"A\" /><br />\nB: <input type=\"text\" name=\"B\" /><br />\nC: <input type=\"text\" name=\"C\" /><br />\n<input type=\"submit\" />\n</form><br />\n<form method=\"post\" action=\"/Cookie/setExp\">\n<b>Version that expires on November 6, 2012:</b><br />\nA: <input type=\"text\" name=\"A\" /><br />\nB: <input type=\"text\" name=\"B\" /><br />\nC: <input type=\"text\" name=\"C\" /><br />\n<input type=\"submit\" />\n</form>\n</body>"), 0)))))));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'A':
   return 0;
   case 'B':
    return 1;
    case 'C':
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
  uw_sign(uw_unnull(uw_Basis_get_cookie(ctx, "Cookie/c")), r);
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
 
 if (!strncmp(request, "/Cookie/main", 12) && (request[12] == 0 || request[12] == '/')) {
  request += 12;
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
    __uwn_wrap_main_4(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Cookie/delete", 14) && (request[14] == 0 || request[14] == '/')) {
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
    
     uw_unit uw_inputs;
     __uwn_wrap_delete_3(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Cookie/set", 11) && (request[11] == 0 || request[11] == '/')) {
   request += 11;
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
    uw_Basis_string uw_input_A;
     uw_Basis_string uw_input_B;
      uw_Basis_string uw_input_C;
       
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input A");
      uw_input_A = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input B");
       uw_input_B = uw_Basis_unurlifyString_fromClient(ctx, &request);
       request = uw_get_input(ctx, 2);
        if (request == NULL)
        uw_error(ctx, FATAL, "Missing input C");
        uw_input_C = uw_Basis_unurlifyString_fromClient(ctx, &request);
        struct __uws_1 uw_inputs = {
         uw_input_A,
          uw_input_B,
           uw_input_C,
            };
     __uwn_wrap_set_2(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Cookie/setExp", 14) && (request[14] == 0 || request[14] == '/')) {
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
    uw_Basis_string uw_input_A;
     uw_Basis_string uw_input_B;
      uw_Basis_string uw_input_C;
       
     request = uw_get_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input A");
      uw_input_A = uw_Basis_unurlifyString_fromClient(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input B");
       uw_input_B = uw_Basis_unurlifyString_fromClient(ctx, &request);
       request = uw_get_input(ctx, 2);
        if (request == NULL)
        uw_error(ctx, FATAL, "Missing input C");
        uw_input_C = uw_Basis_unurlifyString_fromClient(ctx, &request);
        struct __uws_1 uw_inputs = {
         uw_input_A,
          uw_input_B,
           uw_input_C,
            };
     __uwn_wrap_setExp_1(ctx, uw_inputs, 0);
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
  }
 static void uw_initializer(uw_context ctx) {
 uw_begin_initializing(ctx);
  uw_end_initializing(ctx);
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
                                                                  NULL,
                                                                       
                           uw_served_files};
 