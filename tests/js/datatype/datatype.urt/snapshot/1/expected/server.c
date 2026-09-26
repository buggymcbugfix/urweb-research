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
  struct __uws_2 {
   struct __uwd_shape_1* __uwf_1;
    struct __uws_2* __uwf_2;
     };
  
  static char jslib[] = "*runtime elided*";
   static char jsapp[] = "*script elided*";
  
  static struct __uws_2*
   __uwn_shapes_5(uw_context ctx, uw_unit __uwr_$x_0, uw_unit __uwr___1)
   {
   return(({
           struct __uws_2 *tmp = uw_malloc(ctx, sizeof(struct __uws_2));
           *tmp =
           ({ struct __uws_2 tmp =
            {({
              struct __uwd_shape_1 *tmp =
              uw_malloc(ctx, sizeof(struct __uwd_shape_1));
              tmp->tag = __uwc_Circle_3;
              tmp->data.uw_Circle = 1LL;
               tmp;
              }), 
              ({
               struct __uws_2 *tmp =
               uw_malloc(ctx, sizeof(struct __uws_2));
               *tmp =
               ({ struct __uws_2 tmp =
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
                   struct __uws_2 *tmp =
                   uw_malloc(ctx, sizeof(struct __uws_2));
                   *tmp =
                   ({ struct __uws_2 tmp =
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
  
  static uw_unit
   __uwn_wrap_main_9(uw_context ctx, uw_unit __uwr_x0_0, uw_unit __uwr___1)
   {
   return(({
           uw_Basis_source __uwr_s_2 =
           uw_Basis_new_client_source(ctx, "{c:\"c\",v:2}");
           ((uw_write(ctx, "<body"), 0),
            (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onload(ctx,
                                                   uw_Basis_get_settings(ctx,
                                                    0))), 0),
             uw_end_region(ctx), (uw_begin_region(ctx), (uw_write(ctx, uw_Basis_maybe_onunload(ctx,
                                                                        "")), 0),
                                  uw_end_region(ctx), ((uw_write(ctx, ">\n<button onclick='uw_event=event;exec("), 0),
                                                       (uw_begin_region(ctx), ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:1},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                        uw_end_region(ctx), ((uw_write(ctx, 
                                                                              ")'>Circle</button>\n<button onclick='uw_event=event;exec("), 0),
                                                                             (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:2},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                              uw_end_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               ")'>Fetch</button>\n<script type=\"text/javascript\">dyn(\"span\", execD("), 0),
                                                                               (uw_begin_region(ctx),
                                                                               ((uw_write(ctx, 
                                                                               "{c:\"a\",f:{c:\"a\",f:{c:\"n\",n:3},x:{c:\"c\",v:"), 0),
                                                                               (uw_begin_region(ctx),
                                                                               uw_Basis_htmlifySource_w(ctx,
                                                                               __uwr_s_2
                                                                               ),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "}},x:{c:\"c\",v:null}}"), 0))),
                                                                               uw_end_region(ctx),
                                                                               (uw_write(ctx, 
                                                                               "))</script>\n</body>"), 0))))))))));
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
  static void urlifyl_2(uw_context, struct __uws_2 *);
   static void urlify_1(uw_context ctx, struct __uwd_shape_1*
    it0) {
    if
     (it0->tag==__uwc_Dot_2) {
     uw_write(ctx, "Dot");
      } else {
     if
      (it0->tag==__uwc_Circle_3) {
      uw_write(ctx, "Circle/");
       uw_Basis_int it1 = it0->data.uw_Circle;
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
    
    static void urlifyl_2(uw_context ctx, struct __uws_2
     *it0) {
     if (it0) {
      struct __uwd_shape_1* it1 =
      it0->__uwf_1;
      uw_write(ctx, "Cons/");
      urlify_1(ctx, it1);
       ;
      uw_write(ctx, "/");
      urlifyl_2(ctx, it0->__uwf_2);
      } else {
      uw_write(ctx, "Nil");
       }
      }
     
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
  
  
  if (!strcmp(request, "/app.639FFA0FE1AF8B2B197438349036166993FB8407.js")) {
   uw_write_header(ctx, "Content-Type: text/javascript\r\n");
    uw_write_header(ctx, "Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT\r\n");
    uw_write_header(ctx, "Cache-Control: max-age=31536000, public\r\n");
    uw_write(ctx, jsapp);
    return;
    }
   
 
 if (!strncmp(request, "/Datatype/main", 14) && (request[14] == 0 || request[14] == '/')) {
  request += 14;
  if (*request == '/') ++request;
  uw_write_header(ctx, "Content-type: text/html; charset=utf-8\r\n");
   uw_write_header(ctx, "Content-script-type: text/javascript\r\n");
    uw_write(ctx, uw_begin_html5);
   uw_mayReturnIndirectly(ctx);
   uw_set_script_header(ctx, "<script type=\"text/javascript\" src=\"/runtime.678742345B8E282393A78F7E3E4433E00FABF9F2.js\"></script>\n<script type=\"text/javascript\" src=\"/app.639FFA0FE1AF8B2B197438349036166993FB8407.js\"></script>\n");
   uw_set_could_write_db(ctx, 0);
  uw_set_at_most_one_query(ctx, 0);
  uw_set_needs_push(ctx, 0);
  uw_set_needs_sig(ctx, 0);
  uw_login(ctx);
  {
   uw_unit arg0 = uw_Basis_unurlifyUnit(ctx, &request);
    __uwn_wrap_main_9(ctx, arg0, 0);
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
     struct __uws_2* it0 = __uwn_shapes_5(ctx, arg0, 0);
    uw_write(ctx, uw_get_real_script(ctx));
     uw_write(ctx, "\n");
     urlifyl_2(ctx, it0);
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
 