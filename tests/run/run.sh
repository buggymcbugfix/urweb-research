#!/usr/bin/env bash
#
# Runtime tests: an application is compiled, started, driven, and what it
# answered and logged is compared with a golden transcript.  For behaviour
# that shows only when the program runs, where a snapshot of the generated
# code says nothing.
#
#   tests/run/run.sh [-u] [PROJECT[/CASE]...]   everything under tests/run
#                                                unless given; -u rewrites
#                                                what the cases expect
#
# A project is a directory tests/run/PROJECT holding PROJECT.urp and its
# sources; *.c beside it is compiled against include/urweb and linked (the
# .urp says `link X.o`).  It is compiled once, for sqlite.  Its cases are
# the subdirectories holding
#
#   env         optional: VAR=value lines, the application's environment
#   drive       shell lines, run with the application up and its port in
#               $port; what they print opens the transcript
#   expected    the transcript: the drive's output, then the application's
#               log (stdout and stderr, in order), less the runtime's
#               start-up chatter
#
# Each case gets a fresh database and a fresh process, started with -d/-P
# (no fixed port, no polling).  Run from anywhere, after `make mlton`; the
# compiler is bin/urweb -boot, or the one URWEB names, run with URWEB_FLAGS
# if set.  A drive that runs longer than RUN_DRIVE_TIMEOUT seconds (60) is
# stopped and says so in the transcript.  Exit status: 0 all passed, 1 some
# failed, 2 could not run.

set -u

here=$(cd "$(dirname "$0")" && pwd)
top=$(cd "$here/../.." && pwd)
out=$here/out
urweb=${URWEB:-$top/bin/urweb}
urweb_flags=${URWEB_FLAGS:--boot -noEmacs}
cc=${CC:-cc}
drive_timeout=${RUN_DRIVE_TIMEOUT:-60}

update=0
if [ "${1:-}" = "-u" ]; then update=1; shift; fi

die() { echo "run.sh: $*" >&2; exit 2; }

for tool in curl sqlite3 timeout "$cc"; do
  command -v "$tool" >/dev/null || die "$tool is needed"
done
[ -x "$urweb" ] || die "no compiler at $urweb: build it, or set URWEB"

# The cases to run, as PROJECT/CASE.
cases=()
if [ $# -gt 0 ]; then
  for arg in "$@"; do
    if [ -f "$here/$arg/drive" ]; then
      cases+=("$arg")
    elif [ -d "$here/$arg" ]; then
      for d in "$here/$arg"/*/; do [ -f "$d/drive" ] && cases+=("$arg/$(basename "$d")"); done
    else
      die "no project or case named $arg"
    fi
  done
else
  for p in "$here"/*/; do
    for d in "$p"*/; do [ -f "$d/drive" ] && cases+=("$(basename "$p")/$(basename "$d")"); done
  done
fi

app_pid=""
built=""
bad=""

# The application is a daemon, not our child: stop it by pid, and take what
# was built out of the tree, however this ends.
stop_app() {
  [ -n "$app_pid" ] || return 0
  kill "$app_pid" 2>/dev/null
  for _ in $(seq 50); do kill -0 "$app_pid" 2>/dev/null || break; sleep 0.1; done
  kill -9 "$app_pid" 2>/dev/null
  app_pid=""
}
cleanup() {
  stop_app
  for p in $built; do rm -f "$here/$p/$p.exe" "$here/$p"/*.o; done
}
trap cleanup EXIT
trap 'exit 130' INT TERM

rm -rf "$out"
mkdir -p "$out"

# The runtime's start-up chatter, and what varies from run to run.
normalize() {
  sed -e '/^Database connection initialized\.$/d' \
      -e '/^Starting the Ur\/Web native HTTP server/,/^$/d' \
      -e '/^Forking to PID [0-9]*\.\.\.\.$/d' \
      -e '/^Listening on port [0-9]*\.\.\.\.$/d' \
      "$1"
}

failed=0
for case in "${cases[@]}"; do
  project=${case%%/*}
  pdir=$here/$project
  dir=$here/$case
  work=$out/$case
  mkdir -p "$work"

  # The project, once.
  case " $built " in
    *" $project "*) ;;
    *)
      built="$built $project"
      [ -f "$pdir/$project.urp" ] || { echo "$project: no $project.urp" >&2; bad="$bad $project"; }
      ok=1
      for c in "$pdir"/*.c; do
        [ -e "$c" ] || continue
        "$cc" -c -I"$top/include/urweb" -o "${c%.c}.o" "$c" 2> "$out/$project.cc.log" \
          || { echo "$project: $(basename "$c") does not compile" >&2; cat "$out/$project.cc.log" >&2; ok=0; }
      done
      [ $ok = 1 ] || bad="$bad $project"
      case " $bad " in *" $project "*) ;; *)
        ( cd "$pdir" && "$urweb" $urweb_flags -protocol http -dbms sqlite \
            -db "$out/$project.db" -sql "$out/$project.sql" "$project" ) > "$out/$project.build.log" 2>&1 \
          || { echo "$project: does not compile" >&2; cat "$out/$project.build.log" >&2; bad="$bad $project"; } ;;
      esac
      ;;
  esac
  case " $bad " in *" $project "*) echo "$case: skipped, $project did not build" >&2; failed=1; continue ;; esac

  # A fresh database; the sqlite driver reads its path from the environment.
  rm -f "$work/db.sqlite"* "$work/log"
  [ -f "$out/$project.sql" ] && sqlite3 "$work/db.sqlite" < "$out/$project.sql" > /dev/null
  envs=(URWEB_SQLITE_DB_PATH="$work/db.sqlite")
  if [ -f "$dir/env" ]; then
    while IFS= read -r line; do
      case $line in ''|'#'*) ;; *) envs+=("$line") ;; esac
    done < "$dir/env"
  fi

  # Up, driven, down.  The log is stdout and stderr together, in order.
  unset status pid port
  eval "$( env "${envs[@]}" "$pdir/$project.exe" -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>>"$work/log" 2>&1 )"
  if [ "${status:-}" != OK ]; then
    echo "$case: the application did not start" >&2; cat "$work/log" >&2; failed=1; continue
  fi
  app_pid=$pid
  ( cd "$dir" && port=$port timeout "$drive_timeout" bash ./drive ) > "$work/drive.out" 2>&1
  [ $? = 124 ] && echo "drive timed out after ${drive_timeout}s" >> "$work/drive.out"
  stop_app

  {
    cat "$work/drive.out"
    echo "--- log"
    normalize "$work/log"
  } > "$work/actual"

  if [ $update = 1 ]; then
    cp "$work/actual" "$dir/expected"
    echo "$case: updated"
  elif [ ! -f "$dir/expected" ]; then
    echo "$case: no expected file (run with -u to create it)" >&2; failed=1
  elif diff -u "$dir/expected" "$work/actual" > "$work/diff"; then
    echo "$case: ok"
  else
    echo "$case: FAILED" >&2; cat "$work/diff" >&2; failed=1
  fi
done

exit $failed
