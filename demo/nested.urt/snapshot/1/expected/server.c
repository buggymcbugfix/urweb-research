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
  
 
  
  struct __uws_1 {
   uw_Basis_string __uwf_Surname;
    };
  
  struct __uws_2
   {
   uw_Basis_bool __uwf_EnterSurname;
    uw_Basis_string __uwf_Forename;
     };
  
  static uw_unit
   __uwn_$pageC_9(uw_context ctx, uw_Basis_string __uwr_forename_0, 
                   uw_Basis_string __uwr_surname_1, uw_unit __uwr___2)
   {
   return(((uw_write(ctx, "\n<head>\n<title>C</title>\n</head>\n<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<p>Hello "), 0),
                                                      (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                                                              __uwr_forename_0
                                                                              ),
                                                       uw_end_region(ctx), (uw_begin_region(ctx),
                                                                             ({
                                                                              uw_Basis_string
                                                                              disc
                                                                              =
                                                                              __uwr_surname_1;
                                                                              
                                                                              disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               0
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_string
                                                                               __uwr_s_3
                                                                               =
                                                                               disc;
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwr_s_3
                                                                               ));
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_unit
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/nested.ur:47:16-47:19: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                              }),
                                                                            uw_end_region(ctx),
                                                                             ((uw_write(ctx, 
                                                                               "</p>\n"), 0),
                                                                              (uw_begin_region(ctx),
                                                                               ({
                                                                               uw_Basis_string
                                                                               disc
                                                                               =
                                                                               __uwr_surname_1;
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               (uw_write(ctx, 
                                                                               "<a href=\"/Nested/pageA\">Previous</a>"), 0)
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_string
                                                                               __uwr___3
                                                                               =
                                                                               disc;
                                                                               ((uw_write(ctx, 
                                                                               "<a href=\"/Nested/pageB/"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_urlifyString_w(ctx,
                                                                               __uwr_forename_0
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "/_\">Previous</a>"), 0)));
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_unit
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/nested.ur:47:19-49:66: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               }),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\n</body>\n"), 0))))))))));
   }
  
  static uw_unit
   __uwn_wrap_$pageCPRIME_2(uw_context ctx, uw_Basis_string __uwr_x1_0, 
                             struct __uws_1 __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           uw_Basis_string arg0 = __uwr_x1_0;
            uw_Basis_string arg1 = __uwr_x0_1.__uwf_Surname;
            uw_unit arg2 = 0;
           __uwn_$pageC_9(ctx, arg0, arg1, arg2);
           }));
   }
  
  static uw_unit
   __uwn_$pageB_10(uw_context ctx, uw_Basis_string __uwr_forename_0, 
                    uw_unit __uwr_$x_1, uw_unit __uwr___2)
   {
   return(((uw_write(ctx, "\n<head>\n<title>B</title>\n</head>\n<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<form method=\"post\" action=\"/Nested/pageC'/"), 0),
                                                      (uw_begin_region(ctx), uw_Basis_urlifyString_w(ctx,
                                                                              __uwr_forename_0
                                                                              ),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\">\nSurname:\n<input type=\"text\" name=\"Surname\" />\n<input type=\"submit\" />\n</form>\n<a href=\"/Nested/pageA\">Previous</a>\n</body>\n"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_$pageB_4(uw_context ctx, uw_Basis_string __uwr_x1_0, 
                        uw_unit __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           uw_Basis_string arg0 = __uwr_x1_0;
            uw_unit arg1 = __uwr_x0_1;
            uw_unit arg2 = 0;
           __uwn_$pageB_10(ctx, arg0, arg1, arg2);
           }));
   }
  
  static uw_unit
   __uwn_wrap_fromA_5(uw_context ctx, struct __uws_2 __uwr_x0_0, 
                       uw_unit __uwr___1)
   {
   return(({
           uw_Basis_bool disc =
           __uwr_x0_0.__uwf_EnterSurname;
           
           disc == uw_Basis_True ?
            ({
             uw_Basis_string arg0 = __uwr_x0_0.__uwf_Forename;
              uw_unit arg1 = 0;
              uw_unit arg2 = 0;
             __uwn_$pageB_10(ctx, arg0, arg1, arg2);
             })
             :
            disc == uw_Basis_False ?
             ({
              uw_Basis_string arg0 = __uwr_x0_0.__uwf_Forename;
               uw_Basis_string arg1 = NULL;
               uw_unit arg2 = 0;
              __uwn_$pageC_9(ctx, arg0, arg1, arg2);
              })
              :
             ({
              uw_unit
              tmp;
              uw_error(ctx, FATAL, "demo/nested.ur:1:~4-60:7: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_unit
   __uwn_pageA_11(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "\n<head>\n<title>A</title>\n</head>\n<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), (uw_write(ctx, ">\n<form method=\"post\" action=\"/Nested/fromA\">\n<table>\n<tr>\n<td>Forename:</td>\n<td><input type=\"text\" name=\"Forename\" /></td>\n</tr>\n<tr>\n<td>Enter a Surname\?</td>\n<td><input type=\"checkbox\" name=\"EnterSurname\" /></td>\n</tr>\n</table>\n<input type=\"submit\" />\n</form>\n</body>\n"), 0)))));
   }
  
  static uw_unit
   __uwn_wrap_pageA_7(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit arg0 = __uwr_x0_0;
            uw_unit arg1 = 0;
           __uwn_pageA_11(ctx, arg0, arg1);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_8(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit arg0 = 0;
            uw_unit arg1 = 0;
           __uwn_pageA_11(ctx, arg0, arg1);
           }));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'E':
   return 0;
   case 'F':
    return 1;
    case 'S':
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
 
 
 
 if (!strncmp(request, "/Nested/main", 12) && (request[12] == 0 || request[12] == '/')) {
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
    __uwn_wrap_main_8(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Nested/fromA", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
    uw_Basis_bool uw_input_EnterSurname;
     uw_Basis_string uw_input_Forename;
      
     request = uw_get_optional_input(ctx, 0);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input EnterSurname");
      uw_input_EnterSurname = uw_Basis_unurlifyBool(ctx, &request);
      request = uw_get_input(ctx, 1);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input Forename");
       uw_input_Forename = uw_Basis_unurlifyString_fromClient(ctx, &request);
       struct __uws_2 uw_inputs = {
        uw_input_EnterSurname,
         uw_input_Forename,
          };
     __uwn_wrap_fromA_5(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Nested/pageC'", 14) && (request[14] == 0 || request[14] == '/')) {
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
    uw_Basis_string arg0 = uw_Basis_unurlifyString(ctx, &request);
     uw_Basis_string uw_input_Surname;
      
      request = uw_get_input(ctx, 0);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input Surname");
       uw_input_Surname = uw_Basis_unurlifyString_fromClient(ctx, &request);
       struct __uws_1 uw_inputs = {
        uw_input_Surname,
         };
      __uwn_wrap_$pageCPRIME_2(ctx, arg0, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Nested/pageA", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
     __uwn_wrap_pageA_7(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Nested/pageB", 13) && (request[13] == 0 || request[13] == '/')) {
   request += 13;
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
    uw_Basis_string arg0 = uw_Basis_unurlifyString(ctx, &request);
     uw_unit arg1 = uw_Basis_unurlifyUnit(ctx, &request);
      __uwn_wrap_$pageB_4(ctx, arg0, arg1, 0);
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
 