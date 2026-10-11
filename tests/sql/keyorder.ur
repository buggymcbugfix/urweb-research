(* Composite keys must come out in the order they are declared.  Each list
   below is in an order that is neither alphabetical nor the column order,
   so that a rotation can be told from a reversal.  Ref is Uni written out
   by hand: the order the others should have. *)

table u : { X : int, Y : string, Z : int }
  PRIMARY KEY (Z, X, Y)

table t : { A : int, B : string, C : int, D : int }
  PRIMARY KEY (C, A, B),
  CONSTRAINT Uni UNIQUE (D, C, A),
  CONSTRAINT Man UNIQUE ({#D}, {#C}, {#A}),
  CONSTRAINT Ref {unique [#D] [[C = _, A = _]]},
  CONSTRAINT Fk FOREIGN KEY (A, B, C) REFERENCES u (X, Y, Z)

fun main () : transaction page =
    queryI (SELECT * FROM t) (fn _ => return ());
    return <xml/>
