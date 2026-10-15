(* The monad of a task body named through aliases and wrappers: a con
   standing for [io] or [transaction], a polymorphic wrapper for [periodic],
   and a module's own alias of the alias.  Each kind comes out by the
   monad of its body, io or transaction, as the table of tasks in the C
   shows. *)

con myio = io
con mytr = transaction

table t : { N : int }

fun every [m ::: Type -> Type] (n : int) : task_kind unit m = periodic n

structure M = struct
    con io2 = myio
    fun kind (n : int) : task_kind unit io2 = every n
end

task every 7 = fn () => (runTransaction (dml (INSERT INTO t (N) VALUES (1))) : myio unit)
task M.kind 9 = fn () => io_debug "through a module's alias"
task every 11 = fn () => (dml (INSERT INTO t (N) VALUES (2)) : mytr unit)
task initialize = fn () => (return () : mytr unit)

fun main () : transaction page = return <xml><body>up</body></xml>
