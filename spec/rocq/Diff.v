(* Row difference: what follows from CDiff and its rules.
 *
 * Sketch, alongside the design note "Finite sets in Ur: row difference and
 * Boolean unification".  inter and union are definable from CDiff, and one
 * concrete difference is computed by the rules.  The laws are derivations
 * in deq, for rows built from any constructors.  At kind {Unit}, where rows
 * are sets of names, they come from Eq_Unit, whose decision procedure
 * Sets.v proves sound and complete: by deq_complete_unit, every equation of
 * [], ++ and -- at {Unit} that holds in the model is derivable.  Through a
 * row's names, map (fn _ => ()), the laws that only depend on names reach
 * differences at any kind: cutting in either order gives the same row, and
 * r -- r = []. *)

From Stdlib Require Import String List.
Import ListNotations.
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

  (* The ++ in union is between rows the rules know to be disjoint.  The spec
   * puts no guard on ++; the compiler does (elaborate.sml, D.prove on
   * every CConcat), and this is the obligation it would discharge. *)
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

  Lemma deq_diff_l : forall k k' (a a' : con cvar (KRecord k)) (b : con cvar (KRecord k')),
    deq dvar a a'
    -> deq dvar (CDiff a b) (CDiff a' b).
    intros; eapply Eq_Cong; [ eassumption
                            | apply S_CDiff; [ apply S_CVar | apply S_Unchanged ]
                            | apply S_CDiff; [ apply S_CVar | apply S_Unchanged ] ].
  Qed.

  Lemma deq_diff_r : forall k k' (a : con cvar (KRecord k)) (b b' : con cvar (KRecord k')),
    deq dvar b b'
    -> deq dvar (CDiff a b) (CDiff a b').
    intros; eapply Eq_Cong; [ eassumption
                            | apply S_CDiff; [ apply S_Unchanged | apply S_CVar ]
                            | apply S_CDiff; [ apply S_Unchanged | apply S_CVar ] ].
  Qed.


  (* {Unit}: every equation true in the model is derivable. *)
  Theorem deq_complete_unit : forall env e1 e2,
    (forall rho n, eval rho e1 n = eval rho e2 n)
    -> deq dvar (embed env e1) (embed env e2).
    intros; apply Eq_Unit; apply decide_complete; assumption.
  Qed.

  (* A skeleton's variables, from a list of rows. *)
  Definition vars (l : list (con cvar (KRecord KUnit))) (i : nat) : con cvar (KRecord KUnit) :=
    nth i l (CEmpty cvar KUnit).

  Ltac unit_law l e1 e2 := exact (@Eq_Unit cvar dvar (vars l) e1 e2 eq_refl).

  Theorem inter_comm : forall s1 s2 : con cvar (KRecord KUnit),
    deq dvar (inter s1 s2) (inter s2 s1).
    intros; unit_law [s1; s2] (UDiff (UVar 0) (UDiff (UVar 0) (UVar 1)))
                              (UDiff (UVar 1) (UDiff (UVar 1) (UVar 0))).
  Qed.

  Theorem union_comm : forall s1 s2 : con cvar (KRecord KUnit),
    deq dvar (union s1 s2) (union s2 s1).
    intros; unit_law [s1; s2] (UConcat (UVar 0) (UDiff (UVar 1) (UVar 0)))
                              (UConcat (UVar 1) (UDiff (UVar 0) (UVar 1))).
  Qed.

  Theorem diff_union : forall s1 s2 : con cvar (KRecord KUnit),
    deq dvar (CDiff (union s1 s2) s2) (CDiff s1 s2).
    intros; unit_law [s1; s2] (UDiff (UConcat (UVar 0) (UDiff (UVar 1) (UVar 0))) (UVar 1))
                              (UDiff (UVar 0) (UVar 1)).
  Qed.

  (* Symmetric difference, the writeup's form of an equation s = t as f = []. *)
  Definition symdiff k (s t : con cvar (KRecord k)) : con cvar (KRecord k) :=
    CConcat (CDiff s t) (CDiff t s).

  Lemma deq_symdiff_r : forall k (s t t' : con cvar (KRecord k)),
    deq dvar t t'
    -> deq dvar (symdiff s t) (symdiff s t').
    intros; eapply Eq_Cong; [ eassumption
      | apply S_CConcat; apply S_CDiff; solve [ apply S_CVar | apply S_Unchanged ]
      | apply S_CConcat; apply S_CDiff; solve [ apply S_CVar | apply S_Unchanged ] ].
  Qed.

  (* s = t iff their symmetric difference is [].  Right to left: s is
   * (t -- f) ++ (f -- t) at {Unit} for f the symmetric difference, by
   * Eq_Unit, and f = [] turns that into t. *)
  Theorem symdiff_empty : forall s t : con cvar (KRecord KUnit),
    deq dvar (symdiff s t) (CEmpty cvar KUnit) <-> deq dvar s t.
    split; intros H.
    - eapply Eq_Trans.
      { unit_law [s; t]
          (UVar 0)
          (UConcat (UDiff (UVar 1) (UConcat (UDiff (UVar 0) (UVar 1)) (UDiff (UVar 1) (UVar 0))))
                   (UDiff (UConcat (UDiff (UVar 0) (UVar 1)) (UDiff (UVar 1) (UVar 0))) (UVar 1))). }
      simpl; change (CConcat (CDiff s t) (CDiff t s)) with (symdiff s t).
      eapply Eq_Trans; [ apply Eq_Comm; apply (deq_symdiff_r t) | ].
      { apply Eq_Comm; exact H. }
      unit_law [t] (UConcat (UDiff (UVar 0) UEmpty) (UDiff UEmpty (UVar 0))) (UVar 0).
    - eapply Eq_Trans; [ apply (deq_symdiff_r s (Eq_Comm H)) | ].
      unit_law [s] (UConcat (UDiff (UVar 0) (UVar 0)) (UDiff (UVar 0) (UVar 0))) UEmpty.
  Qed.


  (* Any kind.  A difference depends only on the names of its right
   * operand (Eq_Diff_Map_R), and names live at {Unit}. *)

  Definition names k (c : con cvar (KRecord k)) : con cvar (KRecord KUnit) :=
    CApp (CApp (CMap cvar k KUnit) (@CAbs cvar k KUnit (fun _ => CUnit cvar))) c.

  Lemma diff_names : forall k k' (r : con cvar (KRecord k)) (a : con cvar (KRecord k')),
    deq dvar (CDiff r a) (CDiff r (names a)).
    intros; apply Eq_Comm; apply Eq_Diff_Map_R.
  Qed.

  (* Cutting in either order gives the same row, with no guard between the
   * two right operands. *)
  Theorem diff_comm : forall k ka kb (r : con cvar (KRecord k))
                             (a : con cvar (KRecord ka)) (b : con cvar (KRecord kb)),
    deq dvar (CDiff (CDiff r a) b) (CDiff (CDiff r b) a).
    intros.
    eapply Eq_Trans; [ apply deq_diff_l; apply diff_names | ].
    eapply Eq_Trans; [ apply diff_names | ].
    eapply Eq_Trans; [ apply Eq_Comm; apply Eq_Diff_Concat_R | ].
    eapply Eq_Trans; [ apply deq_diff_r | ].
    { unit_law [names a; names b] (UConcat (UVar 0) (UVar 1)) (UConcat (UVar 1) (UVar 0)). }
    eapply Eq_Trans; [ apply Eq_Diff_Concat_R | ].
    eapply Eq_Trans; [ apply Eq_Comm; apply diff_names | ].
    apply deq_diff_l; apply Eq_Comm; apply diff_names.
  Qed.

  Theorem diff_self : forall k (r : con cvar (KRecord k)),
    deq dvar (CDiff r r) (CEmpty cvar k).
    intros; apply Eq_Names_Empty.
    eapply Eq_Trans; [ apply Eq_Diff_Map | ].
    eapply Eq_Trans; [ apply diff_names | ].
    unit_law [names r] (UDiff (UVar 0) (UVar 0)) UEmpty.
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
