#!/bin/sh
#
# Build the fixture tree for urt/check in the directory named: projects,
# their snapshots under PROJECT.urt/snapshot/CASE, and one of every mistake
# the runner is meant to notice.  The artifacts under expected/ start
# empty; a plain run fills them from the stand-in compiler beside this
# script.
#
# A script rather than files in the repository, so that nothing here
# looks like a snapshot to a run over the whole tree.

set -eu
root=$1
mkdir -p "$root"
cd "$root"

# proj DIR NAME: a project, .ur and .urp, under DIR
proj() {
    mkdir -p "$1"
    printf 'fun main () = return <xml>%s</xml>\n' "$2" > "$1/$2.ur"
    printf '%s\n' "$2" > "$1/$2.urp"
}

# snap DIR ARTIFACT...: a case directory with empty args and expected/
snap() {
    d=$1; shift
    mkdir -p "$d/expected"
    : > "$d/args"
    for a in "$@"; do : > "$d/expected/$a"; done
}

# The good ones.
proj . hello
snap hello.urt/snapshot/1 exitcode stdout.txt stderr.txt settings.txt
proj . shop
mkdir -p shop.urt/snapshot
printf 'One case per database.\n' > shop.urt/snapshot/README.md
for db in mysql postgres sqlite; do
    snap "shop.urt/snapshot/$db" exitcode schema.sql
    printf -- '-dbms %s\n' "$db" > "shop.urt/snapshot/$db/args"
done
proj . lonely
proj sub deep
snap sub/deep.urt/snapshot/1 exitcode stdout.txt parsetree.ur typecheck.urs client.js endpoints.json server.c
printf 'The whole pipeline.\n' > sub/deep.urt/README.md
mkdir -p sub/deep.urt/watcher
: > sub/deep.urt/watcher/state
proj sub/demo first
proj sub/demo second
snap sub/demo/merging.urt/snapshot/1 exitcode stdout.txt
printf '.\n' > sub/demo/merging.urt/project
printf -- '-demo /Merging\n' > sub/demo/merging.urt/snapshot/1/args
mkdir -p empty

# The mistakes.
mkdir -p sub/orphan.urt/snapshot/1/expected
: > sub/orphan.urt/snapshot/1/expected/exitcode
proj broken flat
mkdir -p broken/flat.urt/snapshot/expected
: > broken/flat.urt/snapshot/expected/exitcode
: > broken/flat.urt/snapshot/args
proj broken empty
mkdir -p broken/empty.urt/snapshot
printf 'nothing here\n' > broken/empty.urt/snapshot/README.md
proj broken stray
snap broken/stray.urt/snapshot/1 exitcode
printf 'todo\n' > broken/stray.urt/snapshot/notes.txt
: > broken/stray.urt/snapshot/1/expected/bogus.txt
mkdir -p broken/stray.urt/snapshot/1/expected/sub
: > broken/stray.urt/stray.txt
proj broken nocase
snap broken/nocase.urt/snapshot/a exitcode
mkdir -p broken/nocase.urt/snapshot/b
: > broken/nocase.urt/snapshot/b/args
proj broken twins
for c in one two three four; do snap "broken/twins.urt/snapshot/$c" exitcode; done
printf -- '-dbms sqlite' > broken/twins.urt/snapshot/one/args
printf -- '  -dbms\t sqlite\n\n' > broken/twins.urt/snapshot/two/args
rm broken/twins.urt/snapshot/four/args
proj broken nothing
snap broken/nothing.urt/snapshot/1
proj broken noexit
snap broken/noexit.urt/snapshot/1 exitcode parsetree.ur
proj broken fails
snap broken/fails.urt/snapshot/1 exitcode stderr.txt schema.sql
printf -- '-dbms bogus\n' > broken/fails.urt/snapshot/1/args
proj broken stopper
snap broken/stopper.urt/snapshot/1 exitcode stdout.txt
printf -- '-stop parse\n' > broken/stopper.urt/snapshot/1/args
# Runs of the application.  The stand-in compiler makes none, so a run
# that is in order gets as far as failing for want of one, and the one from
# last time goes; for a project called builds it makes one that is no
# server, which gets as far as not starting.  The rest is what is noticed
# before that.
arun() {
    mkdir -p "$1/interaction"
    : > "$1/args"
    printf 'secret signature\n' > "$1/secret"
    printf 'GET /\n' > "$1/interaction/01.request"
}
proj runs good
snap runs/good.urt/snapshot/1 exitcode
arun runs/good.urt/snapshot/1/request-response/first
printf 'POST /Shop/add\n    -d '"'"'name=tea kettle'"'"' -d "price=\\"12\\""\\\n    --cookie visitor=somebody\n' \
    > runs/good.urt/snapshot/1/request-response/first/interaction/02-add.request
printf 'left from last time\n' > runs/good.urt/snapshot/1/app.exe
printf 'Two runs.\n' > runs/good.urt/snapshot/1/request-response/README.md
printf 'The first.\n' > runs/good.urt/snapshot/1/request-response/first/README.md
proj runs builds
snap runs/builds.urt/snapshot/1 exitcode
arun runs/builds.urt/snapshot/1/request-response/1
proj runs stopped
snap runs/stopped.urt/snapshot/1 exitcode
printf -- '-dbms sqlite -tc\n' > runs/stopped.urt/snapshot/1/args
arun runs/stopped.urt/snapshot/1/request-response/1
proj runs stray
snap runs/stray.urt/snapshot/1 exitcode
arun runs/stray.urt/snapshot/1/request-response/a
: > runs/stray.urt/snapshot/1/request-response/notes.txt
: > runs/stray.urt/snapshot/1/request-response/a/cookies.txt
: > runs/stray.urt/snapshot/1/request-response/a/interaction/02.response
: > runs/stray.urt/snapshot/1/request-response/a/interaction/03.sql
: > runs/stray.urt/snapshot/1/request-response/a/interaction/notes
mkdir -p runs/stray.urt/snapshot/1/request-response/a/out/more
: > runs/stray.urt/snapshot/1/request-response/a/out/more/stdout
: > runs/stray.urt/snapshot/1/request-response/a/out/log
: > runs/stray.urt/snapshot/1/request-response/a/out/stdout
mkdir -p runs/stray.urt/snapshot/1/request-response/b.c/interaction
proj runs unready
snap runs/unready.urt/snapshot/1 exitcode
arun runs/unready.urt/snapshot/1/request-response/a
rm runs/unready.urt/snapshot/1/request-response/a/secret
cd runs/unready.urt/snapshot/1/request-response/a/interaction
printf 'get /\n' > 01.request
printf 'GET\n' > 02.request
printf 'GET main\n' > 03.request
printf 'GET /main -H "Accept: text/plain"\n' > 04.request
printf 'POST /main -d\n' > 05.request
printf 'POST /main -d @body\n' > 06.request
printf 'POST /main -b cookies.txt\n' > 07.request
printf "POST /main -d 'never closed\n" > 08.request
cd "$root"
arun runs/unready.urt/snapshot/1/request-response/threads
printf -- '-t 4\n' > runs/unready.urt/snapshot/1/request-response/threads/args
arun runs/unready.urt/snapshot/1/request-response/threads-too
printf -- '-q -t4\n' > runs/unready.urt/snapshot/1/request-response/threads-too/args
# A case that cannot be named stops the run, so it comes last of all.
proj zz badname
snap zz/badname.urt/snapshot/c.d exitcode
