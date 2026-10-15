(* A heap limit of 1 MB in the .urp (limit_env.urp), and the environment
 * variables that set the heap's size and limit when the program starts:
 * which values count, and which are refused. *)

(* 16 * 2^k bytes, by doubling. *)
fun doubled k =
    if k <= 0 then
        "0123456789abcdef"
    else
        let
            val s = doubled (k - 1)
        in
            s ^ s
        end

(* A few bytes: what fits in a heap of one page. *)
fun tiny () = return <xml><body>tiny</body></xml>

(* 64 KiB, and as much again for the strings it is built from. *)
fun small () = return <xml><body>{[strlen (doubled 12)]} bytes</body></xml>

(* 2 MiB, and as much again: over the .urp's limit. *)
fun big () = return <xml><body>{[strlen (doubled 17)]} bytes</body></xml>
