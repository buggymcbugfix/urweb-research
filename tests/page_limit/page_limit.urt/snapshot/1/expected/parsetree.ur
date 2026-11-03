structure Page_limit :
 sig
  val main : unit -> transaction page
   val small : unit -> transaction page
   val half : unit -> transaction page
   val over : unit -> transaction page
  end =
 struct
  val rec
   main : (_ :: Type) -> transaction page =
    fn $x : (_ :: Type) =>
     case $x of
      {} =>
       return
        (Basis.join
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (head {})
            (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
              (title {}) (Basis.cdata "limit")))
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (body {}) (Basis.cdata "main")))
   
   val rec
    small : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.join
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (head {})
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (title {}) (Basis.cdata "limit")))
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (body {})
             (Basis.join (Basis.cdata "\n")
               (Basis.join
                 (Basis.cdata
                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                 (Basis.join (Basis.cdata "\n") (Basis.cdata "    "))))))
   
   val rec
    half : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.join
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (head {})
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (title {}) (Basis.cdata "limit")))
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (body {})
             (Basis.join (Basis.cdata "\n")
               (Basis.join
                 (Basis.cdata
                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                 (Basis.join (Basis.cdata "\n")
                   (Basis.join
                     (Basis.cdata
                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join
                         (Basis.cdata
                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                         (Basis.join (Basis.cdata "\n")
                           (Basis.join
                             (Basis.cdata
                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                             (Basis.join (Basis.cdata "\n")
                               (Basis.join
                                 (Basis.cdata
                                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                 (Basis.join (Basis.cdata "\n")
                                   (Basis.join
                                     (Basis.cdata
                                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                     (Basis.join (Basis.cdata "\n")
                                       (Basis.join
                                         (Basis.cdata
                                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                         (Basis.join (Basis.cdata "\n")
                                           (Basis.join
                                             (Basis.cdata
                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                             (Basis.join (Basis.cdata "\n")
                                               (Basis.join
                                                 (Basis.cdata
                                                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                 (Basis.join (Basis.cdata "\n")
                                                   (Basis.join
                                                     (Basis.cdata
                                                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                     (Basis.join
                                                       (Basis.cdata "\n")
                                                       (Basis.join
                                                         (Basis.cdata
                                                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                         (Basis.join
                                                           (Basis.cdata "\n")
                                                           (Basis.join
                                                             (Basis.cdata
                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                             (Basis.join
                                                               (Basis.cdata
                                                                 "\n")
                                                               (Basis.cdata
                                                                 "    "))))))))))))))))))))))))))))
   
   val rec
    over : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        return
         (Basis.join
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (head {})
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (title {}) (Basis.cdata "limit")))
           (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
             (body {})
             (Basis.join (Basis.cdata "\n")
               (Basis.join
                 (Basis.cdata
                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                 (Basis.join (Basis.cdata "\n")
                   (Basis.join
                     (Basis.cdata
                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                     (Basis.join (Basis.cdata "\n")
                       (Basis.join
                         (Basis.cdata
                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                         (Basis.join (Basis.cdata "\n")
                           (Basis.join
                             (Basis.cdata
                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                             (Basis.join (Basis.cdata "\n")
                               (Basis.join
                                 (Basis.cdata
                                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                 (Basis.join (Basis.cdata "\n")
                                   (Basis.join
                                     (Basis.cdata
                                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                     (Basis.join (Basis.cdata "\n")
                                       (Basis.join
                                         (Basis.cdata
                                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                         (Basis.join (Basis.cdata "\n")
                                           (Basis.join
                                             (Basis.cdata
                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                             (Basis.join (Basis.cdata "\n")
                                               (Basis.join
                                                 (Basis.cdata
                                                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                 (Basis.join (Basis.cdata "\n")
                                                   (Basis.join
                                                     (Basis.cdata
                                                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                     (Basis.join
                                                       (Basis.cdata "\n")
                                                       (Basis.join
                                                         (Basis.cdata
                                                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                         (Basis.join
                                                           (Basis.cdata "\n")
                                                           (Basis.join
                                                             (Basis.cdata
                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                             (Basis.join
                                                               (Basis.cdata
                                                                 "\n")
                                                               (Basis.join
                                                                 (Basis.cdata
                                                                   "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                 (Basis.join
                                                                   (Basis.cdata
                                                                     "\n")
                                                                   (Basis.join
                                                                     (Basis.
                                                                       cdata
                                                                       "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                     (Basis.
                                                                       join
                                                                       (Basis.
                                                                         cdata
                                                                         "\n")
                                                                       (Basis.
                                                                         join
                                                                         (Basis.
                                                                           cdata
                                                                           "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                         (Basis.
                                                                           join
                                                                           (Basis.
                                                                             cdata
                                                                             "\n")
                                                                           (Basis.
                                                                             join
                                                                             (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                             (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "      0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789")
                                                                               (Basis.
                                                                               join
                                                                               (Basis.
                                                                               cdata
                                                                               "\n")
                                                                               (Basis.
                                                                               cdata
                                                                               "    "))))))))))))))))))))))))))))))))))))))))))))))))))))
  end
 export Page_limit