(* Probes of the context heap, from C. *)

(* A single block of that many bytes from uw_malloc, written end to end:
   whether it is word-aligned and could be written. *)
val block : int -> transaction string

(* That many blocks of odd sizes back to back: whether every address is
   word-aligned and no block overlaps the one before it. *)
val odd : int -> transaction string

(* Allocations at a page boundary: one that ends exactly on the next
   boundary, then one byte, which starts a fresh page. *)
val pageBoundary : transaction string

(* uw_malloc of size zero: non-NULL, and takes no room. *)
val zero : transaction string

(* The context's buffer statistics (uw_memstats), in the log. *)
val stats : transaction unit
