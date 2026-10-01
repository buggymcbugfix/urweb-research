## High-level overview.

Central idea:

```
return : t -> eff everywhere [] t
bind : eff where1 what1 t1 -> (t1 -> eff where2 what2 t2) -> eff (where1 ^ where2) (what1 & what2) t2
```

Rough sketch:

```
con client = [Client] (* I'm not sure if Signal fits the pattern. *)
con server = [PageHandler, RpcHandler, Task]
con transaction = [Txn]
con everywhere = client ++ server ++ transaction

val debug : eff [Client, Server, Task, Txn] [Write = [Stderr]] unit

con DB :: Kind = [Read = tbls, Update = tbls, Insert = tbls, Delete = tbls, Clock = bool]

val runTransaction :
  db ::: DB -> 
  t ::: Type -> 
  eff [Txn] [Db = db] t -> 
  eff [Server, Task] [Db = db] t

val now : eff everywhere [Clock] time

val example : eff [Server, Task] [Db = [Read = {T}, Update = {}, Insert = {}, Delete = {}, Clock = False], Clock, Write = [Stderr]] unit =
  foos <- runTransaction (queryL1 (SELECT t.Foo FROM t));
  time <- now;
  debug ("Current date and time: " ^ show now);
  return {}

```

## Motivation.

### Principled & intuitive sidedness-check.

> The most mentioned pain point was the way that the Ur/Web compiler checks that server-side code does not call functions that only make sense in the client, or vice versa. In designing the language, we were worried that it would be too complex to extend the type system to track such constraints, tagging monadic computation types with sidedness information. However, the approach we adopted instead, with ad-hoc static analysis on whole programs at compile time, leads to error messages that confuse even experienced Ur/Web programmers. In general, improvements to error messages and performance of the compiler (which does specialized whole-program optimization) are high on our list of future priorities.
— Chlipala (POPL 2015)

### Effect-tracking.

To help both the compiler and the programmer.

> A key enabling optimization for fusion is a lightweight effect analysis on an imperative intermediate language. Ur/Web programs like the one in Figure 1 are naturally written in a monadic style, where, e.g., a database query might be performed at a point in the code fairly far from where the results are injected into a result HTML document. Moving the query closer to the injection point enables fusion. We summarize program subexpressions with effects to determine when portions commute with each other, enabling useful code motion.
— Chlipala (ICFP 2015)

### Reasoning about data structures.

Indexing HTML by whether it contains `<script>`, i.e. whether it can be serialized or not.

### Correct-by-construction HTML.

Represent the specs direct in code.