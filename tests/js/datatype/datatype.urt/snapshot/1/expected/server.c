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
  
 enum __uwe_color_5 { __uwc_Red_6, __uwc_Green_7, __uwc_Blue_8 };
  
  
  struct __uws_1 {
   uw_Basis_int __uwf_H;
    uw_Basis_int __uwf_W;
     };
  
  enum __uwe_shape_1 { __uwc_Dot_2, __uwc_Circle_3, __uwc_Box_4 };
   
   struct __uwd_shape_1 {
   enum __uwe_shape_1
   tag;
   union {
    uw_Basis_int uw_Circle;
     struct __uws_1 uw_Box;
    } data;
    };
  
  
  
  
  
  struct __uws_2
   {
   struct __uws_2* __uwf_L;
    struct __uws_2* __uwf_R;
     uw_Basis_int __uwf_V;
      };
  
  
  
  
  
   enum __uwe_odd_18 { __uwc_O_19 };
    
    struct __uwd_odd_18 {
    enum __uwe_odd_18 tag;
    union {
     struct __uwd_odd_18* uw_O;
     } data;
     };
  struct __uws_3 {
   struct __uwd_shape_1* __uwf_1;
    struct __uws_3* __uwf_2;
     };
  
  struct __uws_4
   {
   enum __uwe_color_5 __uwf_1;
    uw_Basis_int* __uwf_2;
     struct __uws_2* __uwf_3;
      struct __uwd_odd_18* __uwf_4;
       };
  
  static char jslib[] = "*runtime elided*";
   static char jsapp[] = "*script elided*";
  
  static struct __uws_3*
   __uwn_shapes_20(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_3 *tmp = uw_malloc(ctx, sizeof(struct __uws_3));
           *tmp =
           ({ struct __uws_3 tmp =
            {({
              struct __uwd_shape_1 *tmp =
              uw_malloc(ctx, sizeof(struct __uwd_shape_1));
              tmp->tag = __uwc_Circle_3;
              tmp->data.uw_Circle = 1LL;
               tmp;
              }), 
              ({
               struct __uws_3 *tmp =
               uw_malloc(ctx, sizeof(struct __uws_3));
               *tmp =
               ({ struct __uws_3 tmp =
                {({
                  struct __uwd_shape_1 *tmp =
                  uw_malloc(ctx, sizeof(struct __uwd_shape_1));
                  tmp->tag =
                  __uwc_Box_4;
                  tmp->data.uw_Box =
                   ({ struct __uws_1 tmp = {3LL, 2LL}; tmp; });
                   tmp;
                  }), 
                  ({
                   struct __uws_3 *tmp =
                   uw_malloc(ctx, sizeof(struct __uws_3));
                   *tmp =
                   ({ struct __uws_3 tmp =
                    {({
                      struct __uwd_shape_1 *tmp =
                      uw_malloc(ctx, sizeof(struct __uwd_shape_1));
                      tmp->tag = __uwc_Dot_2;
                      tmp;
                      }), NULL}; tmp; });
                   tmp;
                   })}; tmp; });
               tmp;
               })}; tmp; });
           tmp;
           }));
   }
  
  static struct __uws_4
   __uwn_rest_21(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(({ struct __uws_4 tmp =
           {__uwc_Green_7, 
             ({
              uw_Basis_int *tmp = uw_malloc(ctx, sizeof(uw_Basis_int));
              *tmp = 7LL;
              tmp;
              }), 
             ({
              struct __uws_2 *tmp = uw_malloc(ctx, sizeof(struct __uws_2));
              *tmp =
              ({ struct __uws_2 tmp =
               {NULL, 
                 ({
                  struct __uws_2 *tmp =
                  uw_malloc(ctx, sizeof(struct __uws_2));
                  *tmp =
                  ({ struct __uws_2 tmp = {NULL, NULL, 2LL}; tmp; });
                  tmp;
                  }), 1LL}; tmp; });
              tmp;
              }), 
             ({
              struct __uwd_odd_18 *tmp =
              uw_malloc(ctx, sizeof(struct __uwd_odd_18));
              tmp->tag =
              __uwc_O_19;
              tmp->data.uw_O =
               ({
                struct __uwd_odd_18 *tmp =
                uw_malloc(ctx, sizeof(struct __uwd_odd_18));
                tmp->tag = __uwc_O_19;
                tmp->data.uw_O = NULL;
                 tmp;
                });
               tmp;
              })}; tmp; }));
   }
  
  static uw_Basis_string __uwn_jsify_22(uw_context, struct __uwd_odd_18*);
   static uw_Basis_string __uwn_jsify_23(uw_context, struct __uwd_odd_18*);
   static uw_Basis_string
    __uwn_jsify_22(uw_context ctx, struct __uwd_odd_18* __uwr_x_0)
    {restart:
      
    return(({
            struct __uwd_odd_18* disc =
            __uwr_x_0;
            
            disc == NULL ? "null"
              :
             disc != NULL && 1 ?
              ({struct __uwd_odd_18* __uwr_x_1 = disc;
                 __uwn_jsify_23(ctx, __uwr_x_1);
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/js/datatype/datatype.ur:56:72-56:75: pattern match failure");
               tmp;
               });
            }));
    }
    
    static uw_Basis_string
     __uwn_jsify_23(uw_context ctx, struct __uwd_odd_18* __uwr_x_0)
     {restart:
       
     return(({
             struct __uwd_odd_18* disc =
             __uwr_x_0;
             
             disc->tag == __uwc_O_19 && 1 ?
              ({struct __uwd_odd_18* __uwr_x_1 = disc->data.uw_O;
                 ({
                  uw_Basis_string arg0 = "{n:19,v:";
                   uw_Basis_string arg1 = __uwn_jsify_22(ctx, __uwr_x_1);
                    uw_Basis_string arg2 = "}";
                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                  });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/js/datatype/datatype.ur:56:72-56:75: pattern match failure");
               tmp;
               });
             }));
     }
   
  
  static uw_Basis_string __uwn_jsify_24(uw_context, struct __uws_2*);
   static uw_Basis_string
    __uwn_jsify_24(uw_context ctx, struct __uws_2* __uwr_x_0)
    {restart:
      
    return(({
            struct __uws_2* disc =
            __uwr_x_0;
            
            disc == NULL ? "null"
              :
             disc != NULL && 1 ?
              ({struct __uws_2 __uwr_x_1 = (*disc);
                 ({
                  uw_Basis_string arg0 = "{_L:";
                   
                   uw_Basis_string arg1 =
                    __uwn_jsify_24(ctx, __uwr_x_1.__uwf_L);
                    uw_Basis_string arg2 = ",_R:";
                     
                   uw_Basis_string arg3 =
                    __uwn_jsify_24(ctx, __uwr_x_1.__uwf_R);
                    uw_Basis_string arg4 = ",_V:";
                     
                   uw_Basis_string arg5 =
                    uw_Basis_htmlifyInt(ctx, __uwr_x_1.__uwf_V);
                    uw_Basis_string arg6 = "}";
                     uw_Basis_mstrcat(ctx, arg0, arg1, arg2, arg3, arg4, arg5, 
                                            arg6, NULL);
                  });
               })
               :
              ({
               uw_Basis_string
               tmp;
               uw_error(ctx, FATAL, "tests/js/datatype/datatype.ur:56:61-56:64: pattern match failure");
               tmp;
               });
            }));
    }
   
  
  static uw_unit
   __uwn_wrap_main_25(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_s_2 =
           uw_Basis_new_client_source(ctx, "{c:\"c\",v:2}");
           ({
            uw_Basis_source __uwr_c_3 =
            uw_Basis_new_client_source(ctx, "{c:\"c\",v:8}");
            ({
             uw_Basis_source __uwr_o_4 =
             uw_Basis_new_client_source(ctx,
              ({
               uw_Basis_string arg0 = "{c:\"c\",v:";
                uw_Basis_string arg1 = uw_Basis_htmlifyInt(ctx, 3LL);
                 uw_Basis_string arg2 = "}";
                  uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
               }));
             ({
              uw_Basis_source __uwr_t_5 =
              uw_Basis_new_client_source(ctx,
               ({
                uw_Basis_string arg0 = "{c:\"c\",v:";
                 
                 uw_Basis_string arg1 =
                  __uwn_jsify_24(ctx,
                   ({
                    struct __uws_2 *tmp =
                    uw_malloc(ctx, sizeof(struct __uws_2));
                    *tmp =
                    ({ struct __uws_2 tmp = {NULL, NULL, 5LL}; tmp; });
                    tmp;
                    }));
                  uw_Basis_string arg2 = "}";
                   uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                }));
              ({
               uw_Basis_source __uwr_e_6 =
               uw_Basis_new_client_source(ctx,
                ({
                 uw_Basis_string arg0 = "{c:\"c\",v:";
                  
                  uw_Basis_string arg1 =
                   __uwn_jsify_22(ctx,
                    ({
                     struct __uwd_odd_18 *tmp =
                     uw_malloc(ctx, sizeof(struct __uwd_odd_18));
                     tmp->tag = __uwc_O_19;
                     tmp->data.uw_O = NULL;
                      tmp;
                     }));
                   uw_Basis_string arg2 = "}";
                    uw_Basis_mstrcat(ctx, arg0, arg1, arg2, NULL);
                 }));
               ((uw_write(ctx, "<body"), 0),
                (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                       uw_Basis_get_settings(ctx,
                                                        0))), 0),
                 uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                            "")), 0),
                                      uw_end_region(ctx), ((uw_write(ctx, ">\n<button onclick='uw_event=event;exec("), 0),
                                                           (uw_begin_region(ctx),
                                                             ((uw_write(ctx, "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                              (uw_begin_region(ctx),
                                                                uw_Basis_htmlifySource_w(ctx,
                                                                 __uwr_s_2),
                                                               uw_end_region(ctx),
                                                                (uw_write(ctx, "}},x:{c:\"c\",v:null}}"), 0))),
                                                            uw_end_region(ctx), 
                                                            ((uw_write(ctx, ")'>Circle</button>\n<button onclick='uw_event=event;exec("), 0),
                                                             (uw_begin_region(ctx),
                                                               ((uw_write(ctx, "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                (uw_begin_region(ctx),
                                                                  uw_Basis_htmlifySource_w(ctx,
                                                                   __uwr_s_2),
                                                                 uw_end_region(ctx),
                                                                  (uw_write(ctx, 
                                                                   "}},x:{c:\"c\",v:null}}"), 0))),
                                                              uw_end_region(ctx),
                                                               ((uw_write(ctx, ")'>Fetch</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                (uw_begin_region(ctx),
                                                                  ((uw_write(ctx, 
                                                                    "{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:3},x:{c:\"c\",v:"), 0),
                                                                   (uw_begin_region(ctx),
                                                                     uw_Basis_htmlifySource_w(ctx,
                                                                      __uwr_c_3),
                                                                    uw_end_region(ctx),
                                                                     ((uw_write(ctx, 
                                                                       "}},x:{c:\"c\",v:"), 0),
                                                                      (uw_begin_region(ctx),
                                                                        uw_Basis_htmlifySource_w(ctx,
                                                                         __uwr_o_4
                                                                         ),
                                                                       uw_end_region(ctx),
                                                                        ((uw_write(ctx, 
                                                                          "}},x:{c:\"c\",v:"), 0),
                                                                         (uw_begin_region(ctx),
                                                                           uw_Basis_htmlifySource_w(ctx,
                                                                            __uwr_t_5
                                                                            ),
                                                                          uw_end_region(ctx),
                                                                           ((uw_write(ctx, 
                                                                             "}},x:{c:\"c\",v:"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_e_6
                                                                               ),
                                                                             uw_end_region(ctx),
                                                                              (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))))))))),
                                                                 uw_end_region(ctx),
                                                                  ((uw_write(ctx, 
                                                                    ")'>Rest</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                   (uw_begin_region(ctx),
                                                                     ((uw_write(ctx, 
                                                                       "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:4},x:{c:\"c\",v:"), 0),
                                                                      (uw_begin_region(ctx),
                                                                        uw_Basis_htmlifySource_w(ctx,
                                                                         __uwr_s_2
                                                                         ),
                                                                       uw_end_region(ctx),
                                                                        (uw_write(ctx, 
                                                                         "}},x:{c:\"c\",v:null}}"), 0))),
                                                                    uw_end_region(ctx),
                                                                     ((uw_write(ctx, 
                                                                       "))</script>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                      (uw_begin_region(ctx),
                                                                        ((uw_write(ctx, 
                                                                          "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:5},x:{c:\"c\",v:"), 0),
                                                                         (uw_begin_region(ctx),
                                                                           uw_Basis_htmlifySource_w(ctx,
                                                                            __uwr_c_3
                                                                            ),
                                                                          uw_end_region(ctx),
                                                                           (uw_write(ctx, 
                                                                            "}},x:{c:\"c\",v:null}}"), 0))),
                                                                       uw_end_region(ctx),
                                                                        ((uw_write(ctx, 
                                                                          "))</script>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                         (uw_begin_region(ctx),
                                                                           ((uw_write(ctx, 
                                                                             "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:6},x:{c:\"c\",v:"), 0),
                                                                            (uw_begin_region(ctx),
                                                                              uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_o_4
                                                                               ),
                                                                             uw_end_region(ctx),
                                                                              (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                          uw_end_region(ctx),
                                                                           ((uw_write(ctx, 
                                                                             "))</script>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                            (uw_begin_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:7},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_t_5
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                             uw_end_region(ctx),
                                                                              ((uw_write(ctx, 
                                                                               "))</script>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:9},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_e_6
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))))))))))))));
               });
              });
             });
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
 
 static void urlify_1(uw_context, struct __uwd_shape_1*);
  static void urlifyl_3(uw_context, struct __uws_3 *);
   static void urlify_9(uw_context, uw_Basis_int *);
    static void urlify_12(uw_context, struct __uws_2 *);
     static void urlify_18(uw_context, struct __uwd_odd_18*);
      static void urlify_15(uw_context, struct __uwd_odd_18* );
       static void urlify_1(uw_context ctx, struct __uwd_shape_1*
        it0) {
        if
         (it0->tag==__uwc_Dot_2) {
         uw_write(ctx, "Dot");
          } else {
         if
          (it0->tag==__uwc_Circle_3) {
          uw_write(ctx, "Circle/");
           uw_Basis_int it1 =
           it0->data.uw_Circle;
           uw_Basis_urlifyInt_w(ctx, it1);
            
           } else {
          if
           (it0->tag==__uwc_Box_4) {
           uw_write(ctx, "Box/");
            struct __uws_1 it1 =
            it0->data.uw_Box;
            {
             uw_Basis_int it2 = it1.__uwf_H;
             uw_Basis_urlifyInt_w(ctx, it2);
              }
             {
              uw_Basis_int it2 =
              it1.__uwf_W;
              uw_write(ctx, "/");
               uw_Basis_urlifyInt_w(ctx, it2);
                }
              
            } else {
           uw_error(ctx, FATAL, "Error urlifying datatype shape (%d)", it0->data);
            
            }
           
           }
          
          }
         
         
        }
        
        static void urlifyl_3(uw_context ctx, struct __uws_3
         *it0) {
         if (it0) {
          struct __uwd_shape_1* it1 =
          it0->__uwf_1;
          uw_write(ctx, "Cons/");
          urlify_1(ctx, it1);
           ;
          uw_write(ctx, "/");
          urlifyl_3(ctx, it0->__uwf_2);
          } else {
          uw_write(ctx, "Nil");
           }
          }
         
         static void urlify_9(uw_context ctx, uw_Basis_int
          *it0) {
          if (it0) {
           uw_Basis_int it1 =
            *it0;
            uw_write(ctx, "Just/");
            uw_Basis_urlifyInt_w(ctx, it1);
             ;
            } else {
                     uw_write(ctx, "Nothing");
                     }
           }
          
          static void urlify_12(uw_context ctx, struct __uws_2
           *it0) {
           if (it0) {
            struct __uws_2 it1 =
             *it0;
             uw_write(ctx, "Node/");
             {
              struct __uws_2* it2 = it1.__uwf_L;
              urlify_12(ctx, it2);
               }
              {
               struct __uws_2* it2 =
               it1.__uwf_R;
               uw_write(ctx, "/");
                urlify_12(ctx, it2);
                 }
               {
                uw_Basis_int it2 =
                it1.__uwf_V;
                uw_write(ctx, "/");
                 uw_Basis_urlifyInt_w(ctx, it2);
                  }
                ;
             } else {
                      uw_write(ctx, "Leaf");
                      }
            }
           
           static void urlify_18(uw_context ctx, struct __uwd_odd_18*
            it0) {
            if
             (it0->tag==__uwc_O_19) {
             uw_write(ctx, "O/");
              struct __uwd_odd_18* it1 = it0->data.uw_O;
              urlify_15(ctx, it1);
               
              } else {
             uw_error(ctx, FATAL, "Error urlifying datatype odd (%d)", it0->data);
              
              }
             
             
            }
            
            static void urlify_15(uw_context ctx, struct __uwd_odd_18*
             it0) {
             if (it0) {
              uw_write(ctx, "S/");
               urlify_18(ctx, it0);
                ;
               } else {
                        uw_write(ctx, "Z");
                        }
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
 
 if (!strcmp(request, "/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js")) {
  uw_write_header(ctx, "Content-Type: text/javascript\r\n");
   uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
   uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
   uw_write(ctx, jslib);
   return;
   }
  
  
  if (!strcmp(request, "/app.B96517851CE789B8DC6F4723C382168E197DB3F6.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 if (uw_serve_file(ctx, request, "Thu, 01 Jan 1970 00:00:00 GMT")) return;
 
 if (!strncmp(request, "/Datatype/main", 14) && (request[14] == 0 || request[14] == '/')) {
  request += 14;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.B96517851CE789B8DC6F4723C382168E197DB3F6.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_25(ctx, arg0, 0);
   uw_write(ctx, "</html>");
    return;
   }
   }
  
  if (!strncmp(request, "/Datatype/shapes", 16) && (request[16] == 0 || request[16] == '/')) {
   request += 16;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     struct __uws_3* it0 = __uwn_shapes_20(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     urlifyl_3(ctx, it0);
      return;
    }
    }
  
  if (!strncmp(request, "/Datatype/rest", 14) && (request[14] == 0 || request[14] == '/')) {
   request += 14;
   if (*request == '/') ++request;
   if (uw_hasPostBody(ctx)) {
    uw_Basis_postBody pb = uw_getPostBody(ctx);
     if (pb.data[0])
     request = uw_Basis_strcat(ctx, request, pb.data);
     }
    uw_write_header(ctx, "Content-type: text/plain\r\n");
     uw_set_could_write_db(ctx, 0);
   uw_set_at_most_one_query(ctx, 0);
   uw_set_needs_push(ctx, 0);
   uw_set_needs_sig(ctx, 0);
   uw_login(ctx);
   {
    uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
     struct __uws_4 it0 = __uwn_rest_21(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     {
      enum __uwe_color_5 it1 =
      it0.__uwf_1;
      if (it1==__uwc_Red_6) {
       uw_write(ctx, "Red");
        } else {
       if (it1==__uwc_Green_7) {
        uw_write(ctx, "Green");
         } else {
        if (it1==__uwc_Blue_8) {
         uw_write(ctx, "Blue");
          } else {
         uw_error(ctx, FATAL, "Error urlifying datatype color");
          
          }
         }
        }}
      {
       uw_Basis_int* it1 = it0.__uwf_2;
       uw_write(ctx, "/");
        urlify_9(ctx, it1);
         }
       {
        struct __uws_2* it1 =
        it0.__uwf_3;
        uw_write(ctx, "/");
         urlify_12(ctx, it1);
          }
        {
         struct __uwd_odd_18* it1 =
         it0.__uwf_4;
         uw_write(ctx, "/");
          urlify_15(ctx, it1);
           }
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
 