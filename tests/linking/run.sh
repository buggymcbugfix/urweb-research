#!/usr/bin/env bash
#
# How the compiler links the runtime into a program, and what the program
# then needs, recorded in three transcripts and compared with `expected`:
#
#   expected/linking  The runtime's part of the link command for the flags
#                     that choose how the runtime is linked: none, -static,
#                     -dynamic, both in either order, a protocol.  A
#                     stand-in for the C compiler writes its arguments down
#                     and makes the files that -o names, so that the record
#                     depends on neither the C library, nor the libraries
#                     installed, nor the paths they are in. Then, linked
#                     for real under each protocol, a C file that names
#                     runtime functions the generated code does not need.
#   expected/shared   What a program needs at run time when it is built
#                     with -dynamic, and the same C file.  Left out where
#                     the runtime has no shared libraries (a tree configured
#                     with --disable-shared) and off Linux or without
#                     readelf.
#   expected/no-archives
#                     What the compiler says when the runtime's archives
#                     are missing, with the flags that need them and those
#                     that do not. hide.c, preloaded into the compiler,
#                     hides them. Left out off Linux and without cc.
#
#   tests/linking/run.sh [-u]      -u rewrites expected
#
# Run from the root of the tree (make check-linking). The compiler, -u and
# the exit status are as tests/lib.sh says.

. "$(dirname "$0")/../lib.sh"
no_cases "$@"
needs_urweb
fresh_out

# The stand-in for the C compiler: its arguments, one to a line and `--`
# after the last, go to $FAKECC_LOG, and the files named by -o are made.
cat > "$out/fakecc" <<'EOS'
log=${FAKECC_LOG:?}
for a in "$@"; do printf '%s\n' "$a"; done >> "$log"
echo -- >> "$log"
while [ $# -gt 0 ]; do
  case $1 in
    -o) : > "$2"; shift 2 ;;
    *) shift ;;
  esac
done
EOS

# program NAME PROJECT FLAGS...: the project, with the compiler's own C
# compiler, into out/NAME.exe; the log of the run is out/NAME.log.
program() {
  local name=$1 project=$2; shift 2
  compile "$name.log" -dbms sqlite -db "$out/x.db" "$@" -output "$out/$name.exe" "tests/linking/$project"
}

# faked NAME FLAGS...: as program, but the C compiler is the stand-in; the
# arguments it was called with are in out/NAME.cc.
faked() {
  local name=$1; shift
  FAKECC_LOG=$out/$name.cc program "$name" app/app -ccompiler "sh $out/fakecc" "$@" \
    || { cat "$out/$name.log" >&2; die "the compiler failed on the stand-in"; }
}

