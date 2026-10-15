(* A transaction RPC copying a secret column into a table without an insert
   policy: refused by -iflow. *)

table fruit : { Id : int, Nam : string, Secret : string }
  PRIMARY KEY Id
table pub : { Nam : string }

policy sendClient (SELECT fruit.Id, fruit.Nam FROM fruit)

fun copy (id : int) : transaction unit =
    s <- oneRowE1 (SELECT (fruit.Secret) FROM fruit WHERE fruit.Id = {[id]});
    dml (INSERT INTO pub (Nam) VALUES ({[s]}))

fun main () : transaction page =
    return <xml><body>
      <button value="go" onclick={fn _ => rpc (copy 1)}/>
    </body></xml>
