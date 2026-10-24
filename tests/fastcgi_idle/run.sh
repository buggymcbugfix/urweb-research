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
# Run from the root of the tree (make check-fastcgi-idle). The compiler, -u
# and the exit status are as tests/lib.sh says. Needs python3 and
# spawn-fcgi.

. "$(dirname "$0")/../lib.sh"
no_cases "$@"
needs python3 spawn-fcgi
needs_urweb
fresh_out

rm -f "$here/app/idle.exe"
build build.log -protocol fastcgi tests/fastcgi_idle/app/idle

# The program on a socket under out/, as spawn-fcgi starts one, with only
# what is named here in its environment: no URWEB_NUM_THREADS, so one
# worker thread.  The socket is short of a Unix socket's path limit when
# the tree is; TMPDIR is the way around that.
sock=$out/idle.sock
[ ${#sock} -lt 100 ] || { sock=${TMPDIR:-/tmp}/fastcgi_idle.$$.sock; rm -f "$sock"; }
"${clean_env[@]}" spawn-fcgi -n -s "$sock" -- "$here/app/idle.exe" -q > "$out/program.log" 2>&1 &
app_pid=$!
cleanup() { kill "$app_pid" 2>/dev/null; rm -f "$sock"; }
trap cleanup EXIT
for _ in $(seq 100); do
  [ -S "$sock" ] && break
  sleep 0.05
done
[ -S "$sock" ] || { cat "$out/program.log" >&2; die "the program did not start"; }

python3 "$here/client.py" "$sock" > "$out/transcript" 2>&1
compare "" "$here/expected" "$out/transcript"
exit $failed
