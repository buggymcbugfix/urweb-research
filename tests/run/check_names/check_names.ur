(* checkEnvVar and checkResponseHeader on names known only at run time, from
   the URL, against the .urp's allow rules; and on literals, which the
   compiler decides itself. *)

fun check (kind : string) (name : string) =
    return <xml><body>{[kind]} {[name]}: {[case kind of
                                               "env" => Option.isSome (checkEnvVar name)
                                             | _ => Option.isSome (checkResponseHeader name)]}</body></xml>

fun literal () =
    return <xml><body>
      env TEST_VAR: {[Option.isSome (checkEnvVar "TEST_VAR")]},
      header X-Test: {[Option.isSome (checkResponseHeader "X-Test")]}
    </body></xml>
