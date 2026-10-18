(* The pages of the benchmark (see ../README.md): one of a few bytes, one
 * of 64 KiB, one that reads a row, and one that writes a row. *)

table item : { Id : int, Text : string }
  PRIMARY KEY Id

table log : { N : int, Note : string }

fun tiny () = return <xml><body>tiny</body></xml>

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

fun page () = return <xml><body>{[doubled 12]}</body></xml>

fun read n =
    r <- oneOrNoRows1 (SELECT item.Text FROM item WHERE item.Id = {[n]});
    return <xml><body>{[case r of None => "none" | Some r => r.Text]}</body></xml>

fun write n =
    dml (INSERT INTO log (N, Note) VALUES ({[n]}, 'written'));
    return <xml><body>written {[n]}</body></xml>
