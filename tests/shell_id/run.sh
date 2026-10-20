#!/usr/bin/env bash
#
# What the id of the toolchain in shell.nix reacts to.  urweb_configure
# reconfigures the tree, which means make distclean and deleting out/, when
# the id changes.  The id should change for what configure sees and for
# nothing else.  Each case below edits a copy of the files that define the
# shell, evaluates the id of the shell, and records whether it moved.
#
#   tests/shell_id/run.sh [-u]       -u rewrites expected
#
# A case that records "same" for an edit configure would see is a gap in the
# id, and is pinned as it is: see the comments in the list.
#
# Run from anywhere in the tree (make check-shell-id).  Needs nix-instantiate
# and git, outside nix-shell and nix-build.  Exit status: 0 as expected, 1
# not, 2 could not run.

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
files="shell.nix derivation.nix nixpkgs.nix"

update=0
if [ "${1:-}" = "-u" ]; then update=1; shift; fi

die() { echo "run.sh: $*" >&2; exit 2; }

for tool in nix-instantiate git sed cmp; do
  command -v "$tool" >/dev/null || die "$tool is needed"
done

tmp=$(mktemp -d) || die "no temporary directory"
tree=$tmp/tree
cleanup() { rm -rf "$tmp"; }
trap cleanup EXIT INT TERM

# The tree the shell is evaluated in: a checkout of HEAD, since the package
# reads the tracked files, with the files above as they are in the working
# tree.
git clone -q --shared --no-checkout "$root" "$tree" &&
  git -C "$tree" checkout -q --detach "$(git -C "$root" rev-parse HEAD)" ||
  die "cannot check out HEAD"
for f in $files; do cp "$root/$f" "$tree/$f"; done

# The id, which shell.nix puts in front of the prefix in the shell's hook.
id() {
  (cd "$tree" && nix-instantiate --eval --raw \
    -E '(import ./shell.nix {}).shellHook' 2>"$tmp/err") |
    sed -n 's/.*_urweb_id="\([^ ]*\) .*/\1/p'
}

base=$(id)
[ -n "$base" ] || { cat "$tmp/err" >&2; die "no id: could not evaluate shell.nix"; }

# case NAME FILE SED-EXPRESSION: apply the edit, compare the id, undo it.
case_() {
  local name=$1 file=$2 expr=$3 new
  cp "$tree/$file" "$tmp/saved"
  sed -i "$expr" "$tree/$file"
  cmp -s "$tree/$file" "$tmp/saved" && die "case '$name': the edit changed nothing"
  new=$(id)
  [ -n "$new" ] || { cat "$tmp/err" >&2; die "case '$name': no id"; }
  if [ "$new" = "$base" ]; then echo "same     $name"; else echo "changes  $name"; fi
  cp "$tmp/saved" "$tree/$file"
}

run() {
  # What only nix-build runs, or what is not an input: the id stays.
  case_ 'postPatch added to the package'            derivation.nix '/^  doCheck = true;$/a\  postPatch = "true";'
  case_ 'version of the package bumped'             derivation.nix 's/^  version = "20200209";$/  version = "20200210";/'
  case_ 'preConfigure of the package edited'        derivation.nix 's/^    \.\/autogen\.sh$/    .\/autogen.sh; true/'
  case_ 'env added to the package'                  derivation.nix '/^  doCheck = true;$/a\  env.FOO = "1";'
  case_ 'Makefile.am edited'                        Makefile.am    '$a# edit'
  case_ 'comment in the shell hook edited'          shell.nix      's/Run `repl` to rebuild/Run `repl` to build/'
  # What configure sees through the package: the id changes.
  case_ 'flag added to CCARGS in configureEnv'      derivation.nix 's/-Wno-error=int-conversion"/-Wno-error=int-conversion -Wno-foo"/'
  case_ 'python3 dropped from the check inputs'     derivation.nix "/^    python3 # tests\\/io_curl/d"
  case_ 'icu dropped from the build inputs'         derivation.nix '/^  buildInputs = \[$/{n;/^    icu$/d}'
  case_ 'mlton changed in derivation.nix'           derivation.nix 's/-intbinaryset-fix"/-intbinaryset-fix2"/'
  # What configure sees through the shell's own packages: the id changes,
  # also for tools configure never looks for (rlwrap, smlnj).
  case_ 'rlwrap dropped from the shell packages'    shell.nix      '/^      rlwrap$/d'
  case_ 'smlnj dropped from the shell packages'     shell.nix      '/^      smlnj$/d'
  # What configure sees and the id does not cover.
  # The configure command is in the hook; after changing it the tree has to
  # be cleaned by hand.
  case_ 'configure command in the hook changed'     shell.nix      's/\.\/configure --prefix="\$DEVPREFIX"/& --disable-shared/'
  # Only inputsFrom and packages of shellInputs are in the id; an input
  # given to the shell itself is not.
  case_ 'input added to the shell outside shellInputs' shell.nix   '/^  inherit (shellInputs) inputsFrom packages;$/a\  buildInputs = [ pkgs.hello ];'
}

run >"$tmp/got" || exit 2

if [ "$update" = 1 ]; then
  cp "$tmp/got" "$here/expected"
  echo "rewrote $here/expected"
  exit 0
fi
if diff -u "$here/expected" "$tmp/got"; then
  echo "shell_id: ok"
else
  echo "shell_id: FAILED"
  exit 1
fi
