(* A form with many subform entries, so that the runtime's table of
   subinputs grows while the request's inputs are read, and has pointers into
   it rebased. *)

fun entries n =
    if n <= 0 then <xml/>
    else <xml><entry><hidden{#Num} value={show n}/><textbox{#Text}/></entry>{entries (n - 1)}</xml>

fun handle r =
    return <xml><body>{List.mapX (fn e => <xml>{[e.Num]}={[e.Text]} </xml>) r.Lines}</body></xml>

fun main () = return <xml><body>
  <form><subforms{#Lines}>{entries 20}</subforms><submit action={handle}/></form>
</body></xml>
