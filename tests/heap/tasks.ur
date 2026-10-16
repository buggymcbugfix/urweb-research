(* A periodic task that grows its heap to some MiB on every tick, and says
   so, once: on the first tick it gets through.  Its tick counter is in C
   (Tick), so a tick that is run again from the start counts as another,
   and what it says is how many attempts it took. *)

fun doubled k =
    let
        fun go s k = if k <= 0 then s else go (s ^ s) (k - 1)
    in
        go "0123456789abcdef" k
    end

task periodic 1 = fn () =>
    n <- Tick.periodic;
    message <- return ("periodic tick " ^ show n ^ ": " ^ show (strlen (doubled 18)) ^ " bytes");
    first <- Tick.once message;
    if first then
        debug message
    else
        return ()

fun main () = return <xml><body>up</body></xml>
