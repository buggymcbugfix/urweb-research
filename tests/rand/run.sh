#!/bin/sh
# What the runtime does when OpenSSL's RAND_bytes fails.  shim.c, preloaded,
# makes it fail under a compiled app; the app's answer, the server's log and
# a startup are diffed against `expected`.  The in-tree compiler is used, so
# run `make` first.  `run.sh -u` accepts the new output.
set -eu

here=$(cd "$(dirname "$0")" && pwd)
top=$here/../..
update=0
[ "${1:-}" = -u ] && update=1

tmp=$(mktemp -d)
pid=""
cleanup() {
  [ -z "$pid" ] || kill "$pid" 2>/dev/null || true
  rm -rf "$tmp"
}
trap cleanup EXIT INT TERM

${CC:-cc} -shared -fPIC -o "$tmp/shim.so" "$here/shim.c"

cp "$here/rand.ur" "$tmp/"
printf 'sigfile %s/sig\n\nrand\n' "$tmp" > "$tmp/rand.urp"
"$top/bin/urweb" -boot -noEmacs "$tmp/rand"

# With a signature file already there, startup does not ask OpenSSL for a
# password; without one it does, and the third case below has none.
head -c 16 /dev/zero > "$tmp/sig"

# serve MODE: start the app with the shim in MODE; sets $pid and $port.
serve() {
  eval "$(RAND_SHIM=$1 LD_PRELOAD=$tmp/shim.so "$tmp/rand.exe" -q -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>/dev/null 2>"$tmp/log")"
  [ "$status" = OK ] || { echo "rand.exe failed to start" >&2; exit 1; }
}

# request: the page, then the status line; then the server's log, once stopped.
request() {
  curl -s -w '\n%{http_code}\n' "http://127.0.0.1:$port/Rand/main"
  kill "$pid"
  pid=""
  echo "-- log"
  cat "$tmp/log"
}

{
  echo "== RAND_bytes returns 0, with a reason"
  serve fail
  request

  echo
  echo "== RAND_bytes returns -1"
  serve unsupported
  request

  echo
  echo "== RAND_bytes returns 0 on startup, without a signature file"
  rm -f "$tmp/sig"
  status=0
  RAND_SHIM=fail LD_PRELOAD=$tmp/shim.so "$tmp/rand.exe" -q -a 127.0.0.1 -p 8000 -P 9000 2>&1 || status=$?
  echo "exit status $status"
} > "$tmp/actual"

if [ $update = 1 ]; then
  cp "$tmp/actual" "$here/expected"
else
  diff -u "$here/expected" "$tmp/actual" || { echo "tests/rand failed" >&2; exit 1; }
fi
