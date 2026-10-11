database dbname=test
 
 structure Broadcast :
  sig
   structure Make :
    functor (M :sig
                 con t :: Type
                 end) :
     sig
      con topic :: Type
       val inj : sql_injectable topic
       val create : transaction topic
       val subscribe : topic -> transaction (channel M.t)
       val send : topic -> M.t -> transaction unit
       val subscribers : topic -> transaction int
      end
   end =
  struct
   structure Make =
    functor (M :sig
                 con t :: Type
                 end) =>
     struct
      sequence s
       
       table t : [#Id = int, #Client = client, #Channel = channel M.t] keys
        Basis.primary_key [#Id] [[#Client = (_ :: Type)]] ! !
         {Id = _, Client = _} constraints Basis.no_constraint
       con topic :: _ = int
       val inj : sql_injectable topic = _
       val create = nextval s
       
       val rec
        subscribe : (_ :: Type) -> (_ :: Type) =
         fn id =>
          Basis.bind self
           (fn cli =>
             Basis.bind
              (oneOrNoRows
                (Basis.sql_query
                  {Rows =
                    Basis.sql_query1 [[]]
                     {Distinct = Basis.False, 
                       From = Basis.sql_from_table [#T] t, 
                       Where =
                        Basis.sql_binary Basis.sql_and
                         (Basis.sql_binary Basis.sql_eq
                           (Basis.sql_field [#T] [#Id]) (Basis.sql_inject id))
                         (Basis.sql_binary Basis.sql_eq
                           (Basis.sql_field [#T] [#Client])
                           (Basis.sql_inject cli)), 
                       GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                       Having = Basis.sql_inject Basis.True, 
                       SelectFields =
                        Basis.sql_subset
                         [[#T = ([#Channel = (_ :: Type)], (_ :: {Type}))]], 
                       SelectExps = {}}, 
                    OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                    Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
              (fn ro =>
                case ro of
                 None =>
                  Basis.bind channel
                   (fn ch =>
                     Basis.bind
                      (dml
                        (Basis.insert t
                          {Id = Basis.sql_inject id, 
                            Client = Basis.sql_inject cli, 
                            Channel = Basis.sql_inject ch}))
                      (fn _ : {} => return ch)) | 
                  Some r => return r.#T.#Channel))
       
       val rec
        send : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
         fn id =>
          fn msg =>
           queryI
            (Basis.sql_query
              {Rows =
                Basis.sql_query1 [[]]
                 {Distinct = Basis.False, From = Basis.sql_from_table [#T] t, 
                   Where =
                    Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                     (Basis.sql_inject id), 
                   GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                   Having = Basis.sql_inject Basis.True, 
                   SelectFields =
                    Basis.sql_subset
                     [[#T = ([#Channel = (_ :: Type)], (_ :: {Type}))]], 
                   SelectExps = {}}, 
                OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
            (fn r => Basis.send r.#T.#Channel msg)
       
       val rec
        subscribers : (_ :: Type) -> (_ :: Type) =
         fn id =>
          Basis.bind
           (oneRow
             (Basis.sql_query
               {Rows =
                 Basis.sql_query1 [[#T = ()]]
                  {Distinct = Basis.False, From = Basis.sql_from_table [#T] t, 
                    Where =
                     Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                      (Basis.sql_inject id), 
                    GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                    Having = Basis.sql_inject Basis.True, 
                    SelectFields =
                     Basis.sql_subset [[#T = ([], (_ :: {Type}))]], 
                    SelectExps = {N = Basis.sql_window Basis.sql_count}}, 
                 OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                 Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
           (fn r => return r.#N)
      end
   end
 
 structure Buffer :
  sig
   con t :: Type
    val create : transaction t
    val render : t -> signal xbody
    val write : t -> string -> transaction unit
   end =
  struct
   datatype lines = End | Line of {1 : string, 2 : source lines}
    con t :: _ = {Head : source lines, Tail : source (source lines)}
    
    val create =
     Basis.bind (source End)
      (fn head =>
        Basis.bind (source head) (fn tail => return {Head = head, Tail = tail}))
    
    val rec
     renderL : (_ :: Type) -> (_ :: Type) =
      fn lines =>
       case lines of
        End => Basis.cdata "" | 
         Line {1 = line, 2 = linesS} =>
          Basis.join (Top.txt line)
           (Basis.join
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (br {}) (Basis.cdata ""))
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None
               {Signal = renderS linesS} (dyn {})
               (Basis.cdata [(_ :: _)] [[]] "")))
                                                  and 
      renderS : (_ :: Type) -> (_ :: Type) =
       fn linesS =>
        Basis.bind (signal linesS) (fn lines => return (renderL lines))
    val rec render : (_ :: Type) -> (_ :: Type) = fn t => renderS t.#Head
    
    val rec
     write : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn t =>
       fn s =>
        Basis.bind (get t.#Tail)
         (fn oldTail =>
           Basis.bind (source End)
            (fn newTail =>
              Basis.bind (set oldTail (Line {1 = s, 2 = newTail}))
               (fn _ : {} => set t.#Tail newTail)))
   end
 
 structure Chat : sig
                   val main : unit -> transaction page
                   end =
  struct
   structure Room = Broadcast.Make(struct
                                    con t :: _ = string
                                    end)
    sequence s
    
    table t : [#Id = int, #Title = string, #Room = Room.topic] keys
     Basis.primary_key [#Id] [[]] ! ! {Id = _} constraints Basis.no_constraint
    
    val rec
     chat : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
      fn id =>
       fn $x : (_ :: Type) =>
        case $x of
         {} =>
          Basis.bind
           (oneRow
             (Basis.sql_query
               {Rows =
                 Basis.sql_query1 [[]]
                  {Distinct = Basis.False, From = Basis.sql_from_table [#T] t, 
                    Where =
                     Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                      (Basis.sql_inject id), 
                    GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                    Having = Basis.sql_inject Basis.True, 
                    SelectFields =
                     Basis.sql_subset
                      [[#T =
                         ([#Room = (_ :: Type)] ++ [#Title = (_ :: Type)], 
                           (_ :: {Type}))]], SelectExps = {}}, 
                 OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
                 Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset}))
           (fn r =>
             Basis.bind (Room.subscribe r.#T.#Room)
              (fn ch =>
                Basis.bind (source "")
                 (fn newLine =>
                   Basis.bind Buffer.create
                    (fn buf =>
                      let
                       val rec
                        onload : (_ :: Type) -> (_ :: Type) =
                         fn $x : (_ :: Type) =>
                          case $x of
                           {} =>
                            let
                             val rec
                              listener : (_ :: Type) -> (_ :: Type) =
                               fn $x : (_ :: Type) =>
                                case $x of
                                 {} =>
                                  Basis.bind (recv ch)
                                   (fn s =>
                                     Basis.bind (Buffer.write buf s)
                                      (fn _ : {} => listener {}))
                             in
                             listener {}
                             end
                        
                        val rec
                         getRoom : (_ :: Type) -> (_ :: Type) =
                          fn $x : (_ :: Type) =>
                           case $x of
                            {} =>
                             Basis.bind
                              (oneRow
                                (Basis.sql_query
                                  {Rows =
                                    Basis.sql_query1 [[]]
                                     {Distinct = Basis.False, 
                                       From = Basis.sql_from_table [#T] t, 
                                       Where =
                                        Basis.sql_binary Basis.sql_eq
                                         (Basis.sql_field [#T] [#Id])
                                         (Basis.sql_inject id), 
                                       GroupBy =
                                        Basis.sql_subset_all [(_ :: {{Type}})], 
                                       Having = Basis.sql_inject Basis.True, 
                                       SelectFields =
                                        Basis.sql_subset
                                         [[#T =
                                            ([#Room = (_ :: Type)], 
                                              (_ :: {Type}))]], SelectExps = {}}
                                    , 
                                    OrderBy =
                                     Basis.sql_order_by_Nil [(_ :: {Type})], 
                                    Limit = Basis.sql_no_limit, 
                                    Offset = Basis.sql_no_offset}))
                              (fn r => return r.#T.#Room)
                        
                        val rec
                         speak : (_ :: Type) -> (_ :: Type) =
                          fn line =>
                           Basis.bind (getRoom {})
                            (fn room => Room.send room line)
                        
                        val rec
                         doSpeak : (_ :: Type) -> (_ :: Type) =
                          fn $x : (_ :: Type) =>
                           case $x of
                            {} =>
                             Basis.bind (get newLine)
                              (fn line =>
                                Basis.bind (set newLine "")
                                 (fn _ : {} => rpc (speak line)))
                       in
                       return
                        (Basis.tag Basis.null Basis.None Basis.noStyle
                          Basis.None {Onload = onload {}} (body {})
                          (Basis.join (Basis.cdata "\n")
                            (Basis.join (Basis.cdata "          ")
                              (Basis.join
                                (Basis.tag Basis.null Basis.None Basis.noStyle
                                  Basis.None {} (h1 {}) (Top.txt r.#T.#Title))
                                (Basis.join (Basis.cdata "\n")
                                  (Basis.join (Basis.cdata "\n")
                                    (Basis.join (Basis.cdata "          ")
                                      (Basis.join
                                        (Basis.tag Basis.null Basis.None
                                          Basis.noStyle Basis.None
                                          {Value = "Send:", 
                                            Onclick = fn _ => doSpeak {}}
                                          (button {}) (Basis.cdata ""))
                                        (Basis.join (Basis.cdata " ")
                                          (Basis.join
                                            (Basis.tag Basis.null Basis.None
                                              Basis.noStyle Basis.None
                                              {Source = newLine} (ctextbox {})
                                              (Basis.cdata ""))
                                            (Basis.join (Basis.cdata "\n")
                                              (Basis.join (Basis.cdata "\n")
                                                (Basis.join
                                                  (Basis.cdata "          ")
                                                  (Basis.join
                                                    (Basis.tag Basis.null
                                                      Basis.None Basis.noStyle
                                                      Basis.None {} (h2 {})
                                                      (Basis.cdata "Messages"))
                                                    (Basis.join
                                                      (Basis.cdata "\n")
                                                      (Basis.join
                                                        (Basis.cdata "\n")
                                                        (Basis.join
                                                          (Basis.cdata
                                                            "          ")
                                                          (Basis.join
                                                            (Basis.tag
                                                              Basis.null
                                                              Basis.None
                                                              Basis.noStyle
                                                              Basis.None
                                                              {Signal =
                                                                Buffer.render
                                                                 buf} (dyn {})
                                                              (Basis.cdata
                                                                [(_ :: _)] [[]]
                                                                ""))
                                                            (Basis.join
                                                              (Basis.cdata "\n")
                                                              (Basis.join
                                                                (Basis.cdata
                                                                  "          ")
                                                                (Basis.join
                                                                  (Basis.cdata
                                                                    "\n")
                                                                  (Basis.cdata
                                                                    "        "))))))))))))))))))))))
                       end))))
    
    val rec
     list : (_ :: Type) -> (_ :: Type) =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         queryX'
          (Basis.sql_query
            {Rows =
              Basis.sql_query1 [[]]
               {Distinct = Basis.False, From = Basis.sql_from_table [#T] t, 
                 Where = Basis.sql_inject Basis.True, 
                 GroupBy = Basis.sql_subset_all [(_ :: {{Type}})], 
                 Having = Basis.sql_inject Basis.True, 
                 SelectFields = Basis.sql_subset [[#T = ((_ :: {Type}), [])]], 
                 SelectExps = {}}, 
              OrderBy = Basis.sql_order_by_Nil [(_ :: {Type})], 
              Limit = Basis.sql_no_limit, Offset = Basis.sql_no_offset})
          (fn r =>
            Basis.bind (Room.subscribers r.#T.#Room)
             (fn count =>
               return
                (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
                  (tr {})
                  (Basis.join (Basis.cdata "\n")
                    (Basis.join (Basis.cdata "          ")
                      (Basis.join
                        (Basis.tag Basis.null Basis.None Basis.noStyle
                          Basis.None {} (td {}) (Top.txt r.#T.#Id))
                        (Basis.join (Basis.cdata "\n")
                          (Basis.join (Basis.cdata "          ")
                            (Basis.join
                              (Basis.tag Basis.null Basis.None Basis.noStyle
                                Basis.None {} (td {}) (Top.txt r.#T.#Title))
                              (Basis.join (Basis.cdata "\n")
                                (Basis.join (Basis.cdata "          ")
                                  (Basis.join
                                    (Basis.tag Basis.null Basis.None
                                      Basis.noStyle Basis.None {} (td {})
                                      (Top.txt count))
                                    (Basis.join (Basis.cdata "\n")
                                      (Basis.join (Basis.cdata "          ")
                                        (Basis.join
                                          (Basis.tag Basis.null Basis.None
                                            Basis.noStyle Basis.None {} (td {})
                                            (Basis.form Basis.None Basis.None
                                              (Basis.tag Basis.null Basis.None
                                                Basis.noStyle Basis.None
                                                {Action = chat r.#T.#Id, 
                                                  Value = "Enter"} (submit {})
                                                (Basis.cdata [(_ :: _)] [[]] ""))))
                                          (Basis.join (Basis.cdata "\n")
                                            (Basis.join
                                              (Basis.cdata "          ")
                                              (Basis.join
                                                (Basis.tag Basis.null
                                                  Basis.None Basis.noStyle
                                                  Basis.None {} (td {})
                                                  (Basis.form Basis.None
                                                    Basis.None
                                                    (Basis.tag Basis.null
                                                      Basis.None Basis.noStyle
                                                      Basis.None
                                                      {Action = delete r.#T.#Id,
                                                                               
                                                        Value = "Delete"}
                                                      (submit {})
                                                      (Basis.cdata [(_ :: _)]
                                                        [[]] ""))))
                                                (Basis.join (Basis.cdata "\n")
                                                  (Basis.cdata "        "))))))))))))))))))))
      
       and 
      delete : (_ :: Type) -> (_ :: Type) -> (_ :: Type) =
       fn id =>
        fn $x : (_ :: Type) =>
         case $x of
          {} =>
           Basis.bind
            (dml
              (Basis.delete t
                (Basis.sql_binary Basis.sql_eq (Basis.sql_field [#T] [#Id])
                  (Basis.sql_inject id)))) (fn _ : {} => main {})
                                                                  and 
      main : (_ :: Type) -> (_ :: Type) =
       fn $x : (_ :: Type) =>
        case $x of
         {} =>
          let
           val rec
            create : (_ :: Type) -> (_ :: Type) =
             fn r =>
              Basis.bind (nextval s)
               (fn id =>
                 Basis.bind Room.create
                  (fn room =>
                    Basis.bind
                     (dml
                       (Basis.insert t
                         {Id = Basis.sql_inject id, 
                           Title = Basis.sql_inject r.#Title, 
                           Room = Basis.sql_inject room}))
                     (fn _ : {} => main {})))
           in
           Basis.bind (list {})
            (fn ls =>
              return
               (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
                 (body {})
                 (Basis.join (Basis.cdata "\n")
                   (Basis.join (Basis.cdata "          ")
                     (Basis.join
                       (Basis.tag Basis.null Basis.None Basis.noStyle
                         Basis.None {} (h1 {}) (Basis.cdata "Current Channels"))
                       (Basis.join (Basis.cdata "\n")
                         (Basis.join (Basis.cdata "\n")
                           (Basis.join (Basis.cdata "          ")
                             (Basis.join
                               (Basis.tag Basis.null Basis.None Basis.noStyle
                                 Basis.None {} (tabl {})
                                 (Basis.join (Basis.cdata "\n")
                                   (Basis.join (Basis.cdata "            ")
                                     (Basis.join
                                       (Basis.tag Basis.null Basis.None
                                         Basis.noStyle Basis.None {} (tr {})
                                         (Basis.join (Basis.cdata " ")
                                           (Basis.join
                                             (Basis.tag Basis.null Basis.None
                                               Basis.noStyle Basis.None {}
                                               (th {}) (Basis.cdata "ID"))
                                             (Basis.join (Basis.cdata " ")
                                               (Basis.join
                                                 (Basis.tag Basis.null
                                                   Basis.None Basis.noStyle
                                                   Basis.None {} (th {})
                                                   (Basis.cdata "Title"))
                                                 (Basis.join (Basis.cdata " ")
                                                   (Basis.join
                                                     (Basis.tag Basis.null
                                                       Basis.None Basis.noStyle
                                                       Basis.None {} (th {})
                                                       (Basis.cdata
                                                         "#Subscribers"))
                                                     (Basis.cdata " "))))))))
                                       (Basis.join (Basis.cdata "\n")
                                         (Basis.join
                                           (Basis.cdata "            ")
                                           (Basis.join ls
                                             (Basis.join (Basis.cdata "\n")
                                               (Basis.cdata "          ")))))))))
                               (Basis.join (Basis.cdata "\n")
                                 (Basis.join (Basis.cdata "          ")
                                   (Basis.join (Basis.cdata "\n")
                                     (Basis.join (Basis.cdata "          ")
                                       (Basis.join
                                         (Basis.tag Basis.null Basis.None
                                           Basis.noStyle Basis.None {} (h1 {})
                                           (Basis.cdata "New Channel"))
                                         (Basis.join (Basis.cdata "\n")
                                           (Basis.join
                                             (Basis.cdata "          ")
                                             (Basis.join (Basis.cdata "\n")
                                               (Basis.join
                                                 (Basis.cdata "          ")
                                                 (Basis.join
                                                   (Basis.form Basis.None
                                                     Basis.None
                                                     (Basis.join
                                                       (Basis.cdata "\n")
                                                       (Basis.join
                                                         (Basis.cdata
                                                           "            Title: ")
                                                         (Basis.join
                                                           (Basis.tag
                                                             Basis.null
                                                             Basis.None
                                                             Basis.noStyle
                                                             Basis.None {}
                                                             (textbox [#Title]
                                                               {})
                                                             (Basis.cdata ""))
                                                           (Basis.join
                                                             (Basis.tag
                                                               Basis.null
                                                               Basis.None
                                                               Basis.noStyle
                                                               Basis.None {}
                                                               (br {})
                                                               (Basis.cdata ""))
                                                             (Basis.join
                                                               (Basis.cdata
                                                                 "\n")
                                                               (Basis.join
                                                                 (Basis.cdata
                                                                   "            ")
                                                                 (Basis.join
                                                                   (Basis.tag
                                                                     Basis.null
                                                                     Basis.None
                                                                     Basis.
                                                                      noStyle
                                                                     Basis.None
                                                                     {Action =
                                                                       create}
                                                                     (submit {})
                                                                     (Basis.
                                                                       cdata
                                                                       [(_ ::
                                                                         _)]
                                                                       [[]] ""))
                                                                   (Basis.join
                                                                     (Basis.
                                                                       cdata
                                                                       "\n")
                                                                     (Basis.
                                                                       cdata
                                                                       "          "))))))))))
                                                   (Basis.join
                                                     (Basis.cdata "\n")
                                                     (Basis.cdata "        ")))))))))))))))))))))
           end
   end
 export Chat