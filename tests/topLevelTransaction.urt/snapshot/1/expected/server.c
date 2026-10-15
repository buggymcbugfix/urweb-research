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
   uw_Basis_int __uwf_1;
    struct __uws_1* __uwf_2;
     };
  
  static struct __uws_1* __uwn_$revPRIME_unpoly_1(uw_context,
   struct __uws_1*, struct __uws_1*);
   static struct __uws_1*
    __uwn_$revPRIME_unpoly_1(uw_context ctx, struct __uws_1* __uwr_acc_0, 
                              struct __uws_1* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_1* rearg0 =
                    ({
                     struct __uws_1 *tmp =
                     uw_malloc(ctx, sizeof(struct __uws_1));
                     *tmp =
                     ({ struct __uws_1 tmp = {__uwr_x_2, __uwr_acc_0}; tmp; });
                     tmp;
                     });
                    struct __uws_1* rearg1 = __uwr_ls_3;
                   struct __uws_1*
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_1*
               tmp;
               uw_error(ctx, FATAL, "$/list.ur:47:21-50:13: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int __uwn_$lengthPRIME_unpoly_2(uw_context,
   uw_Basis_int, struct __uws_1*);
   static uw_Basis_int
    __uwn_$lengthPRIME_unpoly_2(uw_context ctx, uw_Basis_int __uwr_acc_0, 
                                 struct __uws_1* __uwr_ls_1)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_1;
            
            disc == NULL ? __uwr_acc_0
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr___2 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_ls_3 = (*disc).__uwf_2;
                  ({
                   uw_Basis_int rearg0 = __uwr_acc_0 + 1LL;
                    struct __uws_1* rearg1 = __uwr_ls_3;
                   uw_Basis_int
                   tmp;
                   __uwr_acc_0 = rearg0;
                    __uwr_ls_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "$/list.ur:72:24-75:16: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_job_3(uw_context ctx, uw_unit __uwr___0)
   {
   return(({
           struct __uws_1* __uwr_l_1 =
           ({
            uw_Basis_bool disc =
            1LL == 0LL;
            
            disc == uw_Basis_True ?
             ({
              struct __uws_1*
              tmp;
              uw_error(ctx, FATAL, "tests/topLevelTransaction.ur:6:30-6:55: %s", 
              "zero");
              tmp;
              })
              :
             disc == uw_Basis_False ?
              ({
               struct __uws_1* arg0 = NULL;
                
                struct __uws_1* arg1 =
                 ({
                  struct __uws_1 *tmp =
                  uw_malloc(ctx, sizeof(struct __uws_1));
                  *tmp = ({ struct __uws_1 tmp = {1LL, NULL}; tmp; });
                  tmp;
                  });
               __uwn_$revPRIME_unpoly_1(ctx, arg0, arg1);
               })
               :
              ({
               struct __uws_1*
               tmp;
               uw_error(ctx, FATAL, "tests/topLevelTransaction.ur:6:16-6:75: pattern match failure");
               tmp;
               });
            });
           ({
            uw_unit __uwr___2 =
            (uw_begin_region(ctx), uw_Basis_debug(ctx,
                                    uw_Basis_intToString(ctx,
                                     (({
                                       uw_Basis_int arg0 = 0LL;
                                        struct __uws_1* arg1 = __uwr_l_1;
                                       __uwn_$lengthPRIME_unpoly_2(ctx,
                                       arg0, arg1);
                                       })) +
                                      (({
                                        uw_Basis_int arg0 = 0LL;
                                         struct __uws_1* arg1 = __uwr_l_1;
                                        __uwn_$lengthPRIME_unpoly_2(ctx,
                                        arg0, arg1);
                                        })))));
            uw_end_region(ctx);
             0;
            });
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_4(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_unit __uwr___2 =
           (uw_begin_region(ctx), __uwn_job_3(ctx, 0));
           uw_end_region(ctx);
            ({
             uw_unit __uwr___3 =
             (uw_begin_region(ctx), __uwn_job_3(ctx, 0));
             uw_end_region(ctx);
              0;
             });
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
 
 
 
 if (!strncmp(request, "/TopLevelTransaction/main", 25) && (request[25] == 0 || request[25] == '/')) {
  request += 25;
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
 