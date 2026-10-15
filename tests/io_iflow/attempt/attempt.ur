(* The leak of io.ur through attemptTransaction: the secret read in one
   attempted transaction, inserted in another: refused by -iflow. *)

table fruit : { Id : int, Nam : string, Secret : string }
  PRIMARY KEY Id
table pub : { Nam : string }

policy sendClient (SELECT fruit.Id, fruit.Nam FROM fruit)

fun copy (id : int) : io unit =
    r <- attemptTransaction (oneRowE1 (SELECT (fruit.Secret) FROM fruit WHERE fruit.Id = {[id]}));
    case r of
        None => return ()
      | Some s => runTransaction (dml (INSERT INTO pub (Nam) VALUES ({[s]})))

fun main () : transaction page =
    return <xml><body>
      <button value="go" onclick={fn _ => io_rpc (copy 1)}/>
    </body></xml>
