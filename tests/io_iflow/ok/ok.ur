(* The copy of io.ur with a policy that permits it: a column may be
   inserted into pub when some fruit has it, and the Nam read in the first
   transaction is such a column.  Accepted by -iflow: what the first
   transaction established about the value holds in the second. *)

table fruit : { Id : int, Nam : string, Secret : string }
  PRIMARY KEY Id
table pub : { Nam : string }

policy sendClient (SELECT fruit.Id, fruit.Nam FROM fruit)
policy mayInsert (SELECT * FROM pub AS New, fruit WHERE New.Nam = fruit.Nam)

fun copy (id : int) : io unit =
    s <- runTransaction (oneRowE1 (SELECT (fruit.Nam) FROM fruit WHERE fruit.Id = {[id]}));
    runTransaction (dml (INSERT INTO pub (Nam) VALUES ({[s]})))

fun main () : transaction page =
    return <xml><body>
      <button value="go" onclick={fn _ => io_rpc (copy 1)}/>
    </body></xml>
