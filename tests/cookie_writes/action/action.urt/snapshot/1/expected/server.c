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
    static int uw_db_check(uw_context ctx) { return 0; };
 
 /* No global setup for LRU cache. */
  
 
  
  struct __uws_1 {
   uw_Basis_string __uwf_Suffix;
    };
  /* cookie Action/c */
  
  static uw_unit
   __uwn_wrap_bump_1(uw_context ctx, struct __uws_1 __uwr_x0_0, 
                      uw_unit __uwr___1)
   {
   return(({
           uw_Basis_string __uwr_v_2 =
           ({
            uw_Basis_string disc =
            ({
             uw_io_transaction __uwt;
             uw_Basis_string
             __uwr;
             uw_set_could_write_db(ctx, 0);
             uw_io_transaction_begin(ctx, &__uwt);
             while (1) {
             int __uwfk = setjmp(*uw_jmp_buf(ctx));
              if (__uwfk == 0) {
              __uwr =
               ({
                uw_Basis_string request = uw_maybe_strdup(ctx, uw_Basis_get_cookie(ctx,
                                                                "Action/c"));
                
                (request ? uw_Basis_unurlifyString(ctx, &request) : NULL);
                });
               uw_io_transaction_commit(ctx, &__uwt);
               break;
               } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
              break;
               }
              }
             uw_io_transaction_end(ctx, &__uwt, 1);
             __uwr;
             });
            
            disc == NULL ?
             ({
              uw_Basis_string arg0 = "first";
               uw_Basis_string arg1 = __uwr_x0_0.__uwf_Suffix;
                uw_Basis_strcat(ctx, arg0, arg1);
              })
              :
             disc != NULL && 1 ?
              ({uw_Basis_string __uwr_v_2 = disc;
                 ({
                  uw_Basis_string arg0 = __uwr_v_2;
                   uw_Basis_string arg1 = __uwr_x0_0.__uwf_Suffix;
                    uw_Basis_strcat(ctx, arg0, arg1);
                  });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/cookie_writes/action/action.ur:10:16-11:4: pattern match failure");
               tmp;
               });
            });
           ({
            uw_unit __uwr___3 =
            (uw_begin_region(ctx), ({
                                    uw_io_transaction __uwt;
                                    uw_unit
                                    __uwr;
                                    uw_set_could_write_db(ctx, 0);
                                    uw_io_transaction_begin(ctx, &__uwt);
                                    while (1) {
                                    int __uwfk = setjmp(*uw_jmp_buf(ctx));
                                     if (__uwfk == 0) {
                                     __uwr =
                                      ({
                                       uw_Basis_string arg0 = "/";
                                        uw_Basis_string arg1 = "Action/c";
                                         
                                        uw_Basis_string arg2 =
                                         uw_Basis_urlifyString(ctx, __uwr_v_2);
                                         uw_Basis_time* arg3 = NULL;
                                          uw_Basis_bool arg4 = uw_Basis_False;
                                           uw_Basis_bool arg5 = uw_Basis_False;
                                            uw_Basis_set_cookie(ctx, arg0, 
                                                                      arg1, 
                                                                      arg2, 
                                                                      arg3, 
                                                                      arg4, 
                                                                      arg5);
                                       });
                                      uw_io_transaction_commit(ctx, &__uwt);
                                      break;
                                      } else if (!uw_io_transaction_retry(ctx, &__uwt, __uwfk)) {
                                     break;
                                      }
                                     }
                                    uw_io_transaction_end(ctx, &__uwt, 1);
                                    __uwr;
                                    }));
            uw_end_region(ctx);
             ((uw_write(ctx, "<body"), 0),
              (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                     uw_Basis_get_settings(ctx,
                                                      0))), 0),
               uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                          "")), 0),
                                    uw_end_region(ctx), ((uw_write(ctx, ">now "), 0),
                                                         (uw_begin_region(ctx), 
                                                          uw_Basis_htmlifyString_w(ctx,
                                                           __uwr_v_2),
                                                          uw_end_region(ctx), (uw_write(ctx, 
                                                                               "</body>"), 0))))));
            });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_2(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<form method=\"post\" action=\"/Action/bump\"><input type=\"hidden\" name=\"Sig\" value=\""), 0),
                                                      (uw_begin_region(ctx), (uw_write(ctx, 
                                                                              uw_Basis_sigString(ctx,
                                                                               0
                                                                               )), 0),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "\" /><input type=\"text\" name=\"Suffix\" /><input type=\"submit\" value=\"bump\" /></form>\n</body>"), 0)))))));
   }
 
 static int uw_input_num(const char *name) {
 if (name[0] == 0) return
  -1;
  switch (name[1]) {
   case 'i':
    return 0;
    case 'u':
     return 1;
     default:
   return -1;
   }}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Action/main")) return 1;
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
  uw_sign(uw_unnull(uw_Basis_get_cookie(ctx, "Action/c")), r);
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
 
 if (!strncmp(request, "/Action/main", 12) && (request[12] == 0 || request[12] == '/')) {
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
    __uwn_wrap_main_2(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    uw_finish_page(ctx);
    return;
   }
   }
  
  if (!strncmp(request, "/Action/bump", 12) && (request[12] == 0 || request[12] == '/')) {
   request += 12;
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
     uw_set_could_write_db(ctx, 0);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_Basis_string uw_input_Suffix;
     
     request = uw_get_input(ctx, 1);
      if (request == NULL)
      uw_error(ctx, FATAL, "Missing input Suffix");
      uw_input_Suffix = uw_Basis_unurlifyString_fromClient(ctx, &request);
      struct __uws_1 uw_inputs = {
       uw_input_Suffix,
        };
     uw_io_request __uwr;
    uw_io_request_begin(ctx, &__uwr);
    {
    int __uwfk = setjmp(*uw_jmp_buf(ctx));
     if (__uwfk == 0) {
     __uwn_wrap_bump_1(ctx, uw_inputs, 0);
      uw_io_request_end(ctx, &__uwr, 0);
      uw_write(ctx, "</html>");
      uw_finish_page(ctx);
      return;
      } else
     uw_io_request_end(ctx, &__uwr, __uwfk);
      }
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
 uw_set_could_write_db(ctx, 0);
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
                                                       1};
 