# The runtime's part of the link command in out/NAME.cc, the one run
# without -c: in the order of the command, -L of the compiler's lib
# directory ($libdir, shown as LIB), what the project links, -static, the
# archives and the libraries of the runtime and of ICU, each -l once, where
# it last appears. A tree configured with ICU_LIBS naming the -l flags as
# well gets ICU's twice, the first time ahead of the project's files. The
# rest depends on where the libraries are, which the test does not care about.
lines() {
  awk -v lib="$libdir" '
    $0 == "--" {
      if (!compile) {
        sep = ""
        for (i = 1; i <= n; i++)
          if (!(tok[i] in last) || last[tok[i]] == i) { printf "%s%s", sep, tok[i]; sep = " " }
        printf "\n"
      }
      n = 0; compile = 0; delete last; next
    }
    $0 == "-c" { compile = 1 }
    $0 == "-static" { tok[++n] = $0 }
    $0 ~ /^-l(urweb|icu|foo)/ { tok[++n] = $0; last[$0] = n }
    $0 ~ /\.a$/ { t = $0; sub(lib, "LIB", t); tok[++n] = t }
    $0 == "-L" lib { tok[++n] = "-LLIB" }
    $0 ~ /\/link[0-9]+_[^\/]*\.o$/ { t = $0; sub(/.*\//, "", t); tok[++n] = t }
  ' "$out/$1.cc"
}

# The compiler's lib directory, as the default link line names it.
faked default
libdir=$(sed -n 's|/liburweb\.a$||p' "$out/default.cc" | head -n 1)
[ -n "$libdir" ] || die "the link command does not name liburweb.a:$(echo; cat "$out/default.cc")"

{
  echo "### (no flag)"
  lines default
  echo "### -protocol static"
  faked protocol -protocol static; lines protocol
  echo "### -static"
  faked static -static; lines static
  echo "### -dynamic"
  faked dynamic -dynamic; lines dynamic
  echo "### -static -dynamic"
  faked sd -static -dynamic; lines sd
  echo "### -dynamic -static"
  faked ds -dynamic -static; lines ds
} > "$out/lines"

# members NAME FLAGS...: which runtime functions the members project fails
# to link for.
members() {
  local name=$1; shift
  if program "$name" members/members "$@"; then
    echo "linked"
  else
    echo "not linked, undefined: $(grep -i undefined "$out/$name.log" | grep -o 'uw_[a-z_]*' | sort -u | tr '\n' ' ' | sed 's/ *$//')"
  fi
}

{
  cat "$out/lines"
  for p in http cgi fastcgi static; do
    echo "### members, -protocol $p"
    members "members-$p" -protocol $p
  done
} > "$out/linking"

# What a program built with -dynamic needs, where there are shared libraries.
shared=0
if [ -e "$libdir/liburweb.so" ] && [ "$(uname -s)" = Linux ] && command -v readelf >/dev/null; then
  shared=1
  needed() {
    local libs
    libs=$(readelf -d "$1" | sed -n 's/.*(NEEDED).*\[\(.*\)\]/\1/p' | grep '^liburweb' | sort | tr '\n' ' ' | sed 's/ *$//')
    echo "${libs:-none}"
  }
  {
    echo "### (no flag), runtime libraries it needs"
    program built plain/plain || { cat "$out/built.log" >&2; die "the application failed to build"; }
    needed "$out/built.exe"
    echo "### -dynamic, runtime libraries it needs"
    program builtdyn plain/plain -dynamic || { cat "$out/builtdyn.log" >&2; die "the application failed to build with -dynamic"; }
    needed "$out/builtdyn.exe"
    echo "### -dynamic, started with the lib directory in LD_LIBRARY_PATH"
    if LD_LIBRARY_PATH=$libdir${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH} "$out/builtdyn.exe" -h 2>&1 | head -n 1 | grep -q '^Usage:'; then
      echo "started"
    else
      echo "did not start"
    fi
    for p in cgi static; do
      echo "### members, -dynamic, -protocol $p"
      members "members-dyn-$p" -dynamic -protocol $p
    done
  } > "$out/shared" 2>&1
fi

# What the compiler says when the runtime's archives are missing. The test
# cannot take them away from the compiler, so hide.c, preloaded into it,
# makes access() fail for the paths that end in $HIDE. The C compiler is
# the stand-in, and a case records whether the compiler called it. Linux
# only, where the compiler takes access() from the C library at run time.
noarchives=0
if [ "$(uname -s)" = Linux ] && command -v cc >/dev/null; then
  noarchives=1
  build_preload hide "$here/hide.c"
  # hidden NAME SUFFIX FLAGS...: compile app/ with the paths that end in
  # SUFFIX hidden; what the compiler printed, its exit status, and whether
  # it called the C compiler.
  hidden() {
    local name=$1 suffix=$2 status called; shift 2
    HIDE=$suffix LD_PRELOAD=$out/hide.so FAKECC_LOG=$out/$name.cc \
      program "$name" app/app -ccompiler "sh $out/fakecc" "$@"
    status=$?
    sed "s|$libdir|LIB|g" "$out/$name.log"
    if [ -s "$out/$name.cc" ]; then called=called; else called="not called"; fi
    echo "exit $status, C compiler $called"
  }
  {
    echo "### liburweb_http.a hidden"
    hidden hide-proto /liburweb_http.a
    echo "### liburweb.a hidden"
    hidden hide-runtime /liburweb.a
    echo "### every archive hidden"
    hidden hide-all .a
    echo "### every archive hidden, -static"
    hidden hide-static .a -static
    echo "### every archive hidden, -dynamic"
    hidden hide-dynamic .a -dynamic
    echo "### liburweb_http.a hidden, -protocol fastcgi"
    hidden hide-fastcgi /liburweb_http.a -protocol fastcgi
  } > "$out/no-archives"
fi

# check NAME: out/NAME against expected/NAME.
check() { compare "$1" "$here/expected/$1" "$out/$1"; }
check linking
if [ $shared = 1 ]; then check shared; else echo "$suite: shared skipped, no shared runtime in $libdir"; fi
if [ $noarchives = 1 ]; then check no-archives; else echo "$suite: no-archives skipped, not on Linux or no cc"; fi
exit $failed
