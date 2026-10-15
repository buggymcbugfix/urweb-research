#!/usr/bin/env bash
#
# io code under contention: an application (app/) with io RPCs that sleep
# outside any transaction, driven by several clients at once, with what
# each request answered, when it was answered relative to the others, the
# server's log and the database compared with what each case expects.
# Timing goes into the transcript in coarse buckets ("within 1.5 s", "after
# 3 s or more"), so that a run repeats on any machine; the sleeps are long
# enough for the buckets to be wide apart.
#
#   tests/io_concurrent/run.sh [-u] [CASE...]   all cases unless given; -u
#                                               rewrites what the cases expect
#
# A case is cases/NAME.sh, a script sourced with the helpers below, writing a
# transcript that is compared with cases/NAME.expected.  Each case gets the
# application started afresh, with several worker threads, on a database of
# its own loaded from the schema and whatever the case seeds.
#
# Run from the root of the tree (make check-io-concurrent): the compiler is
# the one URWEB names, with the flags in URWEB_FLAGS, or else bin/urweb with
# -boot -noEmacs.  Needs curl and sqlite3, and on Linux cc for the shim
# writer_first.c.  Exit status: 0 every case passed, 1 some failed, 2
# could not run.

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$here/out
urweb=${URWEB:-$root/bin/urweb}
urweb_flags=${URWEB_FLAGS:--boot -noEmacs}

update=0
if [ "${1:-}" = "-u" ]; then update=1; shift; fi

die() { echo "run.sh: $*" >&2; exit 2; }

for tool in curl sqlite3; do
  command -v "$tool" >/dev/null || die "$tool is needed"
done
[ -x "$urweb" ] || die "no compiler at $urweb: build it (make mlton) or set URWEB"

rm -rf "$out"
mkdir -p "$out"

# The application, twice: as it is, and with the SQL cache.  The compiler
# runs from the root, so that the file names in its messages are as the
# snapshots have them.
build() {
  ( cd "$root" && "$urweb" $urweb_flags -dbms sqlite "$@" tests/io_concurrent/app/concurrent ) \
    > "$out/build.log" 2>&1 || { cat "$out/build.log" >&2; die "the application failed to build"; }
}
build -sql "$out/schema.sql" -output "$out/concurrent.exe"
build -sqlcache -output "$out/concurrent-cache.exe"

# For the cases that need read-write locks to let a waiting writer go first,
# as macOS's do: on Linux a shim that makes glibc's locks do so (see
# writer_first.c); elsewhere the platform's own locks.
writer_first=
if [ "$(uname -s)" = Linux ]; then
  cc -shared -fPIC -O2 -o "$out/writer_first.so" "$here/writer_first.c" -ldl > "$out/writer_first.log" 2>&1 \
    || { cat "$out/writer_first.log" >&2; die "the shim writer_first.c failed to build"; }
  writer_first=$out/writer_first.so
fi

app_pid=
cleanup() {
  [ -n "$app_pid" ] && kill "$app_pid" 2>/dev/null
  # whatever a case left in the background
  jobs -p | xargs -r kill 2>/dev/null
}
trap cleanup EXIT

# ---------------------------------------------------------------- helpers
# For the cases.  Everything they print goes to the transcript.

transcript=
log=
db=
port=
log_mark=0
declare -A client_id client_pass

# seed SQL...: rows for the database, before the application starts.
seed() { printf '%s\n' "$@" | sqlite3 "$db"; }

# launch [-cache] [-writer-first] [FLAGS...]: the application, with eight
# worker threads unless the flags say otherwise, on the case's database; the
# case's seed must come first.  With -cache, the build with the SQL cache;
# with -writer-first, under the shim above where there is one (ASan is told
# not to mind that it comes first).  The server says pid=, port= and status=
# on descriptor 3 once it listens (-d3), and goes to the background; status
# is left empty if it gives up instead.
launch() {
  local exe=$out/concurrent.exe preload= asan=${ASAN_OPTIONS:-}
  while :; do
    case "${1:-}" in
      -cache) exe=$out/concurrent-cache.exe; shift ;;
      -writer-first) preload=$writer_first; shift ;;
      *) break ;;
    esac
  done
  [ -n "$preload" ] && asan="${asan:+$asan:}verify_asan_link_order=0"
  status=
  eval "$( env -i ${LD_LIBRARY_PATH:+LD_LIBRARY_PATH="$LD_LIBRARY_PATH"} PATH="$PATH" \
             ${asan:+ASAN_OPTIONS="$asan"} ${UBSAN_OPTIONS:+UBSAN_OPTIONS="$UBSAN_OPTIONS"} \
             ${preload:+LD_PRELOAD="$preload"} \
             URWEB_SQLITE_DB_PATH="$db" \
             "$exe" -t 8 "$@" -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>> "$log" 2>&1 )"
}

