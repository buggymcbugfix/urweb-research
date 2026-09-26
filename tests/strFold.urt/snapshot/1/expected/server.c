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
   uw_Basis_int __uwf_At;
    uw_Basis_int __uwf_Sum;
     };
  struct __uws_2 {
   struct __uws_1 __uwf_1;
    uw_Basis_bool __uwf_2;
     };
  struct __uws_3 {
   uw_Basis_int __uwf_1;
    uw_Basis_bool __uwf_2;
     };
  struct __uws_4 {
   uw_Basis_int __uwf_Letters;
    uw_Basis_int __uwf_Others;
     };
  struct __uws_5 {
   struct __uws_4 __uwf_1;
    uw_Basis_bool __uwf_2;
     };
  struct __uws_6 {
   uw_Basis_char* __uwf_1;
    uw_Basis_bool __uwf_2;
     };
  
  static char jslib[] = "*runtime elided*";
   static char jsapp[] = "*script elided*";
  
  static uw_Basis_int*
   __uwn_sum_1672(uw_context ctx, uw_Basis_string __uwr_s_0)
   {
   return(({
           struct __uws_2 disc =
           ({
            uw_Basis_string __uwfold_s_1 = __uwr_s_0;
            struct __uws_1 __uwfold_acc_1 = ({ struct __uws_1 tmp = {0LL, 0LL};
                                             tmp; });
            int32_t __uwfold_i_1 = 0;
            uw_Basis_bool __uwfold_go_1 = uw_Basis_True;
            while (__uwfold_s_1[__uwfold_i_1]) {
            uw_Basis_char __uwr_c_1;
            U8_NEXT(__uwfold_s_1, __uwfold_i_1, -1, __uwr_c_1);
            struct __uws_1 __uwr_acc_2 = __uwfold_acc_1;
            struct __uws_2 __uwfold_r_1 = ({
                                           uw_Basis_bool disc =
                                           uw_Basis_isdigit(ctx, __uwr_c_1);
                                           
                                           disc == uw_Basis_True ?
                                            ({ struct __uws_2 tmp =
                                             {({ struct __uws_1 tmp =
                                               {__uwr_acc_2.__uwf_At + 1LL, 
                                                 (__uwr_acc_2.__uwf_Sum +
                                                   uw_Basis_ord(ctx, __uwr_c_1))
                                                  - 48LL}; tmp; }), 
                                               uw_Basis_True}; tmp; })
                                             :
                                            disc == uw_Basis_False ?
                                             ({ struct __uws_2 tmp =
                                              {__uwr_acc_2, uw_Basis_False};
                                              tmp; })
                                              :
                                             ({
                                              struct __uws_2
                                              tmp;
                                              uw_error(ctx, FATAL, "tests/strFold.ur:14:22-14:34: pattern match failure");
                                              tmp;
                                              });
                                           });
            __uwfold_acc_1 = __uwfold_r_1.__uwf_1;
            if (__uwfold_r_1.__uwf_2 == uw_Basis_False) {
            __uwfold_go_1 = uw_Basis_False;
            break;
            }
            }
            ({ struct __uws_2 tmp = {__uwfold_acc_1, __uwfold_go_1}; tmp; });
            });
           
           1 && 1 ?
            ({struct __uws_1 __uwr_r_1 = disc.__uwf_1;
               uw_Basis_bool __uwr_ok_2 = disc.__uwf_2;
                ({
                 uw_Basis_bool disc =
                 __uwr_ok_2;
                 
                 disc == uw_Basis_True ?
                  ({
                   uw_Basis_int *tmp =
                   uw_malloc(ctx, sizeof(uw_Basis_int));
                   *tmp = __uwr_r_1.__uwf_Sum;
                   tmp;
                   })
                   :
                  disc == uw_Basis_False ?
                   ({
                    uw_Basis_int *tmp =
                    uw_malloc(ctx, sizeof(uw_Basis_int));
                    *tmp = - __uwr_r_1.__uwf_At;
                    tmp;
                    })
                    :
                   ({
                    uw_Basis_int*
                    tmp;
                    uw_error(ctx, FATAL, "tests/strFold.ur:19:8-19:47: pattern match failure");
                    tmp;
                    });
                 });
             })
             :
            ({
             uw_Basis_int*
             tmp;
             uw_error(ctx, FATAL, "tests/strFold.ur:14:8-17:54: pattern match failure");
             tmp;
             });
           }));
   }
  
  static uw_Basis_int __uwn_deep_1674(uw_context, uw_Basis_char, uw_Basis_int);
   static uw_Basis_int
    __uwn_deep_1674(uw_context ctx, uw_Basis_char __uwr_ch_0, 
                     uw_Basis_int __uwr_n_1)
    {restart:
      
    return(({
            uw_Basis_bool disc =
            __uwr_ch_0 == '(';
            
            disc == uw_Basis_True ?
             ({
              uw_Basis_char rearg0 = ')';
               uw_Basis_int rearg1 = __uwr_n_1 + 1LL;
              uw_Basis_int
              tmp;
              __uwr_ch_0 = rearg0;
               __uwr_n_1 = rearg1;
              goto restart;
              tmp;
              })
              :
             disc == uw_Basis_False ? __uwr_n_1
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "tests/strFold.ur:28:39-28:81: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_wrap_main_1680(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_src_2 =
           uw_Basis_new_client_source(ctx,
            ({
             uw_Basis_string arg0 = "{c:\"c\",v:";
              uw_Basis_string arg1 = uw_Basis_jsifyString(ctx, "");
               uw_Basis_string arg2 = "}";
                uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
             }));
           ((uw_write(ctx, "<body"), 0),
            (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                   uw_Basis_get_settings(ctx,
                                                    0))), 0),
             uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                        "")), 0),
                                  uw_end_region(ctx), ((uw_write(ctx, ">\n"), 0),
                                                       (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx,
                                                                               ({
                                                                               uw_Basis_string __uwfold_s_3 = 
                                                                               "a1b22c333";
                                                                               uw_Basis_int __uwfold_acc_3 = 0LL;
                                                                               int32_t __uwfold_i_3 = 0;
                                                                               uw_Basis_bool __uwfold_go_3 = uw_Basis_True;
                                                                               while (__uwfold_s_3[__uwfold_i_3]) {
                                                                               uw_Basis_char __uwr_c_3;
                                                                               U8_NEXT(__uwfold_s_3, __uwfold_i_3, -1, __uwr_c_3);
                                                                               uw_Basis_int __uwr_acc_4 = __uwfold_acc_3;
                                                                               struct
                                                                               __uws_3 __uwfold_r_3 = 
                                                                               ({
                                                                               struct
                                                                               __uws_3
                                                                               tmp
                                                                               =
                                                                               {({
                                                                               uw_Basis_bool
                                                                               disc
                                                                               =
                                                                               uw_Basis_isdigit(ctx,
                                                                               __uwr_c_3
                                                                               );
                                                                               
                                                                               disc
                                                                               
                                                                               ==
                                                                               uw_Basis_True
                                                                               
                                                                               ?
                                                                               __uwr_acc_4
                                                                               +
                                                                               1LL
                                                                               
                                                                               :
                                                                               disc
                                                                               
                                                                               ==
                                                                               uw_Basis_False
                                                                               
                                                                               ?
                                                                               __uwr_acc_4
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_int
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "tests/strFold.ur:9:52-9:83: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               }),
                                                                               
                                                                               uw_Basis_True};
                                                                               tmp;
                                                                               });
                                                                               __uwfold_acc_3 = __uwfold_r_3.__uwf_1;
                                                                               if (__uwfold_r_3.__uwf_2 == uw_Basis_False) {
                                                                               __uwfold_go_3 = uw_Basis_False;
                                                                               break;
                                                                               }
                                                                               }
                                                                               ({ 
                                                                               struct
                                                                               __uws_3 tmp = {__uwfold_acc_3, __uwfold_go_3}; tmp; });
                                                                               }).__uwf_1
                                                                               ),
                                                        uw_end_region(ctx), ((uw_write(ctx, 
                                                                              " "), 0),
                                                                             (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_Basis_int*
                                                                               disc
                                                                               =
                                                                               __uwn_sum_1672(ctx,
                                                                               "123");
                                                                               
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
                                                                               uw_Basis_int
                                                                               __uwr_x_3
                                                                               =
                                                                               (*disc);
                                                                               uw_Basis_intToString(ctx,
                                                                               __uwr_x_3
                                                                               );
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:98:10-98:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               })
                                                                               ),
                                                                              uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyString_w(ctx,
                                                                               ({
                                                                               uw_Basis_int*
                                                                               disc
                                                                               =
                                                                               __uwn_sum_1672(ctx,
                                                                               "12x4");
                                                                               
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
                                                                               uw_Basis_int
                                                                               __uwr_x_3
                                                                               =
                                                                               (*disc);
                                                                               uw_Basis_intToString(ctx,
                                                                               __uwr_x_3
                                                                               );
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               uw_Basis_string
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "$/top.ur:98:10-98:18: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               })
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               ({
                                                                               uw_Basis_string __uwfold_s_3 = 
                                                                               "ab, cd";
                                                                               struct
                                                                               __uws_4 __uwfold_acc_3 = 
                                                                               ({
                                                                               struct
                                                                               __uws_4
                                                                               tmp
                                                                               =
                                                                               {0LL
                                                                               ,
                                                                               
                                                                               0LL};
                                                                               tmp;
                                                                               });
                                                                               int32_t __uwfold_i_3 = 0;
                                                                               uw_Basis_bool __uwfold_go_3 = uw_Basis_True;
                                                                               while (__uwfold_s_3[__uwfold_i_3]) {
                                                                               uw_Basis_char __uwr_c_3;
                                                                               U8_NEXT(__uwfold_s_3, __uwfold_i_3, -1, __uwr_c_3);
                                                                               struct
                                                                               __uws_4 __uwr_acc_4 = __uwfold_acc_3;
                                                                               struct
                                                                               __uws_5 __uwfold_r_3 = 
                                                                               ({
                                                                               struct
                                                                               __uws_5
                                                                               tmp
                                                                               =
                                                                               {({
                                                                               uw_Basis_bool
                                                                               disc
                                                                               =
                                                                               uw_Basis_isalpha(ctx,
                                                                               __uwr_c_3
                                                                               );
                                                                               
                                                                               disc
                                                                               
                                                                               ==
                                                                               uw_Basis_True
                                                                               
                                                                               ?
                                                                               ({
                                                                               struct
                                                                               __uws_4
                                                                               tmp
                                                                               =
                                                                               {__uwr_acc_4.__uwf_Letters
                                                                               +
                                                                               1LL
                                                                               ,
                                                                               
                                                                               __uwr_acc_4.__uwf_Others};
                                                                               tmp;
                                                                               })
                                                                               
                                                                               :
                                                                               disc
                                                                               
                                                                               ==
                                                                               uw_Basis_False
                                                                               
                                                                               ?
                                                                               ({
                                                                               struct
                                                                               __uws_4
                                                                               tmp
                                                                               =
                                                                               {__uwr_acc_4.__uwf_Letters
                                                                               ,
                                                                               
                                                                               __uwr_acc_4.__uwf_Others
                                                                               +
                                                                               1LL};
                                                                               tmp;
                                                                               })
                                                                               
                                                                               :
                                                                               ({
                                                                               struct
                                                                               __uws_4
                                                                               tmp;
                                                                               uw_error(ctx, FATAL, "tests/strFold.ur:23:24-24:73: pattern match failure");
                                                                               tmp;
                                                                               });
                                                                               }),
                                                                               
                                                                               uw_Basis_True};
                                                                               tmp;
                                                                               });
                                                                               __uwfold_acc_3 = __uwfold_r_3.__uwf_1;
                                                                               if (__uwfold_r_3.__uwf_2 == uw_Basis_False) {
                                                                               __uwfold_go_3 = uw_Basis_False;
                                                                               break;
                                                                               }
                                                                               }
                                                                               ({ 
                                                                               struct
                                                                               __uws_5 tmp = {__uwfold_acc_3, __uwfold_go_3}; tmp; });
                                                                               }).__uwf_1.__uwf_Letters
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               ({
                                                                               uw_Basis_string __uwfold_s_3 = 
                                                                               "((a)";
                                                                               uw_Basis_int __uwfold_acc_3 = 0LL;
                                                                               int32_t __uwfold_i_3 = 0;
                                                                               uw_Basis_bool __uwfold_go_3 = uw_Basis_True;
                                                                               while (__uwfold_s_3[__uwfold_i_3]) {
                                                                               uw_Basis_char __uwr_c_3;
                                                                               U8_NEXT(__uwfold_s_3, __uwfold_i_3, -1, __uwr_c_3);
                                                                               uw_Basis_int __uwr_acc_4 = __uwfold_acc_3;
                                                                               struct
                                                                               __uws_3 __uwfold_r_3 = 
                                                                               ({
                                                                               struct
                                                                               __uws_3
                                                                               tmp
                                                                               =
                                                                               {({
                                                                               uw_Basis_char
                                                                               arg0
                                                                               =
                                                                               __uwr_c_3;
                                                                               
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               __uwr_acc_4;
                                                                               __uwn_deep_1674(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               }),
                                                                               
                                                                               uw_Basis_True};
                                                                               tmp;
                                                                               });
                                                                               __uwfold_acc_3 = __uwfold_r_3.__uwf_1;
                                                                               if (__uwfold_r_3.__uwf_2 == uw_Basis_False) {
                                                                               __uwfold_go_3 = uw_Basis_False;
                                                                               break;
                                                                               }
                                                                               }
                                                                               ({ 
                                                                               struct
                                                                               __uws_3 tmp = {__uwfold_acc_3, __uwfold_go_3}; tmp; });
                                                                               }).__uwf_1
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               " "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyBool_w(ctx,
                                                                               ({
                                                                               uw_Basis_string __uwfold_s_3 = 
                                                                               "ab!c";
                                                                               uw_Basis_char* __uwfold_acc_3 = NULL;
                                                                               int32_t __uwfold_i_3 = 0;
                                                                               uw_Basis_bool __uwfold_go_3 = uw_Basis_True;
                                                                               while (__uwfold_s_3[__uwfold_i_3]) {
                                                                               uw_Basis_char __uwr_c_3;
                                                                               U8_NEXT(__uwfold_s_3, __uwfold_i_3, -1, __uwr_c_3);
                                                                               uw_Basis_char* __uwr_acc_4 = __uwfold_acc_3;
                                                                               struct
                                                                               __uws_6 __uwfold_r_3 = 
                                                                               ({
                                                                               struct
                                                                               __uws_6
                                                                               tmp
                                                                               =
                                                                               {({
                                                                               uw_Basis_char
                                                                               *tmp
                                                                               =
                                                                               uw_malloc(ctx, sizeof(
                                                                               uw_Basis_char));
                                                                               *tmp
                                                                               =
                                                                               __uwr_c_3;
                                                                               tmp;
                                                                               }),
                                                                               
                                                                               !
                                                                               (__uwr_c_3
                                                                               ==
                                                                               '!')};
                                                                               tmp;
                                                                               });
                                                                               __uwfold_acc_3 = __uwfold_r_3.__uwf_1;
                                                                               if (__uwfold_r_3.__uwf_2 == uw_Basis_False) {
                                                                               __uwfold_go_3 = uw_Basis_False;
                                                                               break;
                                                                               }
                                                                               }
                                                                               ({ 
                                                                               struct
                                                                               __uws_6 tmp = {__uwfold_acc_3, __uwfold_go_3}; tmp; });
                                                                               }).__uwf_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "\n <script type=\"text/javascript\">var d=inp(exec("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_src_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "));</script>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_src_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))))))))))))));
           }));
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
  
  
  if (!strcmp(request, "/app.48DE9027C4B19B356B2B0E62ECDABE59A73A0011.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 
 if (!strncmp(request, "/StrFold/main", 13) && (request[13] == 0 || request[13] == '/')) {
  request += 13;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.5176DF5A2DB34FEB6231025A1D7D8D4A059FC48D.js\"></script>\n<script type=\"text/javascript\" src=\"/app.48DE9027C4B19B356B2B0E62ECDABE59A73A0011.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_1680(ctx, arg0, 0);
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
                                                                  NULL};
 