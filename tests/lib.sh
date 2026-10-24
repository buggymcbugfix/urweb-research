# What the runners under tests/ have in common: io_curl, io_concurrent,
# fastcgi_idle, fastcgi_kept, linking and shell_id, each a run.sh that
# starts by sourcing this file,
#
#   . "$(dirname "$0")/../lib.sh"
#
# The runner then has here, its directory; root, the tree's; out, where it
# writes (here/out, made afresh by fresh_out); suite, the name of its
# directory; and failed, 0 until a comparison fails. A first argument -u is
# taken off and sets update to 1: the comparisons then rewrite what is
# expected. The compiler is the one URWEB names, run with the flags in
# URWEB_FLAGS, or else bin/urweb with -boot -noEmacs. A runner exits with 0
# when it passed, 1 when it failed, and 2 when it could not run (die).

set -u

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../.." && pwd)
out=$here/out
suite=$(basename "$here")
urweb=${URWEB:-$root/bin/urweb}
urweb_flags=${URWEB_FLAGS:--boot -noEmacs}
failed=0
update=0
if [ "${1:-}" = -u ]; then update=1; shift; fi

# die MESSAGE: the runner cannot run.
die() { echo "$suite: $*" >&2; exit 2; }

# no_cases ARG...: for a runner that takes no arguments but -u.
no_cases() { [ $# -eq 0 ] || die "usage: run.sh [-u]"; }

# needs TOOL...: each on PATH.
needs() {
  local tool
  for tool in "$@"; do command -v "$tool" >/dev/null || die "$tool is needed"; done
}

# needs_urweb: the compiler.
needs_urweb() { [ -x "$urweb" ] || die "no compiler at $urweb: build it (make mlton) or set URWEB"; }

# fresh_out: out/, empty.
fresh_out() { rm -rf "$out" && mkdir -p "$out"; }

# compile LOG ARG...: the compiler on ARG..., what it says in out/LOG, its
# exit status. It runs from the root of the tree, so that the file names in
# its messages are as the snapshots have them.
compile() {
  local log=$out/$1; shift
  ( cd "$root" && "$urweb" $urweb_flags "$@" ) > "$log" 2>&1
}

# build LOG ARG...: compile, and if that fails, what it said, and die.
build() { compile "$@" || { cat "$out/$1" >&2; die "the application failed to build"; }; }

# build_preload NAME SOURCE: out/NAME.so, built with cc from the C file
# SOURCE, for LD_PRELOAD.
build_preload() {
  cc -shared -fPIC -O2 -o "$out/$1.so" "$2" -ldl > "$out/$1.log" 2>&1 \
    || { cat "$out/$1.log" >&2; die "$(basename "$2") failed to build"; }
}

# "${clean_env[@]}" [VAR=VALUE...] CMD...: CMD with nothing in its
# environment but PATH, LD_LIBRARY_PATH and the sanitizers' options where
# they are set, and the VAR=VALUE given, which win over those. A command,
# not a function, so that CMD & leaves CMD's pid in $!.
clean_env=(env -i ${LD_LIBRARY_PATH:+"LD_LIBRARY_PATH=$LD_LIBRARY_PATH"} "PATH=$PATH"
           ${ASAN_OPTIONS:+"ASAN_OPTIONS=$ASAN_OPTIONS"} ${UBSAN_OPTIONS:+"UBSAN_OPTIONS=$UBSAN_OPTIONS"})

# compare NAME EXPECTED ACTUAL: with -u, ACTUAL becomes EXPECTED; otherwise
# the two are compared, and a difference fails the runner. What it prints
# starts with the suite and NAME, which may be empty: "io_curl: drop ok".
compare() {
  local label=$suite:${1:+ $1} expected=$2 actual=$3
  if [ "$update" = 1 ]; then
    mkdir -p "$(dirname "$expected")" && cp "$actual" "$expected" && echo "$label updated"
  elif [ ! -f "$expected" ]; then
    echo "$label: nothing expected yet (run with -u to record)" >&2; failed=1
  elif diff -u "$expected" "$actual" > "$actual.diff"; then
    echo "$label ok"
  else
    cat "$actual.diff" >&2; echo "$label FAILED" >&2; failed=1
  fi
}

# For the runners whose cases drive an application server, each case on a
# database and with a log of its own (io_curl, io_concurrent). db, log and
# log_mark (the lines of the log already looked at) are the case's, and so
# is transcript, where what it prints goes; app_pid is the server's while
# it runs.
app_pid=
db=
log=
log_mark=0
transcript=

# fresh_case NAME: out/NAME.db with the schema out/schema.sql, and an empty
# out/NAME.log.
fresh_case() {
  db=$out/$1.db
  log=$out/$1.log
  log_mark=0
  rm -f "$db" "$db-wal" "$db-shm"
  sqlite3 "$db" < "$out/schema.sql" > /dev/null
  : > "$log"
}

# seed SQL...: rows for the database, before the application starts.
seed() { printf '%s\n' "$@" | sqlite3 "$db"; }

# stop: the server, if it runs, until it is gone.
stop() {
  if [ -n "$app_pid" ]; then
    kill "$app_pid" 2>/dev/null
    for _ in $(seq 100); do kill -0 "$app_pid" 2>/dev/null || break; sleep 0.05; done
    app_pid=
  fi
}

# wait_for TEXT: until the server has logged a line holding TEXT, 30 s at
# most; then what it logged since the last look, by the runner's show_log.
wait_for() {
  local i
  for i in $(seq 300); do
    tail -n +"$((log_mark + 1))" "$log" | grep -q -F -- "$1" && break
    sleep 0.1
  done
  tail -n +"$((log_mark + 1))" "$log" | grep -q -F -- "$1" || echo "~ not logged within 30 s: $1"
  show_log
}

# run_cases [CASE...]: the cases named, or else every cases/NAME.sh. Each
# is sourced after the runner's case_setup NAME, with what it prints in its
# transcript out/NAME.actual, which is then compared with
# cases/NAME.expected; the server is stopped after each.
run_cases() {
  local names=() f
  if [ $# -gt 0 ]; then
    names=("$@")
  else
    for f in "$here"/cases/*.sh; do names+=("$(basename "$f" .sh)"); done
  fi
  for case in "${names[@]}"; do
    [ -f "$here/cases/$case.sh" ] || { echo "$suite: $case: no such case" >&2; failed=1; continue; }
    transcript=$out/$case.actual
    case_setup "$case"
    . "$here/cases/$case.sh" > "$transcript" 2>&1
    stop
    compare "$case" "$here/cases/$case.expected" "$transcript"
  done
}
