(* Rows of kind {Unit} as sets of names.
 *
 * A first-order syntax for {Unit} rows built from row variables (by index),
 * literal names, [], ++ and --; their meaning, as Semantics.v gives it; and
 * a decision procedure for their equality, sound and complete for that
 * meaning.  Syntax.v turns it into the rule Eq_Unit.
 *
 * Every operation acts name by name, so at one name a term's value depends
 * only on which variables hold that name and on which literal, if any, the
 * name is.  decide compares the two sides at every literal of either side
 * and at one name that is none of them, for every in/out pattern of the
 * variables: exponential in the number of variables, which in a type is
 * small. *)

From Stdlib Require Import List Bool String Ascii Lia.
Import ListNotations.

Require Import Name.

Set Implicit Arguments.


Inductive uexp : Type :=
| UVar : nat -> uexp
| UName : name -> uexp
| UEmpty : uexp
| UConcat : uexp -> uexp -> uexp
| UDiff : uexp -> uexp -> uexp.

(* The meaning: Semantics.v's cDen at {Unit}, clause for clause. *)
Fixpoint eval (rho : nat -> name -> option unit) (e : uexp) : name -> option unit :=
  match e with
  | UVar i => rho i
  | UName s => fun n => if name_eq_dec n s then Some tt else None
  | UEmpty => fun _ => None
  | UConcat e1 e2 => fun n => match eval rho e1 n with
                              | None => eval rho e2 n
                              | v => v
                              end
  | UDiff e1 e2 => fun n => match eval rho e2 n with
                            | None => eval rho e1 n
                            | Some _ => None
                            end
  end.

(* At one name: bits says which variables hold it, lit is the name itself,
 * or None for a name that is no literal of the terms compared. *)
Fixpoint evalb (bits : nat -> bool) (lit : option name) (e : uexp) : bool :=
  match e with
  | UVar i => bits i
  | UName s => match lit with
               | Some n => if name_eq_dec n s then true else false
               | None => false
               end
  | UEmpty => false
  | UConcat e1 e2 => evalb bits lit e1 || evalb bits lit e2
  | UDiff e1 e2 => evalb bits lit e1 && negb (evalb bits lit e2)
  end.

Fixpoint literals (e : uexp) : list name :=
  match e with
  | UVar _ | UEmpty => []
  | UName s => [s]
  | UConcat e1 e2 | UDiff e1 e2 => literals e1 ++ literals e2
  end.

(* One more than the largest variable index. *)
Fixpoint width (e : uexp) : nat :=
  match e with
  | UVar i => S i
  | UName _ | UEmpty => 0
  | UConcat e1 e2 | UDiff e1 e2 => max (width e1) (width e2)
  end.

Fixpoint patterns (w : nat) : list (list bool) :=
  match w with
  | O => [[]]
  | S w => map (cons true) (patterns w) ++ map (cons false) (patterns w)
  end.

Definition decide (e1 e2 : uexp) : bool :=
  forallb (fun lit =>
             forallb (fun p => Bool.eqb (evalb (fun i => nth i p false) lit e1)
                                        (evalb (fun i => nth i p false) lit e2))
                     (patterns (max (width e1) (width e2))))
          (None :: map Some (literals e1 ++ literals e2)).


Lemma eval_evalb : forall rho e n,
  eval rho e n = if evalb (fun i => if rho i n then true else false) (Some n) e
                 then Some tt else None.
  intros rho e; induction e; simpl; intros;
    repeat match goal with
           | [ H : forall n, eval _ ?e n = _ |- context[eval _ ?e ?n] ] => rewrite (H n)
           end;
    repeat match goal with
           | [ |- context[evalb ?b ?l ?e] ] => destruct (evalb b l e)
           | [ |- context[rho ?i ?n] ] => destruct (rho i n) as [ [] | ]
           | [ |- context[name_eq_dec ?n ?s] ] => destruct (name_eq_dec n s)
           end; reflexivity.
Qed.

Lemma evalb_ext : forall b1 b2 lit e,
  (forall i, i < width e -> b1 i = b2 i)
  -> evalb b1 lit e = evalb b2 lit e.
  induction e; simpl; intros; try reflexivity;
    [ apply H; lia
    | rewrite IHe1, IHe2 by (intros; apply H; lia); reflexivity
    | rewrite IHe1, IHe2 by (intros; apply H; lia); reflexivity ].
Qed.

Lemma evalb_nolit : forall bits n e,
  ~ In n (literals e)
  -> evalb bits (Some n) e = evalb bits None e.
  induction e; simpl; intros; try reflexivity;
    [ destruct (name_eq_dec n n0); [ subst; tauto | reflexivity ]
    | rewrite in_app_iff in H; rewrite IHe1, IHe2 by tauto; reflexivity
    | rewrite in_app_iff in H; rewrite IHe1, IHe2 by tauto; reflexivity ].
Qed.

Lemma patterns_complete : forall p, In p (patterns (List.length p)).
  induction p as [ | b p ]; simpl; [ tauto | ].
  apply in_or_app; destruct b; [ left | right ]; apply in_map; assumption.
Qed.

(* A name that is none of a list's: longer than all of them. *)
Fixpoint rep (k : nat) : string :=
  match k with
  | O => EmptyString
  | S k => String "a"%char (rep k)
  end.

Definition fresh (l : list name) : name :=
  rep (S (fold_right (fun s m => max (String.length s) m) 0 l)).

Lemma fresh_not_in : forall l, ~ In (fresh l) l.
  assert (Hrep : forall k, String.length (rep k) = k)
    by (induction k; simpl; congruence).
  assert (Hle : forall s l, In s l
                -> String.length s <= fold_right (fun s m => max (String.length s) m) 0 l)
    by (induction l; simpl; intuition; subst; lia).
  unfold fresh; intros l H; apply Hle in H; rewrite Hrep in H; lia.
Qed.


(* The two sides of decide at one name, for e1 or e2. *)
Lemma evalb_at : forall bits p w L n e,
  width e <= w
  -> incl (literals e) L
  -> (forall i, i < w -> nth i p false = bits i)
  -> evalb bits (Some n) e
     = evalb (fun i => nth i p false)
             (if in_dec name_eq_dec n L then Some n else None) e.
  intros; destruct (in_dec _ _ _);
    [ | rewrite (evalb_nolit _ _ e) by auto ];
    apply evalb_ext; intros; symmetry; apply H1; lia.
Qed.

Theorem decide_sound : forall e1 e2,
  decide e1 e2 = true
  -> forall rho n, eval rho e1 n = eval rho e2 n.
  unfold decide; intros e1 e2 H rho n.
  set (w := max (width e1) (width e2)) in *.
  set (bits := fun i => if rho i n then true else false).
  set (p := map bits (seq 0 w)).
  assert (Hp : forall i, i < w -> nth i p false = bits i).
  { intros; unfold p; rewrite nth_indep with (d' := bits 0)
      by (rewrite length_map, length_seq; assumption).
    rewrite map_nth, seq_nth by assumption; reflexivity. }
  set (lit := if in_dec name_eq_dec n (literals e1 ++ literals e2) then Some n else None).
  assert (Hlit : In lit (None :: map Some (literals e1 ++ literals e2)))
    by (unfold lit; destruct (in_dec _ _ _); simpl; auto using in_map).
  assert (Hpat : In p (patterns w))
    by (replace w with (List.length p) by (unfold p; rewrite length_map, length_seq; reflexivity);
        apply patterns_complete).
  rewrite forallb_forall in H; specialize (H lit Hlit).
  rewrite forallb_forall in H; specialize (H p Hpat).
  apply Bool.eqb_prop in H; unfold lit in H.
  rewrite !eval_evalb; fold bits.
  rewrite (@evalb_at bits p w (literals e1 ++ literals e2) n e1),
          (@evalb_at bits p w (literals e1 ++ literals e2) n e2), H;
    auto using incl_appl, incl_appr, incl_refl; unfold w; lia.
Qed.

(* The valuation that decide's pattern p describes at the name n. *)
Definition at_name (p : list bool) (n : name) : nat -> name -> option unit :=
  fun i m => if name_eq_dec m n then if nth i p false then Some tt else None else None.

Lemma eval_at_name : forall p n e,
  eval (at_name p n) e n = if evalb (fun i => nth i p false) (Some n) e then Some tt else None.
  intros; rewrite eval_evalb; erewrite evalb_ext; [ reflexivity | ].
  intros; unfold at_name; destruct (name_eq_dec n n); [ | congruence ].
  destruct (nth i p false); reflexivity.
Qed.

Theorem decide_complete : forall e1 e2,
  (forall rho n, eval rho e1 n = eval rho e2 n)
  -> decide e1 e2 = true.
  unfold decide; intros e1 e2 H.
  apply forallb_forall; intros lit _.
  apply forallb_forall; intros p _.
  apply Bool.eqb_true_iff.
  destruct lit as [ s | ].
  - generalize (H (at_name p s) s); rewrite !eval_at_name.
    destruct (evalb _ _ e1), (evalb _ _ e2); congruence.
  - generalize (H (at_name p (fresh (literals e1 ++ literals e2))) (fresh (literals e1 ++ literals e2))).
    rewrite !eval_at_name, !evalb_nolit
      by (intro Hin; apply (fresh_not_in (literals e1 ++ literals e2)); apply in_or_app; auto).
    destruct (evalb _ _ e1), (evalb _ _ e2); congruence.
Qed.