# start [-cache] [-writer-first] [FLAGS...]: launch, and give up on the
# case if the application did not start.
start() {
  launch "$@"
  [ "${status:-}" = OK ] || { cat "$log" >&2; die "the application did not start"; }
  app_pid=$pid
}

# try_start [-cache] [-writer-first] [FLAGS...]: launch, for a case where
# the application may not start: whether it started or gave up, and after
# how long, in buckets.
try_start() {
  local s e
  s=$(date +%s%N)
  launch "$@"
  e=$(date +%s%N)
  if [ "${status:-}" = OK ]; then
    app_pid=$pid
    bucket "the application started" $(( (e - s) / 1000000 ))
  else
    bucket "the application gave up" $(( (e - s) / 1000000 ))
  fi
}

stop() {
  if [ -n "$app_pid" ]; then
    kill "$app_pid" 2>/dev/null
    for _ in $(seq 100); do kill -0 "$app_pid" 2>/dev/null || break; sleep 0.05; done
    app_pid=
  fi
}

# client NAME: a client, made by loading the page; its id and password
# are what the page's script carries, for the requests the browser would
# make with them.
client() {
  local body
  body=$(curl -s -m 30 "http://127.0.0.1:$port/Concurrent/main")
  client_id[$1]=$(printf '%s' "$body" | sed -n 's/.*client_id=\([0-9]*\);.*/\1/p')
  client_pass[$1]=$(printf '%s' "$body" | sed -n 's/.*client_pass=\([0-9]*\);.*/\1/p')
  [ -n "${client_id[$1]}" ] || echo "~ client $1: no id in the page"
  echo "client $1 made"
}

# The curl commands behind the requests: an RPC as the browser sends it,
# for a client when one is named; a page; the client's messages (a long
# poll: answered when a message comes).
curl_rpc() {   # NAME-or-"" PATH
  local hdr=()
  if [ -n "$1" ]; then hdr=(-H "UrWeb-Client: ${client_id[$1]}" -H "UrWeb-Pass: ${client_pass[$1]}"); fi
  curl -s -i -m 60 -X POST -H 'Content-type: text/plain' "${hdr[@]}" "http://127.0.0.1:$port$2"
}
curl_page() { curl -s -i -m 60 "http://127.0.0.1:$port$1"; }
curl_msgs() { curl -s -i -m 60 -H "UrWeb-Client: ${client_id[$1]}" -H "UrWeb-Pass: ${client_pass[$1]}" "http://127.0.0.1:$port/.msgs"; }

# What a response looks like in the transcript: the status line, then the
# body, less the headers that vary.
tidy() { tr -d '\r' | grep -v -E '^(Date|Server|Content-[Ll]ength|Connection|Content-type|Content-script-type):' | sed -e 's#</div>#\n#g' -e 's/<[^>]*>//g' | grep -v '^\s*$'; }

# bg_rpc LABEL CLIENT PATH, bg_page LABEL PATH, bg_msgs LABEL CLIENT: a
# request in the background, timed; `wait` collects them, then `took` and
# `show` tell about each.
timed() {   # LABEL CMD...
  local label=$1 s e; shift
  s=$(date +%s%N)
  "$@" > "$out/$transcript_name.$label.out" 2>&1
  e=$(date +%s%N)
  echo $(( (e - s) / 1000000 )) > "$out/$transcript_name.$label.ms"
}
bg_rpc() { timed "$1" curl_rpc "$2" "$3" & }
bg_page() { timed "$1" curl_page "$2" & }
bg_msgs() { timed "$1" curl_msgs "$2" & }

# bucket WHAT MS: WHAT, and when, in buckets.
bucket() {
  if [ "$2" -lt 1500 ]; then echo "$1 within 1.5 s"
  elif [ "$2" -ge 3000 ]; then echo "$1 after 3 s or more"
  else echo "$1 after $2 ms (between the buckets)"
  fi
}

# took LABEL: how long the request took, in buckets.
took() { bucket "$1: answered" "$(cat "$out/$transcript_name.$1.ms")"; }

