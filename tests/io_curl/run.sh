#!/usr/bin/env bash
#
# HTTP requests from io code, between transactions: an application (app/)
# using urweb-curl against a stub payment provider (httpd.py), driven the way
# a browser and a clock would, with the answers, the server's log and the
# database compared with what each case expects.
#
#   tests/io_curl/run.sh [-u] [CASE...]      all cases unless given; -u
#                                            rewrites what the cases expect
#
# A case is cases/NAME.sh, a script sourced with the helpers below, writing a
# transcript that is compared with cases/NAME.expected.  Each case gets the
# application started afresh, on a database of its own loaded from the
# schema and whatever the case seeds, and the provider's charges forgotten.
#
# Run from the root of the tree (make check-io-curl): the compiler is the
# one URWEB names, with the flags in URWEB_FLAGS, or else bin/urweb with
# -boot -noEmacs.  Needs python3 (the provider), curl and sqlite3.  Exit
# status: 0 every case passed, 1 some failed, 2 could not run.

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$here/out
urweb=${URWEB:-$root/bin/urweb}
urweb_flags=${URWEB_FLAGS:--boot -noEmacs}

update=0
if [ "${1:-}" = "-u" ]; then update=1; shift; fi

die() { echo "run.sh: $*" >&2; exit 2; }

for tool in python3 curl sqlite3; do
  command -v "$tool" >/dev/null || die "$tool is needed"
done
[ -x "$urweb" ] || die "no compiler at $urweb: build it (make mlton) or set URWEB"

rm -rf "$out"
mkdir -p "$out"

# The application, and the schema its database starts from.  The compiler
# runs from the root, so that the file names in its messages are as the
# snapshots have them.
( cd "$root" && rm -f tests/io_curl/app/curl_io.exe \
    && "$urweb" $urweb_flags -dbms sqlite -sql "$out/schema.sql" tests/io_curl/app/curl_io ) \
  > "$out/build.log" 2>&1 || { cat "$out/build.log" >&2; die "the application failed to build"; }
exe=$here/app/curl_io.exe

# The provider, until this script ends.
python3 "$here/httpd.py" > "$out/httpd.out" 2> "$out/httpd.err" &
httpd_pid=$!
app_pid=
cleanup() {
  [ -n "$app_pid" ] && kill "$app_pid" 2>/dev/null
  kill "$httpd_pid" 2>/dev/null
}
trap cleanup EXIT
for _ in $(seq 100); do
  grep -q '^closed=' "$out/httpd.out" 2>/dev/null && break
  sleep 0.1
done
grep -q '^closed=' "$out/httpd.out" || { cat "$out/httpd.err" >&2; die "the provider did not start"; }
http=http://127.0.0.1:$(sed -n 's/^http=//p' "$out/httpd.out")
closed=http://127.0.0.1:$(sed -n 's/^closed=//p' "$out/httpd.out")

# ---------------------------------------------------------------- helpers
# For the cases.  Everything they print goes to the transcript.

transcript=
log=
db=
port=
log_mark=0

# seed SQL...: rows for the database, before the application starts.
seed() { printf '%s\n' "$@" | sqlite3 "$db"; }

# start [-t N]: the application, on the case's database; the case's seed
# must come first.  The server says pid=, port= and status= on descriptor 3
# once it listens (-d3), and goes to the background.  Its environment is
# only what is named here, plus the sanitizers' options when set.
start() {
  eval "$( env -i ${LD_LIBRARY_PATH:+LD_LIBRARY_PATH="$LD_LIBRARY_PATH"} PATH="$PATH" \
             ${ASAN_OPTIONS:+ASAN_OPTIONS="$ASAN_OPTIONS"} ${UBSAN_OPTIONS:+UBSAN_OPTIONS="$UBSAN_OPTIONS"} \
             CURL_TEST_HTTP="$http" CURL_TEST_CLOSED="$closed" \
             URWEB_SQLITE_DB_PATH="$db" URWEB_MIN_HEAP=$((256 * 1024 * 1024)) \
             "$exe" "$@" -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>> "$log" 2>&1 )"
  [ "${status:-}" = OK ] || { cat "$log" >&2; die "the application did not start"; }
  app_pid=$pid
}

