(* An io RPC copying a secret column into a table without an insert policy,
   with the read and the insert each in a transaction of its own: refused by
   -iflow, as the transaction RPC beside it is. *)

table fruit : { Id : int, Nam : string, Secret : string }
  PRIMARY KEY Id
table pub : { Nam : string }

policy sendClient (SELECT fruit.Id, fruit.Nam FROM fruit)

fun copy (id : int) : io unit =
    s <- runTransaction (oneRowE1 (SELECT (fruit.Secret) FROM fruit WHERE fruit.Id = {[id]}));
    runTransaction (dml (INSERT INTO pub (Nam) VALUES ({[s]})))

fun main () : transaction page =
    return <xml><body>
      <button value="go" onclick={fn _ => io_rpc (copy 1)}/>
    </body></xml>