# slowest LABEL...: how long the slowest of the requests took, in buckets,
# for requests whose order is a matter of timing.
slowest() {
  local l ms max=0
  for l in "$@"; do
    ms=$(cat "$out/$transcript_name.$l.ms")
    if [ "$ms" -gt "$max" ]; then max=$ms; fi
  done
  bucket "the slowest of $*: answered" "$max"
}

# together LABEL1 LABEL2: whether two requests of the same length were
# served side by side: both within 3 s.
together() {
  local a b
  a=$(cat "$out/$transcript_name.$1.ms")
  b=$(cat "$out/$transcript_name.$2.ms")
  if [ "$a" -lt 3000 ] && [ "$b" -lt 3000 ]; then
    echo "$1, $2: served together, both answered within 3 s"
  else
    echo "$1, $2: answered after $a ms and $b ms"
  fi
}

# show LABEL: the response.
show() { echo "> $1"; tidy < "$out/$transcript_name.$1.out"; }

# rpc PATH, page PATH, msgs CLIENT: the same, in the foreground.
rpc() { echo "> POST $1"; curl_rpc "" "$1" | tidy; }
page() { echo "> GET $1"; curl_page "$1" | tidy; }
msgs() { echo "> messages of $1"; curl_msgs "$1" | tidy; }

# ticks LABEL: how many ticks a messages response holds.
ticks() {
  local n
  n=$(grep -c '^tick$' "$out/$transcript_name.$1.out")
  if [ "$n" -eq 0 ]; then echo "$1: no tick"; else echo "$1: ticks"; fi
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
# about connections and requests, sorted, since the threads interleave.
show_log() {
  local n
  n=$(wc -l < "$log")
  tail -n +"$((log_mark + 1))" "$log" | head -n "$((n - log_mark))" \
    | grep -v -E '^(Database connection initialized|Starting the|ONLY DURING|behind a|Forking|Listening|Accepted|Handling|Serving|Served URI|Failed URI|Processed request for messages)|^$' \
    | sort | uniq -c | sed 's/^ *\([0-9]*\) /  | \1 x /'
  log_mark=$n
}

# reruns TEXT: how often, since the last look at the log, a request was run
# again for an error saying TEXT: never, once or more than once, since how
# often is a matter of timing.  The log is left to show_log or log_kinds.
reruns() {
  local n
  n=$(tail -n +"$((log_mark + 1))" "$log" | grep -c -F -- "unlimited retry: $1")
  case $n in
    0) echo "run again for \"$1\": never" ;;
    1) echo "run again for \"$1\": once" ;;
    *) echo "run again for \"$1\": more than once" ;;
  esac
}

# log_kinds: as show_log, but each kind of line once, without its count,
# for what is repeated as often as the timing has it.
log_kinds() {
  local n
  n=$(wc -l < "$log")
  tail -n +"$((log_mark + 1))" "$log" | head -n "$((n - log_mark))" \
    | grep -v -E '^(Database connection initialized|Starting the|ONLY DURING|behind a|Forking|Listening|Accepted|Handling|Serving|Served URI|Failed URI|Processed request for messages)|^$' \
    | sort -u | sed 's/^/  | /'
  log_mark=$n
}

# values LABEL...: the values the requests answered, sorted, for calls
# whose order is a matter of timing.
values() {
  local l
  for l in "$@"; do tidy < "$out/$transcript_name.$l.out" | tail -1; done | sort -n | tr '\n' ' ' | sed 's/ $/\n/'
}

# rows: the rows of t, counted by writer and kind, and the counter.
rows() {
  echo "> database"
  sqlite3 "$db" "SELECT uw_Who || ': ' || group_concat(uw_N, ' ') FROM (SELECT uw_Who, uw_N FROM uw_Concurrent_t ORDER BY uw_Who, uw_N) GROUP BY uw_Who ORDER BY uw_Who;
                 SELECT 'counter ' || uw_V FROM uw_Concurrent_counter;" | sed 's/^/  /'
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
  transcript_name=$case
  transcript=$out/$case.actual
  log=$out/$case.log
  db=$out/$case.db
  log_mark=0
  client_id=(); client_pass=()
  [ -f "$script" ] || { echo "$case: no such case" >&2; failed=1; continue; }
  rm -f "$db" "$db-wal" "$db-shm"
  sqlite3 "$db" < "$out/schema.sql" > /dev/null
  : > "$log"
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
