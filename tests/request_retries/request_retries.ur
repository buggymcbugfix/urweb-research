(* How many times a request is run when its transaction asks for a bounded
   retry: [flaky] asks every time, and the request fails once the attempts
   are used up; [once] asks on its first attempt only and is served on the
   second.  The server's log counts the attempts.  The C probes are those
   of ../ioprobe. *)

fun flaky () : transaction page =
    Probe.boundedRetry "flaky";
    return <xml><body>never served</body></xml>

fun once () : transaction page =
    Probe.failOnce "once";
    return <xml><body>served on the second attempt</body></xml>

fun main () : transaction page = return <xml><body>up</body></xml>
