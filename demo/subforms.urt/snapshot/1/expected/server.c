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
   uw_Basis_string __uwf_Num;
    uw_Basis_string __uwf_Text;
     };
  struct __uws_2 {
   struct __uws_1 __uwf_1;
    struct __uws_2* __uwf_2;
     };
  struct __uws_3 {
   struct __uws_2* __uwf_Lines;
    };
  
  static uw_unit __uwn_$subPRIME_7(uw_context, struct __uws_2*);
   static uw_unit __uwn_$subPRIME_7(uw_context ctx, struct __uws_2* __uwr_ls_0)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_ls_0;
            
            disc == NULL ? 0
              :
             disc != NULL && 1 && 1 ?
              ({struct __uws_1 __uwr_r_1 = (*disc).__uwf_1;
                 struct __uws_2* __uwr_ls_2 = (*disc).__uwf_2;
                  ((uw_write(ctx, "\n<p>"), 0),
                   (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                           __uwr_r_1.__uwf_Num),
                    uw_end_region(ctx), ((uw_write(ctx, " = "), 0),
                                         (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                                                 __uwr_r_1.__uwf_Text
                                                                 ),
                                          uw_end_region(ctx), ((uw_write(ctx, "</p>\n"), 0),
                                                               (uw_begin_region(ctx),
                                                                 __uwn_$subPRIME_7(ctx,
                                                                  __uwr_ls_2),
                                                                uw_end_region(ctx),
                                                                 (uw_write(ctx, 
                                                                  "\n"), 0)))))));
               })
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, "demo/subforms.ur:1:~4-14:7: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_subfrms_8(uw_context, uw_Basis_int);
   static uw_unit __uwn_subfrms_8(uw_context ctx, uw_Basis_int __uwr_n_0)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            __uwr_n_0 <= 0LL;
            
            disc == uw_Basis_True ? 0
              :
             disc == uw_Basis_False ?
              ((uw_write(ctx, "\n<input type=\"hidden\" name=\".i\" value=\"1\" />\n<input value=\""), 0),
               (uw_begin_region(ctx), uw_Basis_attrifyString_w(ctx,
                                       uw_Basis_intToString(ctx, __uwr_n_0)),
                uw_end_region(ctx), ((uw_write(ctx, "\" type=\"hidden\" name=\"Num\" />\n<p>"), 0),
                                     (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                             __uwr_n_0),
                                      uw_end_region(ctx), ((uw_write(ctx, ": <input type=\"text\" name=\"Text\" /></p>\n<input type=\"hidden\" name=\".e\" value=\"1\" />\n"), 0),
                                                           (uw_begin_region(ctx),
                                                             __uwn_subfrms_8(ctx,
                                                              __uwr_n_0 - 1LL),
                                                            uw_end_region(ctx), 
                                                            (uw_write(ctx, "\n"), 0)))))))
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, "demo/subforms.ur:16:0-28:2: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_wrap_sub_3(uw_context ctx, struct __uws_3 __uwr_x0_0, 
                     uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n"), 0),
                                                      (uw_begin_region(ctx), __uwn_$subPRIME_7(ctx,
                                                                              __uwr_x0_0.__uwf_Lines),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\n</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_form_9(uw_context ctx, uw_Basis_int __uwr_n_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<form method=\"post\" action=\"/Subforms/sub\">\n<input type=\"hidden\" name=\".s\" value=\"Lines\" />\n"), 0),
                                                      (uw_begin_region(ctx), __uwn_subfrms_8(ctx,
                                                                              __uwr_n_0),
                                                       uw_end_region(ctx), ((uw_write(ctx, 
                                                                             "\n<input type=\"hidden\" name=\".e\" value=\"1\" />\n<input type=\"submit\" />\n</form>\n<a href=\"/Subforms/form/"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              uw_Basis_urlifyInt_w(ctx,
                                                                               __uwr_n_0
                                                                               +
                                                                               1LL
                                                                               ),
                                                                             uw_end_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "\">One more blank</a><br />\n"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ({
                                                                               uw_Basis_bool
                                                                               disc
                                                                               =
                                                                               !
                                                                               (__uwr_n_0
                                                                               <=
                                                                               0LL);
                                                                               
                                                                               disc
                                                                               
                                                                               ==
                                                                               uw_Basis_True
                                                                               
                                                                               ?
                                                                               ((uw_write(ctx, 
                                                                               "<a href=\"/Subforms/form/"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_urlifyInt_w(ctx,
                                                                               __uwr_n_0
                                                                               -
                                                                               1LL
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\">One fewer blank</a>"), 0)))
                                                                               
                                                                               :
                                                                               disc
                                                                               
                                                                               ==
                                                                               uw_Basis_False
                                                                               
                                                                               ?
                                                                               0
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_unit
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "demo/subforms.ur:36:47-37:3: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               }),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\n</body>"), 0)))))))))));
   }
  
  static uw_unit
   __uwn_wrap_form_5(uw_context ctx, uw_Basis_int __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int arg0 = __uwr_x0_0;
            uw_unit arg1 = 0;
           __uwn_form_9(ctx, arg0, arg1);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_6(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_int arg0 = 1LL;
            uw_unit arg1 = 0;
           __uwn_form_9(ctx, arg0, arg1);
           }));
   }
 
 static int uw_input_num(const char *name) {
 switch (name[0])
  {
  case 'L':
   return 0;
   case 'N':
    return 0;
    case 'T':
     return 1;
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
 
 
 
 if (!strncmp(request, "/Subforms/main", 14) && (request[14] == 0 || request[14] == '/')) {
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
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_6(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Subforms/sub", 13) && (request[13] == 0 || request[13] == '/')) {
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
    struct __uws_2* uw_input_Lines;
     
     {
      int status;
      uw_input_Lines =
      NULL;
      for (status = uw_enter_subforms(ctx, 0); status; status = uw_next_entry(ctx)) {
      struct __uws_2* result =
       uw_malloc(ctx, sizeof(struct __uws_2));
       {uw_Basis_string uw_input_Num;
         uw_Basis_string uw_input_Text;
          
        request = uw_get_input(ctx, 0);
         if (request == NULL)
         uw_error(ctx, FATAL, "Missing input Num");
         uw_input_Num = uw_Basis_unurlifyString_fromClient(ctx, &request);
         result->__uwf_1.__uwf_Num = uw_input_Num;
         
         request = uw_get_input(ctx, 1);
          if (request == NULL)
          uw_error(ctx, FATAL, "Missing input Text");
          uw_input_Text = uw_Basis_unurlifyString_fromClient(ctx, &request);
          result->__uwf_1.__uwf_Text = uw_input_Text;
          }
        
       result->__uwf_2 = uw_input_Lines;
       uw_input_Lines = result;
       }}
      struct __uws_3 uw_inputs = {
       uw_input_Lines,
        };
     __uwn_wrap_sub_3(ctx, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/Subforms/form", 14) && (request[14] == 0 || request[14] == '/')) {
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
     __uwn_wrap_form_5(ctx, arg0, 0);
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
 