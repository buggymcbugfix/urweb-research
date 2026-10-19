#!/usr/bin/env bash
#
# A FastCGI program and the connections a web server opens to it
# (client.py): many kept open at once, one after the other and closed, two
# requests in one write, and the peers FCGI_WEB_SERVER_ADDRS lets in.  The
# transcript says what became of each and is compared with `expected`.
#
#   tests/fastcgi_kept/run.sh [-u]      -u rewrites expected
#
# Run from the root of the tree (make check-fastcgi-kept): the compiler is
# the one URWEB names, with the flags in URWEB_FLAGS, or else bin/urweb
# with -boot -noEmacs.  Needs python3.  Exit status: 0 passed, 1 failed, 2
# could not run.

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$here/out
urweb=${URWEB:-$root/bin/urweb}
urweb_flags=${URWEB_FLAGS:--boot -noEmacs}

die() { echo "run.sh: $*" >&2; exit 2; }

update=0
case "${1:-}" in
  "") ;;
  -u) update=1 ;;
  *) die "usage: run.sh [-u]" ;;
esac

command -v python3 >/dev/null || die "python3 is needed"
[ -x "$urweb" ] || die "no compiler at $urweb: build it (make mlton) or set URWEB"

rm -rf "$out"
mkdir -p "$out"

( cd "$root" && rm -f tests/fastcgi_kept/app/kept.exe \
    && "$urweb" $urweb_flags -protocol fastcgi tests/fastcgi_kept/app/kept ) \
  > "$out/build.log" 2>&1 || { cat "$out/build.log" >&2; die "the application failed to build"; }

# The client's own failures (a Python error) are not part of the transcript:
# the golden is only ever written from a run that ended normally.
python3 "$here/client.py" "$here/app/kept.exe" "$out" > "$out/transcript" 2> "$out/client.log" \
  || { cat "$out/client.log" >&2; die "the client failed"; }
if [ $update = 1 ]; then
  cp "$out/transcript" "$here/expected"
  echo "fastcgi_kept: updated"
elif diff -u "$here/expected" "$out/transcript" > "$out/diff"; then
  echo "fastcgi_kept: ok"
else
  echo "fastcgi_kept: FAILED" >&2; cat "$out/diff" >&2; exit 1
fi
