(* Row difference: what follows from CDiff and its rules.
 *
 * Sketch, alongside the design note "Finite sets in Ur: row difference and
 * Boolean unification".  The syntactic part shows that inter and union are
 * definable and that the ++ in union is always well formed, and derives
 * one concrete difference by the rules.  The semantic part shows that at
 * kind {Unit} the model's rows are finite sets: inter and union commute,
 * and difference behaves as set difference. *)

From Stdlib Require Import String.
Open Scope string_scope.

Require Import Axioms.
Require Import Syntax.
Require Import Semantics.

Set Implicit Arguments.


Section vars.
  Variable cvar : kind -> Type.
  Variable dvar : forall k, con cvar (KRecord k) -> con cvar (KRecord k) -> Type.

  (* As Top would define them:
   *   con inter = fn s1 s2 => s1 -- (s1 -- s2)
   *   con union = fn s1 s2 => s1 ++ (s2 -- s1) *)
  Definition inter k (s1 s2 : con cvar (KRecord k)) : con cvar (KRecord k) :=
    CDiff s1 (CDiff s1 s2).
  Definition union k (s1 s2 : con cvar (KRecord k)) : con cvar (KRecord k) :=
    CConcat s1 (CDiff s2 s1).

  (* union needs no guard: its ++ is between rows the rules know to be disjoint. *)
  Theorem union_disj : forall k (s1 s2 : con cvar (KRecord k)),
    disj dvar s1 (CDiff s2 s1).
    intros; apply DComm; apply DDiff.
  Qed.

  (* Congruence under ++, by Eq_Cong with a one-hole context. *)
  Lemma deq_concat_l : forall k (a a' b : con cvar (KRecord k)),
    deq dvar a a'
    -> deq dvar (CConcat a b) (CConcat a' b).
    intros; eapply Eq_Cong; [ eassumption
                            | apply S_CConcat; [ apply S_CVar | apply S_Unchanged ]
                            | apply S_CConcat; [ apply S_CVar | apply S_Unchanged ] ].
  Qed.

  Lemma deq_concat_r : forall k (a b b' : con cvar (KRecord k)),
    deq dvar b b'
    -> deq dvar (CConcat a b) (CConcat a b').
    intros; eapply Eq_Cong; [ eassumption
                            | apply S_CConcat; [ apply S_Unchanged | apply S_CVar ]
                            | apply S_CConcat; [ apply S_Unchanged | apply S_CVar ] ].
  Qed.

  (* [A = int, B = string] -- [B] = [A = int], by the rules alone. *)
  Example diff_computes : forall (tint tstring : con cvar KType) k' (c : con cvar k'),
    deq dvar (CDiff (CConcat (CSingle (Name cvar "A") tint) (CSingle (Name cvar "B") tstring))
                    (CSingle (Name cvar "B") c))
             (CSingle (Name cvar "A") tint).
    intros.
    eapply Eq_Trans; [ apply Eq_Diff_Concat | ].
    eapply Eq_Trans; [ apply deq_concat_l; apply Eq_Diff_Single_Out; discriminate | ].
    eapply Eq_Trans; [ apply deq_concat_r; apply Eq_Diff_Single_In | ].
    eapply Eq_Trans; [ apply Eq_Concat_Comm; [ exact (CEmpty _ _) | apply DComm; apply DEmpty ] | ].
    apply Eq_Concat_Empty.
  Qed.
End vars.


(* The model.  At any kind, inter keeps the fields of s1 whose names are in
 * s2, and union has every field of s1 and the fields of s2 that s1 lacks. *)

Theorem inter_den : forall k (s1 s2 : con kDen (KRecord k)) n,
  cDen (inter s1 s2) n = match cDen s2 n with
                         | None => None
                         | Some _ => cDen s1 n
                         end.
  simpl; intros; destruct (cDen s2 n); destruct (cDen s1 n); reflexivity.
Qed.

Theorem union_den : forall k (s1 s2 : con kDen (KRecord k)) n,
  cDen (union s1 s2) n = match cDen s1 n with
                         | None => cDen s2 n
                         | v => v
                         end.
  simpl; intros; destruct (cDen s1 n); reflexivity.
Qed.

(* At {Unit}, where a row is a finite set of names, inter and union are the
 * set operations: they commute, and difference is set difference. *)

Ltac units := repeat match goal with
                     | [ |- context[cDen ?S ?N] ] => destruct (cDen S N) as [ [] | ]
                     end; intros; try reflexivity; discriminate.

Theorem inter_comm : forall (s1 s2 : con kDen (KRecord KUnit)),
  cDen (inter s1 s2) = cDen (inter s2 s1).
  intros; apply ext_eq; intro n; simpl; units.
Qed.

Theorem union_comm : forall (s1 s2 : con kDen (KRecord KUnit)),
  cDen (union s1 s2) = cDen (union s2 s1).
  intros; apply ext_eq; intro n; simpl; units.
Qed.

Theorem diff_union : forall (s1 s2 : con kDen (KRecord KUnit)),
  cDen (CDiff (union s1 s2) s2) = cDen (CDiff s1 s2).
  intros; apply ext_eq; intro n; simpl; units.
Qed.

(* Symmetric difference, the writeup's form of an equation s = t as f = []. *)
Definition symdiff cvar k (s t : con cvar (KRecord k)) : con cvar (KRecord k) :=
  CConcat (CDiff s t) (CDiff t s).

Theorem symdiff_empty : forall (s t : con kDen (KRecord KUnit)),
  cDen (symdiff s t) = cDen (CEmpty _ _) <-> cDen s = cDen t.
  split; intros H; apply ext_eq; intro n; generalize (f_equal (fun r => r n) H); simpl; units.
Qed.
