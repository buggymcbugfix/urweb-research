#!/bin/sh
#
# The formatter against the real compiler: for each file given, the
# compiler's parse of the file and of its formatted form must be the same
# program (`urweb -stop parse` prints the desugared AST, positions
# excluded, so two files that print the same are the same program), every
# comment must survive, and formatting the result again must change
# nothing.  One line per file, `ok` or `FAIL` with the reason, then a
# summary; exit status 1 if any file failed, 2 if it could not run.
#
#   urt/tests/format/oracle.sh FILE.ur...
#
# URWEB names the compiler, run with the flags in URWEB_FLAGS if that is
# set; otherwise it is the in-tree build, bin/urweb, run with -boot
# -noEmacs (or URWEB_FLAGS).  FMT names the formatter (default:
# urt/urt-format).  The compiler must accept every file: an input that it
# rejects is a failure too.

set -u
me=${0##*/}
here=$(cd "$(dirname "$0")" && pwd -P) || exit 2
if [ -n "${URWEB-}" ]; then
    UFLAGS=${URWEB_FLAGS-}
else
    URWEB=$here/../../../bin/urweb
    UFLAGS=${URWEB_FLAGS--boot -noEmacs}
fi
case $URWEB in
    /*) ;;
    *) URWEB=$(pwd -P)/$URWEB ;;   # it runs elsewhere
esac
FMT=${FMT:-$here/../../urt-format}
[ -x "$URWEB" ] || { echo "$me: no compiler at $URWEB" >&2; exit 2; }
[ -x "$FMT" ] || { echo "$me: no formatter at $FMT" >&2; exit 2; }
[ $# -gt 0 ] || { echo "usage: $me FILE.ur..." >&2; exit 2; }

tmp=$(mktemp -d) || exit 2
trap 'rm -rf "$tmp"' EXIT INT TERM
ok=0 failed=0

# parse SIDE: the compiler's parse of the project under $tmp/SIDE, as it
# prints it, into $tmp/SIDE.raw, and with the positions stripped into
# $tmp/SIDE.dump; what it complained about into $tmp/SIDE.err
parse() {
    # shellcheck disable=SC2086 # the flags are words
    (cd "$tmp/$1" && "$URWEB" $UFLAGS -stop parse "$m" > "$tmp/$1.raw" 2> "$tmp/$1.err")
    sed -E 's#^[^ ]+:[0-9]+:[0-9]+: \(to [0-9]+:[0-9]+\) ##' "$tmp/$1.raw" > "$tmp/$1.dump"
}

# The comments of a file, one per line, continuation lines' indentation
# dropped (the formatter may re-indent those).
comments() {
    "$FMT" -comments "$1" | sed -E 's/(\\n)(\\t| )+/\1/g'
}

fail() {
    echo "FAIL $f ($1)"
    failed=$((failed + 1))
}

for f in "$@"; do
    b=${f##*/}
    m=${b%.ur*}
    case $b in
        *.urs) ext=urs ;;
        *) ext=ur ;;
    esac
    for side in a b; do
        rm -rf "${tmp:?}/$side"
        mkdir -p "$tmp/$side"
        echo "$m" > "$tmp/$side/$m.urp"
        # a .urs needs a .ur next to it; an empty one will do for parsing
        [ $ext = ur ] || : > "$tmp/$side/$m.ur"
    done
    # the compiler must accept the input as it is
    cp "$f" "$tmp/a/$b"
    parse a
    if grep -q '^Parse failure$' "$tmp/a.err"; then
        fail "the compiler rejects the input: $(grep -m1 -E ':[0-9]+:[0-9]+: ' "$tmp/a.err")"
        continue
    fi
    if ! "$FMT" "$f" > "$tmp/formatted.$ext" 2> "$tmp/fmt.err"; then
        fail "the formatter rejects a file the compiler accepts: $(head -1 "$tmp/fmt.err")"
        continue
    fi
    # canonical forms of both sides (comments out, XML whitespace merged),
    # since the compiler's parse of the two must agree modulo those
    "$FMT" -canonical "$f" > "$tmp/a/$b"
    "$FMT" -canonical "$tmp/formatted.$ext" > "$tmp/b/$b"
    parse a
    parse b
    if ! cmp -s "$tmp/a.dump" "$tmp/b.dump"; then
        fail "parse differs"
        diff "$tmp/a.dump" "$tmp/b.dump" | head -n "${DIFFLINES:-6}"
        continue
    fi
    comments "$f" > "$tmp/a.comments"
    comments "$tmp/formatted.$ext" > "$tmp/b.comments"
    if ! cmp -s "$tmp/a.comments" "$tmp/b.comments"; then
        fail "comments differ"
        diff "$tmp/a.comments" "$tmp/b.comments" | head -n "${DIFFLINES:-6}"
        continue
    fi
    if ! "$FMT" "$tmp/formatted.$ext" > "$tmp/again.$ext" 2>/dev/null || ! cmp -s "$tmp/formatted.$ext" "$tmp/again.$ext"; then
        fail "not idempotent"
        diff "$tmp/formatted.$ext" "$tmp/again.$ext" | head -n "${DIFFLINES:-6}"
        continue
    fi
    echo "ok $f"
    ok=$((ok + 1))
done
echo "oracle: $((ok + failed)) files, $failed failed"
[ $failed = 0 ]
