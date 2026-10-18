#!/usr/bin/env bash
#
# A FastCGI program with its default single thread, and two connections
# to it (client.py): one kept open and idle, as a web server with
# keep-alive leaves a connection after a request, and a second one with
# a request on it while the first idles.  The transcript says whether the
# second is answered, and is compared with `expected`.
#
#   tests/fastcgi_idle/run.sh [-u]      -u rewrites expected
#
# Run from the root of the tree (make check-fastcgi-idle): the compiler is
# the one URWEB names, with the flags in URWEB_FLAGS, or else bin/urweb
# with -boot -noEmacs.  Needs python3 and spawn-fcgi.  Exit status: 0
# passed, 1 failed, 2 could not run.

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$here/out
urweb=${URWEB:-$root/bin/urweb}
urweb_flags=${URWEB_FLAGS:--boot -noEmacs}

update=0
if [ "${1:-}" = "-u" ]; then update=1; shift; fi

die() { echo "run.sh: $*" >&2; exit 2; }

for tool in python3 spawn-fcgi; do
  command -v "$tool" >/dev/null || die "$tool is needed"
done
[ -x "$urweb" ] || die "no compiler at $urweb: build it (make mlton) or set URWEB"

rm -rf "$out"
mkdir -p "$out"

( cd "$root" && rm -f tests/fastcgi_idle/app/idle.exe \
    && "$urweb" $urweb_flags -protocol fastcgi tests/fastcgi_idle/app/idle ) \
  > "$out/build.log" 2>&1 || { cat "$out/build.log" >&2; die "the application failed to build"; }

# The program on a socket under out/, as spawn-fcgi starts one, with only
# what is named here in its environment: no URWEB_NUM_THREADS, so one
# worker thread.  The socket is short of a Unix socket's path limit when
# the tree is; TMPDIR is the way around that.
sock=$out/idle.sock
[ ${#sock} -lt 100 ] || { sock=${TMPDIR:-/tmp}/fastcgi_idle.$$.sock; rm -f "$sock"; }
env -i ${LD_LIBRARY_PATH:+LD_LIBRARY_PATH="$LD_LIBRARY_PATH"} PATH="$PATH" \
    ${ASAN_OPTIONS:+ASAN_OPTIONS="$ASAN_OPTIONS"} ${UBSAN_OPTIONS:+UBSAN_OPTIONS="$UBSAN_OPTIONS"} \
    spawn-fcgi -n -s "$sock" -- "$here/app/idle.exe" -q > "$out/program.log" 2>&1 &
app_pid=$!
cleanup() { kill "$app_pid" 2>/dev/null; rm -f "$sock"; }
trap cleanup EXIT
for _ in $(seq 100); do
  [ -S "$sock" ] && break
  sleep 0.05
done
[ -S "$sock" ] || { cat "$out/program.log" >&2; die "the program did not start"; }

python3 "$here/client.py" "$sock" > "$out/transcript" 2>&1
if [ $update = 1 ]; then
  cp "$out/transcript" "$here/expected"
  echo "fastcgi_idle: updated"
elif diff -u "$here/expected" "$out/transcript" > "$out/diff"; then
  echo "fastcgi_idle: ok"
else
  echo "fastcgi_idle: FAILED" >&2; cat "$out/diff" >&2; exit 1
fi
