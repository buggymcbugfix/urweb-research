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
  
  struct __uws_2 {
   uw_Basis_string __uwf_X;
    };
  
  struct __uws_3 {
   uw_Basis_string __uwf_1;
    struct __uws_3* __uwf_2;
     };
  
  
  static uw_unit __uwn_toXml_4(uw_context, struct __uws_1*);
   static uw_unit __uwn_toXml_4(uw_context ctx, struct __uws_1* __uwr_ls_0)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_0;
            
            disc == NULL ? (uw_write(ctx, "[]"), 0)
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_1 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_lsPRIME_2 = (*disc).__uwf_2;
                  (uw_begin_region(ctx), uw_Basis_htmlifyInt_w(ctx, __uwr_x_1),
                   uw_end_region(ctx), ((uw_write(ctx, " :: "), 0),
                                        ({
                                         struct __uws_1* rearg0 =
                                          __uwr_lsPRIME_2;
                                         uw_unit
                                         tmp;
                                         __uwr_ls_0 = rearg0;
                                         goto restart;
                                         tmp;
                                         })));
               })
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, "demo/listFun.ur:8:4-13:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static struct __uws_1* __uwn_$revPRIME_unpoly_5(uw_context,
   struct __uws_1*, struct __uws_1*);
   static struct __uws_1*
    __uwn_$revPRIME_unpoly_5(uw_context ctx, struct __uws_1* __uwr_ls_0, 
                              struct __uws_1* __uwr_acc_1)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_0;
            
            disc == NULL ? __uwr_acc_1
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_lsPRIME_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_1* rearg0 = __uwr_lsPRIME_3;
                    
                    struct __uws_1* rearg1 =
                     ({
                      struct __uws_1 *tmp =
                      uw_malloc(ctx, sizeof(struct __uws_1));
                      *tmp =
                      ({ struct __uws_1 tmp = {__uwr_x_2, __uwr_acc_1}; tmp; });
                      tmp;
                      });
                   struct __uws_1*
                   tmp;
                   __uwr_ls_0 = rearg0;
                    __uwr_acc_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_1*
               tmp;
               uw_error(ctx, FATAL, "demo/list.ur:16:12-19:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int __uwn_$lengthPRIME_unpoly_6(uw_context,
   struct __uws_1*, uw_Basis_int);
   static uw_Basis_int
    __uwn_$lengthPRIME_unpoly_6(uw_context ctx, struct __uws_1* __uwr_ls_0, 
                                 uw_Basis_int __uwr_acc_1)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_ls_0;
            
            disc == NULL ? __uwr_acc_1
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_int __uwr___2 = (*disc).__uwf_1;
                 struct __uws_1* __uwr_lsPRIME_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_1* rearg0 = __uwr_lsPRIME_3;
                    uw_Basis_int rearg1 = __uwr_acc_1 + 1LL;
                   uw_Basis_int
                   tmp;
                   __uwr_ls_0 = rearg0;
                    __uwr_acc_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "demo/list.ur:6:12-9:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_urlify_list_s_7(uw_context, struct __uws_1*);
   static uw_unit
    __uwn_urlify_list_s_7(uw_context ctx, struct __uws_1* __uwr_x_0)
    {restart:
      
    return(({
            struct __uws_1* disc =
            __uwr_x_0;
            
            disc == NULL ? (uw_write(ctx, "Nil"), 0)
              :
             disc != NULL && 1 ?
              ({struct __uws_1 __uwr_a_1 = (*disc);
                 ((uw_write(ctx, "Cons/"), 0),
                  (uw_begin_region(ctx), uw_Basis_urlifyInt_w(ctx,
                                          __uwr_a_1.__uwf_1),
                   uw_end_region(ctx), ((uw_write(ctx, "/"), 0),
                                        ({
                                         struct __uws_1* rearg0 =
                                          __uwr_a_1.__uwf_2;
                                         uw_unit
                                         tmp;
                                         __uwr_x_0 = rearg0;
                                         goto restart;
                                         tmp;
                                         }))));
               })
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_console_8(uw_context ctx, struct __uws_1* __uwr_ls_0, 
                    uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\nCurrent list: "), 0),
                                                      (uw_begin_region(ctx), __uwn_toXml_4(ctx,
                                                                              __uwr_ls_0),
                                                       uw_end_region(ctx), ((uw_write(ctx, 
                                                                             "<br />\nReversed list: "), 0),
                                                                            (uw_begin_region(ctx),
                                                                              __uwn_toXml_4(ctx,
                                                                               ({
                                                                               struct
                                                                               __uws_1*
                                                                               arg0
                                                                               =
                                                                               __uwr_ls_0;
                                                                               
                                                                               struct
                                                                               __uws_1*
                                                                               arg1
                                                                               =
                                                                               NULL;
                                                                               __uwn_$revPRIME_unpoly_5(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               })),
                                                                             uw_end_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "<br />\nLength: "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               ({
                                                                               struct
                                                                               __uws_1*
                                                                               arg0
                                                                               =
                                                                               __uwr_ls_0;
                                                                               
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               0LL;
                                                                               __uwn_$lengthPRIME_unpoly_6(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               })
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "<br />\n<br />\n<form method=\"post\" action=\"/ListShop/IL/cons/"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               __uwn_urlify_list_s_7(ctx,
                                                                               __uwr_ls_0),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\">\nAdd element: <input type=\"text\" name=\"X\" /> <input type=\"submit\" />\n</form>\n</body>"), 0)))))))))))));
   }
  
  static uw_unit
   __uwn_wrap_$cons_9(uw_context ctx, struct __uws_1* __uwr_x1_0, 
                       struct __uws_2 __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           uw_Basis_int* disc =
           uw_Basis_stringToInt(ctx, __uwr_x0_1.__uwf_X);
           
           disc == NULL ?
            ((uw_write(ctx, "<body"), 0),
             (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                    uw_Basis_get_settings(ctx,
                                                     0))), 0),
              uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                         "")), 0),
                                   uw_end_region(ctx), (uw_write(ctx, ">Invalid string!</body>"), 0))))
             :
            disc != NULL && 1 ?
             ({uw_Basis_int __uwr_v_3 = (*disc);
                ({
                 struct __uws_1* arg0 =
                  ({
                   struct __uws_1 *tmp =
                   uw_malloc(ctx, sizeof(struct __uws_1));
                   *tmp =
                   ({ struct __uws_1 tmp = {__uwr_v_3, __uwr_x1_0}; tmp; });
                   tmp;
                   });
                  uw_unit arg1 = 0;
                 __uwn_console_8(ctx, arg0, arg1);
                 });
              })
              :
             ({
              uw_unit
              tmp;
              uw_error(ctx, FATAL, "demo/listFun.ur:13:4-30:11: pattern match failure");
              tmp;
              });
           }));
   }
  
  static uw_unit __uwn_toXml_13(uw_context, struct __uws_3*);
   static uw_unit __uwn_toXml_13(uw_context ctx, struct __uws_3* __uwr_ls_0)
    {restart:
      
    return(({
            struct __uws_3* disc =
            __uwr_ls_0;
            
            disc == NULL ? (uw_write(ctx, "[]"), 0)
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_string __uwr_x_1 = (*disc).__uwf_1;
                 struct __uws_3* __uwr_lsPRIME_2 = (*disc).__uwf_2;
                  (uw_begin_region(ctx), uw_Basis_htmlifyString_w(ctx,
                                          __uwr_x_1),
                   uw_end_region(ctx), ((uw_write(ctx, " :: "), 0),
                                        ({
                                         struct __uws_3* rearg0 =
                                          __uwr_lsPRIME_2;
                                         uw_unit
                                         tmp;
                                         __uwr_ls_0 = rearg0;
                                         goto restart;
                                         tmp;
                                         })));
               })
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, "demo/listFun.ur:8:4-13:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static struct __uws_3* __uwn_$revPRIME_unpoly_14(uw_context,
   struct __uws_3*, struct __uws_3*);
   static struct __uws_3*
    __uwn_$revPRIME_unpoly_14(uw_context ctx, struct __uws_3* __uwr_ls_0, 
                               struct __uws_3* __uwr_acc_1)
    {restart:
      
    return(({
            struct __uws_3* disc =
            __uwr_ls_0;
            
            disc == NULL ? __uwr_acc_1
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_string __uwr_x_2 = (*disc).__uwf_1;
                 struct __uws_3* __uwr_lsPRIME_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_3* rearg0 = __uwr_lsPRIME_3;
                    
                    struct __uws_3* rearg1 =
                     ({
                      struct __uws_3 *tmp =
                      uw_malloc(ctx, sizeof(struct __uws_3));
                      *tmp =
                      ({ struct __uws_3 tmp = {__uwr_x_2, __uwr_acc_1}; tmp; });
                      tmp;
                      });
                   struct __uws_3*
                   tmp;
                   __uwr_ls_0 = rearg0;
                    __uwr_acc_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               struct __uws_3*
               tmp;
               uw_error(ctx, FATAL, "demo/list.ur:16:12-19:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_Basis_int __uwn_$lengthPRIME_unpoly_15(uw_context,
   struct __uws_3*, uw_Basis_int);
   static uw_Basis_int
    __uwn_$lengthPRIME_unpoly_15(uw_context ctx, struct __uws_3* __uwr_ls_0, 
                                  uw_Basis_int __uwr_acc_1)
    {restart:
      
    return(({
            struct __uws_3* disc =
            __uwr_ls_0;
            
            disc == NULL ? __uwr_acc_1
              :
             disc != NULL && 1 && 1 ?
              ({uw_Basis_string __uwr___2 = (*disc).__uwf_1;
                 struct __uws_3* __uwr_lsPRIME_3 = (*disc).__uwf_2;
                  ({
                   struct __uws_3* rearg0 = __uwr_lsPRIME_3;
                    uw_Basis_int rearg1 = __uwr_acc_1 + 1LL;
                   uw_Basis_int
                   tmp;
                   __uwr_ls_0 = rearg0;
                    __uwr_acc_1 = rearg1;
                   goto restart;
                   tmp;
                   });
               })
               :
              ({
               uw_Basis_int
               tmp;
               uw_error(ctx, FATAL, "demo/list.ur:6:12-9:4: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit __uwn_urlify_list_s_16(uw_context, struct __uws_3*);
   static uw_unit
    __uwn_urlify_list_s_16(uw_context ctx, struct __uws_3* __uwr_x_0)
    {restart:
      
    return(({
            struct __uws_3* disc =
            __uwr_x_0;
            
            disc == NULL ? (uw_write(ctx, "Nil"), 0)
              :
             disc != NULL && 1 ?
              ({struct __uws_3 __uwr_a_1 = (*disc);
                 ((uw_write(ctx, "Cons/"), 0),
                  (uw_begin_region(ctx), uw_Basis_urlifyString_w(ctx,
                                          __uwr_a_1.__uwf_1),
                   uw_end_region(ctx), ((uw_write(ctx, "/"), 0),
                                        ({
                                         struct __uws_3* rearg0 =
                                          __uwr_a_1.__uwf_2;
                                         uw_unit
                                         tmp;
                                         __uwr_x_0 = rearg0;
                                         goto restart;
                                         tmp;
                                         }))));
               })
               :
              ({
               uw_unit
               tmp;
               uw_error(ctx, FATAL, ":0:0-0:0: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_console_17(uw_context ctx, struct __uws_3* __uwr_ls_0, 
                     uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), ((uw_write(ctx, ">\nCurrent list: "), 0),
                                                      (uw_begin_region(ctx), __uwn_toXml_13(ctx,
                                                                              __uwr_ls_0),
                                                       uw_end_region(ctx), ((uw_write(ctx, 
                                                                             "<br />\nReversed list: "), 0),
                                                                            (uw_begin_region(ctx),
                                                                              __uwn_toXml_13(ctx,
                                                                               ({
                                                                               struct
                                                                               __uws_3*
                                                                               arg0
                                                                               =
                                                                               __uwr_ls_0;
                                                                               
                                                                               struct
                                                                               __uws_3*
                                                                               arg1
                                                                               =
                                                                               NULL;
                                                                               __uwn_$revPRIME_unpoly_14(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               })),
                                                                             uw_end_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "<br />\nLength: "), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifyInt_w(ctx,
                                                                               ({
                                                                               struct
                                                                               __uws_3*
                                                                               arg0
                                                                               =
                                                                               __uwr_ls_0;
                                                                               
                                                                               uw_Basis_int
                                                                               arg1
                                                                               =
                                                                               0LL;
                                                                               __uwn_$lengthPRIME_unpoly_15(ctx,
                                                                               arg0
                                                                               ,
                                                                               
                                                                               arg1);
                                                                               })
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "<br />\n<br />\n<form method=\"post\" action=\"/ListShop/SL/cons/"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               __uwn_urlify_list_s_16(ctx,
                                                                               __uwr_ls_0),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "\">\nAdd element: <input type=\"text\" name=\"X\" /> <input type=\"submit\" />\n</form>\n</body>"), 0)))))))))))));
   }
  
  static uw_unit
   __uwn_wrap_$cons_18(uw_context ctx, struct __uws_3* __uwr_x1_0, 
                        struct __uws_2 __uwr_x0_1, uw_unit __uwr___2)
   {
   return(({
           struct __uws_3* arg0 =
            ({
             struct __uws_3 *tmp = uw_malloc(ctx, sizeof(struct __uws_3));
             *tmp =
             ({ struct __uws_3 tmp = {__uwr_x0_1.__uwf_X, __uwr_x1_0}; tmp; });
             tmp;
             });
            uw_unit arg1 = 0;
           __uwn_console_17(ctx, arg0, arg1);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_19(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_3* arg0 = NULL;
            uw_unit arg1 = 0;
           __uwn_console_17(ctx, arg0, arg1);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_20(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_1* arg0 = NULL;
            uw_unit arg1 = 0;
           __uwn_console_8(ctx, arg0, arg1);
           }));
   }
  
  static uw_unit
   __uwn_wrap_main_21(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(((uw_write(ctx, "<body"), 0),
           (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                  uw_Basis_get_settings(ctx, 0))), 0),
            uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                       "")), 0),
                                 uw_end_region(ctx), (uw_write(ctx, ">\nPick your poison:<br />\n<ul>\n<li> <a href=\"/ListShop/IL/main\">Integers</a></li>\n<li> <a href=\"/ListShop/SL/main\">Strings</a></li>\n</ul>\n</body>"), 0)))));
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
 
 static struct __uws_3 *unurlify_10(uw_context, char **);
  static struct __uws_1 *unurlify_1(uw_context, char **);
   static struct __uws_3
    *unurlify_10(uw_context ctx, char **request) {
    return ((*request)[0] == '/' ? ++*request : *request,
     ((!strncmp(*request, "Nil", 3) && ((*request)[3] == 0 || (*request)[3] == '/')) ? (*request
     +=
     3, NULL) : ((!strncmp(*request, "Cons", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? (*request
     +=
     4, ((*request)[0] == '/' ? ++*request : NULL), 
     ({
      struct __uws_3 *tmp = uw_malloc(ctx, sizeof(struct __uws_3));
      *tmp =
      ({
       uw_Basis_string uwr_1 = uw_Basis_unurlifyString(ctx, request);
        struct __uws_3* uwr_2 = unurlify_10(ctx, request);
         struct __uws_3 tmp = { uwr_1, uwr_2 };
       tmp;
       });
      tmp;
      }))
     : (uw_error(ctx, FATAL, "Error unurlifying datatype list_s"), NULL))));
     }
    
    static struct __uws_1
     *unurlify_1(uw_context ctx, char **request) {
     return ((*request)[0] == '/' ? ++*request : *request,
      ((!strncmp(*request, "Nil", 3) && ((*request)[3] == 0 || (*request)[3] == '/')) ? (*request
      +=
      3, NULL) : ((!strncmp(*request, "Cons", 4) && ((*request)[4] == 0 || (*request)[4] == '/')) ? (*request
      +=
      4, ((*request)[0] == '/' ? ++*request : NULL), 
      ({
       struct __uws_1 *tmp = uw_malloc(ctx, sizeof(struct __uws_1));
       *tmp =
       ({
        uw_Basis_int uwr_1 = uw_Basis_unurlifyInt(ctx, request);
         struct __uws_1* uwr_2 = unurlify_1(ctx, request);
          struct __uws_1 tmp = { uwr_1, uwr_2 };
        tmp;
        });
       tmp;
       }))
      : (uw_error(ctx, FATAL, "Error unurlifying datatype list_s"), NULL))));
      }
     
     static void uw_handle(uw_context ctx, char *request) {
 uw_Basis_string ims = uw_Basis_requestHeader(ctx, "If-modified-since");
 if (ims && !strcmp(ims, "Thu, 01 Jan 1970 00:00:00 GMT")) {
 uw_clear_headers(ctx);
  uw_write_header(ctx, uw_supports_direct_status ? "HTTP/1.1 304 Not Modified\r\n" : "Status: 304 Not Modified\r\n");
  return;
  }
 
 
 
 if (!strncmp(request, "/ListShop/main", 14) && (request[14] == 0 || request[14] == '/')) {
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
    __uwn_wrap_main_21(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/ListShop/IL/main", 17) && (request[17] == 0 || request[17] == '/')) {
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
     __uwn_wrap_main_20(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/ListShop/SL/main", 17) && (request[17] == 0 || request[17] == '/')) {
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
     __uwn_wrap_main_19(ctx, arg0, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/ListShop/SL/cons", 17) && (request[17] == 0 || request[17] == '/')) {
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
    struct __uws_3* arg0 = unurlify_10(ctx, &request);
     uw_Basis_string uw_input_X;
      
      request = uw_get_input(ctx, 0);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input X");
       uw_input_X = uw_Basis_unurlifyString_fromClient(ctx, &request);
       struct __uws_2 uw_inputs = {
        uw_input_X,
         };
      __uwn_wrap_$cons_18(ctx, arg0, uw_inputs, 0);
    uw_write(ctx, "</html>");
     return;
    }
    }
  
  if (!strncmp(request, "/ListShop/IL/cons", 17) && (request[17] == 0 || request[17] == '/')) {
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
    struct __uws_1* arg0 = unurlify_1(ctx, &request);
     uw_Basis_string uw_input_X;
      
      request = uw_get_input(ctx, 0);
       if (request == NULL)
       uw_error(ctx, FATAL, "Missing input X");
       uw_input_X = uw_Basis_unurlifyString_fromClient(ctx, &request);
       struct __uws_2 uw_inputs = {
        uw_input_X,
         };
      __uwn_wrap_$cons_9(ctx, arg0, uw_inputs, 0);
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
 