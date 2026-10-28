#!/usr/bin/env bash
#
# Where a program's log lines go, with each of the flags that choose them
# and with standard error connected to the journal: a FastCGI, an HTTP, a
# CGI and a static build of one application (client.py starts them and
# sends the requests). The transcript is compared with `expected`.
#
#   tests/log_levels/run.sh [-u]      -u rewrites expected
#
# Run from the root of the tree (make check-log-levels). The compiler, -u
# and the exit status are as tests/lib.sh says. Needs python3.

. "$(dirname "$0")/../lib.sh"
no_cases "$@"
needs python3
needs_urweb
fresh_out

for protocol in fastcgi http cgi static; do
  build "build-$protocol.log" -protocol "$protocol" -output "$out/levels-$protocol.exe" tests/log_levels/app/levels
done

python3 "$here/client.py" "$out/levels-fastcgi.exe" "$out/levels-http.exe" "$out/levels-cgi.exe" \
  "$out/levels-static.exe" "$out" > "$out/transcript" 2> "$out/client.log" \
  || { cat "$out/client.log" >&2; die "the client failed"; }
compare "" "$here/expected" "$out/transcript"
exit $failed
