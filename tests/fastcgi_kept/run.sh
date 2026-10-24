#!/usr/bin/env bash
#
# A FastCGI program and the connections a web server opens to it
# (client.py): many kept open at once, one after the other and closed, two
# requests in one write, and the peers FCGI_WEB_SERVER_ADDRS lets in.  The
# transcript says what became of each and is compared with `expected`.
#
#   tests/fastcgi_kept/run.sh [-u]      -u rewrites expected
#
# Run from the root of the tree (make check-fastcgi-kept). The compiler, -u
# and the exit status are as tests/lib.sh says. Needs python3.

. "$(dirname "$0")/../lib.sh"
no_cases "$@"
needs python3
needs_urweb
fresh_out

rm -f "$here/app/kept.exe"
build build.log -protocol fastcgi tests/fastcgi_kept/app/kept

# The client's own failures (a Python error) are not part of the transcript:
# the golden is only ever written from a run that ended normally.
python3 "$here/client.py" "$here/app/kept.exe" "$out" > "$out/transcript" 2> "$out/client.log" \
  || { cat "$out/client.log" >&2; die "the client failed"; }
compare "" "$here/expected" "$out/transcript"
exit $failed
