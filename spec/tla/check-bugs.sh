#!/bin/sh
# One TLC run per Bug constant of Request.tla, with that constant alone set
# to TRUE in Request.cfg: the runtime as it was in that one respect. Each
# run has to end in a violated invariant, except for the constants listed
# as hardening below, which stand for a change no invariant can see on its
# own (README.md says why): their runs have to end clean, so that the
# claim stays checked. The first run that does otherwise fails the
# script. The logs go to out/bug-<name>.log. The Makefile, which runs
# this, says how TLC is run (TLC).
set -e
: "${TLC:?the TLC command, set by the Makefile: run make bugs}"
cd "$(dirname "$0")"
mkdir -p out
hardening="BugFreeKeepsStack"
bugs=$(sed -n 's/^  \(Bug[A-Za-z]*\) = FALSE$/\1/p' Request.cfg)
[ -n "$bugs" ] || { echo "check-bugs: no Bug constants in Request.cfg" >&2; exit 2; }
for bug in $bugs; do
  sed "s/^  $bug = FALSE\$/  $bug = TRUE/" Request.cfg > "out/bug-$bug.cfg"
  if $TLC -metadir "out/bug-$bug" -config "out/bug-$bug.cfg" Request.tla > "out/bug-$bug.log" 2>&1; then
    case " $hardening " in
      *" $bug "*) echo "check-bugs: $bug: no violation alone, as documented"; continue ;;
    esac
    echo "check-bugs: $bug: TLC found no violation" >&2; exit 1
  fi
  if ! grep -q 'Invariant Invariants is violated' "out/bug-$bug.log"; then
    echo "check-bugs: $bug: TLC failed without a violation (see out/bug-$bug.log)" >&2; exit 1
  fi
  case " $hardening " in
    *" $bug "*) echo "check-bugs: $bug: a violation, where none alone was documented" >&2; exit 1 ;;
  esac
  echo "check-bugs: $bug: found"
done
