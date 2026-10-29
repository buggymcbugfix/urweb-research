val main : unit -> transaction page
val fail : unit -> transaction page
val slow : unit -> transaction page
val long : int -> transaction page
val lines : unit -> transaction page
val catalog : Lang.language -> int -> transaction page
val greet : string -> transaction page
val form : unit -> transaction page
val hook : postBody -> transaction page
