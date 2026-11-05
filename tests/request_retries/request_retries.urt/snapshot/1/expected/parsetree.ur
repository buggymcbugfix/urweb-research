database dbname=request_retries.db
 
 extern structure Probe :
  sig
   val step : io int
    val stepT : transaction int
    val inTransaction : io bool
    val inTransactionT : transaction bool
    val effect : string -> io unit
    val effectT : string -> transaction unit
    val failOnce : string -> transaction unit
    val boundedRetry : string -> transaction unit
    val fatalIo : string -> io unit
    val retryIo : string -> io unit
    val armConflict : string -> transaction unit
    val armConflictIo : string -> io unit
    val beginIo : io unit
    val dmlIo : string -> int -> io unit
    val pageWriteT : string -> transaction unit
    val register : string -> transaction unit
    val registerNoRollback : string -> transaction unit
    val registerIo : string -> io unit
    val failInCommitBeforeDb : string -> transaction unit
    val failInCommit : string -> transaction unit
    val raiseInCommit : string -> transaction unit
    val pushCleanupT : string -> transaction unit
    val popCleanupT : transaction unit
    val pushCleanupIo : string -> io unit
    val popCleanupIo : io unit
    val pendingWrite : string -> transaction unit
    val finishPendingWrite : transaction unit
    val finishPendingWriteIo : io unit
    val sleepIo : int -> io unit
    val sleepT : int -> transaction unit
    val traceSqlIo : io unit
   end
 
 structure Request_retries :
  sig
   val main : unit -> transaction page
    val flaky : unit -> transaction page
    val once : unit -> transaction page
   end =
  struct
   val rec
    flaky : (_ :: Type) -> transaction page =
     fn $x : (_ :: Type) =>
      case $x of
       {} =>
        Basis.bind (Probe.boundedRetry "flaky")
         (fn _ : {} =>
           return
            (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
              (body {}) (Basis.cdata "never served")))
    
    val rec
     once : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         Basis.bind (Probe.failOnce "once")
          (fn _ : {} =>
            return
             (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
               (body {}) (Basis.cdata "served on the second attempt")))
    
    val rec
     main : (_ :: Type) -> transaction page =
      fn $x : (_ :: Type) =>
       case $x of
        {} =>
         return
          (Basis.tag Basis.null Basis.None Basis.noStyle Basis.None {}
            (body {}) (Basis.cdata "up"))
   end
 export Request_retries