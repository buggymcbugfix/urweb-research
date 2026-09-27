#include "include/urweb/config.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <math.h>
 #include <time.h>
 #include "tests/ffi_datatype/outc.h"
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
  
 
  
  
  static uw_Basis_string
   __uwn_urShow_1(uw_context ctx, uw_Outc_outcome __uwr_o_0)
   {
   return(({
           uw_Outc_outcome disc =
           __uwr_o_0;
           
           disc->tag == uw_Outc_Sent ? "Ur:sent"
             :
            disc->tag == uw_Outc_NotSent && 1 ?
             ({uw_Basis_string __uwr_m_1 = disc->data.uw_NotSent;
                ({
                 uw_Basis_string arg0 = "Ur:notsent:";
                  uw_Basis_string arg1 = __uwr_m_1;
                   uw_Basis_strcat(ctx, arg0, arg1);
                 });
              })
              :
             disc->tag == uw_Outc_Unknown && 1 ?
              ({uw_Basis_string __uwr_m_1 = disc->data.uw_Unknown;
                 ({
                  uw_Basis_string arg0 = "Ur:unknown:";
                   uw_Basis_string arg1 = __uwr_m_1;
                    uw_Basis_strcat(ctx, arg0, arg1);
                  });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/ffi_datatype/ffi_datatype.ur:5:4-10:0: pattern match failure");
               tmp;
               });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_2(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Outc_outcome __uwr_a_2 =
           uw_Outc_send(ctx, "ok");
           ({
            uw_Outc_outcome __uwr_b_3 =
            uw_Outc_send(ctx, "later");
            ({
             uw_Outc_outcome __uwr_c_4 =
             uw_Outc_send(ctx, "\?");
             ((uw_write(ctx, "<body"), 0),
              (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                     uw_Basis_get_settings(ctx,
                                                      0))), 0),
               uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                          "")), 0),
                                    uw_end_region(ctx), ((uw_write(ctx, ">\n<p>"), 0),
                                                         (uw_begin_region(ctx), 
                                                          uw_Basis_htmlifyString_w(ctx,
                                                           __uwn_urShow_1(ctx,
                                                            __uwr_a_2)),
                                                          uw_end_region(ctx), ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               uw_Outc_describe(ctx,
                                                                               __uwr_a_2
                                                                               )),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</p>\n<p>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwn_urShow_1(ctx,
                                                                               __uwr_b_3)
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               uw_Outc_describe(ctx,
                                                                               __uwr_b_3
                                                                               )),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</p>\n<p>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwn_urShow_1(ctx,
                                                                               __uwr_c_4)
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               uw_Outc_describe(ctx,
                                                                               __uwr_c_4
                                                                               )),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</p>\n<p>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               __uwn_urShow_1(ctx,
                                                                               ({
                                                                               struct
                                                                               uw_Outc_outcome
                                                                               *tmp
                                                                               =
                                                                               uw_malloc(ctx, sizeof(struct uw_Outc_outcome));
                                                                               tmp->tag
                                                                               =
                                                                               uw_Outc_NotSent;
                                                                               tmp->data.uw_NotSent
                                                                               =
                                                                               "made in Ur";
                                                                               tmp;
                                                                               }))
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               uw_Outc_describe(ctx,
                                                                               ({
                                                                               struct
                                                                               uw_Outc_outcome
                                                                               *tmp
                                                                               =
                                                                               uw_malloc(ctx, sizeof(struct uw_Outc_outcome));
                                                                               tmp->tag
                                                                               =
                                                                               uw_Outc_NotSent;
                                                                               tmp->data.uw_NotSent
                                                                               =
                                                                               "made in Ur";
                                                                               tmp;
                                                                               })
                                                                               )),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</p>\n</body>"), 0))))))))))))))))))));
             });
            });
           }));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/Ffi_datatype/main")) return 1;
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
 
 if (!strncmp(request, "/Ffi_datatype/main", 18) && (request[18] == 0 || request[18] == '/')) {
  request += 18;
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
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_2(ctx, arg0, 0);
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
 