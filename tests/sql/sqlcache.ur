(* A query the server caches under -sqlcache, with an option parameter in
   its key, and the DML that flushes it; beside them a datatype the client
   holds a value of.  The cache checks, stores and flushes sqlcache put
   around them are in the C, and it is the last pass that adds to the
   program before the C is numbered. *)

table t : { Id : int, Val : option int, Name : string }
  PRIMARY KEY Id

datatype shape = Dot | Circle of int

fun named (v : option int) : transaction xbody =
    queryX1 (SELECT t.Name FROM t WHERE t.Val = {[v]})
            (fn r => <xml><div>{[r.Name]}</div></xml>)

fun add (r : { Id : string, Name : string }) : transaction page =
    dml (INSERT INTO t (Id, Val, Name) VALUES ({[readError r.Id]}, {[Some 3]}, {[r.Name]}));
    main ()

and main () : transaction page =
    s <- source (Circle 2);
    threes <- named (Some 3);
    nones <- named None;
    return <xml><body>
      {threes}
      {nones}
      <form><textbox{#Id}/><textbox{#Name}/><submit action={add}/></form>
      <button value="Dot" onclick={fn _ => set s Dot}/>
      <dyn signal={v <- signal s; return <xml>{[case v of Dot => 0 | Circle n => n]}</xml>}/>
    </body></xml>