stop() {
  if [ -n "$app_pid" ]; then
    kill "$app_pid" 2>/dev/null
    for _ in $(seq 100); do kill -0 "$app_pid" 2>/dev/null || break; sleep 0.05; done
    app_pid=
  fi
}

# rpc PATH: an RPC as the browser sends it; the status line, any cookies,
# and the body (the script line and the value).
rpc() {
  echo "> POST $1"
  curl -s -i -m 30 -X POST -H 'Content-type: text/plain' "http://127.0.0.1:$port$1" \
    | tr -d '\r' | grep -v -E '^(Date|Server|Content-[Ll]ength|Connection|Content-type):'
}

# page PATH: a page, as text, one line per div.
page() {
  echo "> GET $1"
  curl -s -m 30 "http://127.0.0.1:$port$1" \
    | sed -e 's#</div>#\n#g' -e 's/<[^>]*>//g' | grep -v '^\s*$'
}

# wait_for TEXT: until the server has logged a line holding TEXT, 30 s at
# most; then the log since the last look.
wait_for() {
  local i
  for i in $(seq 300); do
    tail -n +"$((log_mark + 1))" "$log" | grep -q -F -- "$1" && break
    sleep 0.1
  done
  tail -n +"$((log_mark + 1))" "$log" | grep -q -F -- "$1" || echo "~ not logged within 30 s: $1"
  show_log
}

# show_log: what the server logged since the last look, less its own lines
# about connections and requests.
show_log() {
  local n
  n=$(wc -l < "$log")
  tail -n +"$((log_mark + 1))" "$log" | head -n "$((n - log_mark))" \
    | grep -v -E '^(Database connection initialized|Starting the|ONLY DURING|behind a|Forking|Listening|Accepted|Handling|Serving)|^$' \
    | sed 's/^/  | /'
  log_mark=$n
}

# charges: how many charges the provider has made so far, and for which
# keys.
charges() {
  echo "> provider charges"
  curl -s -m 5 "$http/charges" | sed 's/^/  /'
}

# rows: the orders and the jobs, from the database.
rows() {
  echo "> database"
  sqlite3 "$db" "SELECT rtrim('order ' || uw_Id || ' (' || uw_Provider || '): ' || uw_Status || ' ' || uw_Ref) FROM uw_Curl_io_orders ORDER BY uw_Id;
                 SELECT rtrim('job ' || uw_Id || ': ' || uw_Status || ' ' || uw_Answer) FROM uw_Curl_io_jobs ORDER BY uw_Id;" | sed 's/^/  /'
}

# ------------------------------------------------------------------ cases

if [ $# -gt 0 ]; then
  cases=("$@")
else
  cases=()
  for f in "$here"/cases/*.sh; do cases+=("$(basename "$f" .sh)"); done
fi

failed=0
for case in "${cases[@]}"; do
  script=$here/cases/$case.sh
  expected=$here/cases/$case.expected
  transcript=$out/$case.actual
  log=$out/$case.log
  db=$out/$case.db
  log_mark=0
  [ -f "$script" ] || { echo "$case: no such case" >&2; failed=1; continue; }
  rm -f "$db" "$db-wal" "$db-shm"
  sqlite3 "$db" < "$out/schema.sql" > /dev/null
  : > "$log"
  curl -s -m 5 -X POST "$http/reset" > /dev/null
  . "$script" > "$transcript" 2>&1
  stop
  if [ $update = 1 ]; then
    cp "$transcript" "$expected"
    echo "$case: updated"
  elif [ ! -f "$expected" ]; then
    echo "$case: nothing expected yet (run with -u to record)" >&2; failed=1
  elif diff -u "$expected" "$transcript" > "$out/$case.diff"; then
    echo "$case: ok"
  else
    echo "$case: FAILED" >&2; cat "$out/$case.diff" >&2; failed=1
  fi
done

exit $failed
