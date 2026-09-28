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
  
 
  
  
  static uw_unit __uwn_describe_1(uw_context ctx, uw_Basis_file __uwr_f_0)
   {
   return(((uw_write(ctx, "\n"), 0),
           (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                   ({
                                    uw_Basis_string disc =
                                    uw_Basis_fileName(ctx, __uwr_f_0);
                                    
                                    disc == NULL ? "(no name)"
                                      :
                                     disc != NULL && 1 ?
                                      ({uw_Basis_string __uwr_v_1 = disc;
                                         __uwr_v_1;
                                       })
                                       :
                                      ({
                                       uw_Basis_string
                                       tmp;
                                       uw_error(ctx, FATAL, "$/top.ur:115:10-115:18: pattern match failure");
                                       tmp;
                                       });
                                    })),
            uw_end_region(ctx), ((uw_write(ctx, " ["), 0),
                                 (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                                         uw_Basis_fileMimeType(ctx,
                                                          __uwr_f_0)),
                                  uw_end_region(ctx), ((uw_write(ctx, "] "), 0),
                                                       (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               uw_Basis_blobSize(ctx,
                                                                               uw_Basis_fileData(ctx,
                                                                               __uwr_f_0
                                                                               ))
                                                                               ),
                                                        uw_end_region(ctx), ((uw_write(ctx, 
                                                                              " bytes:\n"), 0),
                                                                             (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_Basis_string
                                                                               disc
                                                                               =
                                                                               uw_Basis_textOfBlob(ctx,
                                                                               uw_Basis_fileData(ctx,
                                                                               __uwr_f_0
                                                                               ));
                                                                               
                                                                               disc
                                                                               ==
                                                                               NULL
                                                                               ?
                                                                               "(binary)"
                                                                               
                                                                               :
                                                                               disc
                                                                               !=
                                                                               NULL
                                                                               &&
                                                                               1
                                                                               ?
                                                                               ({
                                                                               uw_Basis_string
                                                                               __uwr_s_1
                                                                               =
                                                                               disc;
                                                                               __uwr_s_1;
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
                                                                              uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\n"), 0))))))))));
   }
  
  static uw_unit __uwn_check_2(uw_context ctx, uw_Basis_string __uwr_path_0)
   {
   return(({
           uw_Basis_file* disc =
           uw_Basis_checkServedFile(ctx, __uwr_path_0);
           
           disc != NULL && 1 ?
            ({uw_Basis_file __uwr_f_1 = (*disc);
               __uwn_describe_1(ctx, __uwr_f_1);
             })
             :
            disc == NULL ?
             ((uw_write(ctx, "nothing serves "), 0),
              uw_Basis_htmlifyString_w(ctx, __uwr_path_0))
              :
             ({
              uw_unit
              tmp;
              uw_error(ctx, FATAL, "tests/served_file/served_file.ur:12:0-17:0: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_unit
   __uwn_wrap_look_3(uw_context ctx, uw_Basis_string __uwr_x0_0, 
                      uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">"), 0),
                                                      (uw_begin_region(ctx), __uwn_check_2(ctx,
                                                                              __uwr_x0_0),
                                                       uw_end_region(ctx), (uw_write(ctx, 
                                                                            "</body>"), 0)))))));
   }
  
  static uw_unit
   __uwn_wrap_main_4(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\n<p>"), 0),
                                                      (uw_begin_region(ctx), __uwn_describe_1(ctx,
                                                                              uw_Basis_blessServedFile(ctx,
                                                                               "/hello.txt"
                                                                               )),
                                                       uw_end_region(ctx), ((uw_write(ctx, 
                                                                             "</p>\n<p>"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              __uwn_check_2(ctx,
                                                                               "/img/dot.png"),
                                                                             uw_end_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "</p>\n<p>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               __uwn_check_2(ctx,
                                                                               "/data"),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "</p>\n<p>"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               __uwn_check_2(ctx,
                                                                               "/nope"),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "</p>\n</body>"), 0)))))))))))));
   }
 
 static int uw_input_num(const char *name) {
 return -1;}
 
 static uw_periodic my_periodics[] = {{NULL}};
 
 static int uw_check_url(const char *s) {
  if (!strncmp(s, "#", 1)) return 1;
   if (!strcmp(s, "/hello.txt")) return 1;
    if (!strcmp(s, "/img/dot.png")) return 1;
     if (!strcmp(s, "/data")) return 1;
      if (!strcmp(s, "/Served_file/main")) return 1;
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
 {"/data", NULL, 14, "\x6E\x6F\x74\x20\x74\x65\x78\x74\x3A\x20\x00\x01\x02\x0A"},
  
  {"/hello.txt", "text/plain", 14, "\x48\x65\x6C\x6C\x6F\x2C\x20\x77\x6F\x72\x6C\x64\x21\x0A"},
  
  {"/img/dot.png", "image/png", 69, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A\x00\x00\x00\x0D\x49\x48\x44\x52\x00\x00\x00\x01\x00\x00\x00\x01\x08\x02\x00\x00\x00\x90\x77\x53\xDE\x00\x00\x00\x0C\x49\x44\x41\x54\x78\x9C\x63\xF8\xCF\xC0\x00\x00\x03\x01\x01\x00\xC9\xFE\x92\xEF\x00\x00\x00\x00\x49\x45\x4E\x44\xAE\x42\x60\x82"},
  {NULL, NULL, 0, NULL}};
 
 static void uw_handle(uw_context ctx, char *request) {
 uw_Basis_string ims = uw_Basis_requestHeader(ctx, "If-modified-since");
 if (ims && !strcmp(ims, "Thu, 01 Jan 1970 00:00:00 GMT")) {
 uw_clear_headers(ctx);
  uw_write_header(ctx, uw_supports_direct_status ? "HTTP/1.1 304 Not Modified\r\n" : "Status: 304 Not Modified\r\n");
  return;
  }
 
 
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Served_file/main", 17) && (request[17] == 0 || request[17] == '/')) {
  request += 17;
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
  
  if (!strncmp(request, "/Served_file/look", 17) && (request[17] == 0 || request[17] == '/')) {
   request += 17;
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
     __uwn_wrap_look_3(ctx, arg0, 0);
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
 