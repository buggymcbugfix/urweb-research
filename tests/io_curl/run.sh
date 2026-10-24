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
# Run from the root of the tree (make check-io-curl). The compiler, -u and
# the exit status are as tests/lib.sh says. Needs python3 (the provider),
# curl and sqlite3.

. "$(dirname "$0")/../lib.sh"
needs python3 curl sqlite3
needs_urweb
fresh_out

# The application, and the schema its database starts from.
rm -f "$here/app/curl_io.exe"
build build.log -dbms sqlite -sql "$out/schema.sql" tests/io_curl/app/curl_io
exe=$here/app/curl_io.exe

# The provider, until this script ends.
python3 "$here/httpd.py" > "$out/httpd.out" 2> "$out/httpd.err" &
httpd_pid=$!
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

port=
sig=

# start [-t N]: the application, on the case's database; the case's seed
# must come first.  The server says pid=, port= and status= on descriptor 3
# once it listens (-d3), and goes to the background.  Its environment is
# only what is named here, plus the sanitizers' options when set.
start() {
  eval "$( "${clean_env[@]}" CURL_TEST_HTTP="$http" CURL_TEST_CLOSED="$closed" URWEB_SQLITE_DB_PATH="$db" \
             "$exe" "$@" -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>> "$log" 2>&1 )"
  [ "${status:-}" = OK ] || { cat "$log" >&2; die "the application did not start"; }
  app_pid=$pid
  # The cookie signature the page carries, sent with every RPC as the
  # browser sends it: the payment functions read the provider's address
  # from the environment, which the compiler counts as a read of request
  # data (under cgi the environment carries the request), so their handlers
  # ask for it.  The key is drawn afresh at every start.
  sig=$(curl -s -m 5 "http://127.0.0.1:$port/Curl_io/main" | sed -n 's/.*sig="\([0-9A-F]*\)".*/\1/p')
  [ -n "$sig" ] || { cat "$log" >&2; die "the page carries no cookie signature"; }
}

# rpc PATH: an RPC as the browser sends it, signature included; the status
# line, any cookies, and the body (the script line and the value).
rpc() {
  echo "> POST $1"
  curl -s -i -m 30 -X POST -H 'Content-type: text/plain' -H "UrWeb-Sig: $sig" "http://127.0.0.1:$port$1" \
    | tr -d '\r' | grep -v -E '^(Date|Server|Content-[Ll]ength|Connection|Content-type):'
}

# page PATH: a page, as text, one line per div.
page() {
  echo "> GET $1"
  curl -s -m 30 "http://127.0.0.1:$port$1" \
    | sed -e 's#</div>#\n#g' -e 's/<[^>]*>//g' | grep -v '^\s*$'
}

# show_log: what the server logged since the last look, less its own lines
# about connections and requests.
show_log() {
  local n
  n=$(wc -l < "$log")
  tail -n +"$((log_mark + 1))" "$log" | head -n "$((n - log_mark))" \
    | grep -v -E '^(Database connection initialized|Starting the|ONLY DURING|behind a|Forking|Listening|Accepted|Handling|Serving|Served URI|Failed URI)|^$' \
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

# case_setup NAME: what a case starts from (see run_cases in tests/lib.sh),
# and the provider's charges forgotten.
case_setup() {
  fresh_case "$1"
  curl -s -m 5 -X POST "$http/reset" > /dev/null
}

run_cases "$@"
exit $failed
