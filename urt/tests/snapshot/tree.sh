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
: > runs/stray.urt/snapshot/1/request-response/a/interaction/03.dml.sql
: > runs/stray.urt/snapshot/1/request-response/a/interaction/04.stderr
: > runs/stray.urt/snapshot/1/request-response/a/interaction/01.stdout.firefox
: > runs/stray.urt/snapshot/1/request-response/a/interaction/01.user.1.page.html
: > runs/stray.urt/snapshot/1/request-response/a/interaction/01.page.html
: > runs/stray.urt/snapshot/1/request-response/a/interaction/notes
mkdir -p runs/stray.urt/snapshot/1/request-response/a/out/more
: > runs/stray.urt/snapshot/1/request-response/a/out/more/stdout
: > runs/stray.urt/snapshot/1/request-response/a/out/log
: > runs/stray.urt/snapshot/1/request-response/a/out/stdout
: > runs/stray.urt/snapshot/1/request-response/a/out/startup.stdout
: > runs/stray.urt/snapshot/1/request-response/a/out/shutdown.stderr
mkdir -p runs/stray.urt/snapshot/1/request-response/b.c/interaction
# A project the stand-in compiler builds the stand-in server for: its run
# gets as far as answers, and the answers show what was sent.
proj runs serves
snap runs/serves.urt/snapshot/1 exitcode
arun runs/serves.urt/snapshot/1/request-response/talk
cd runs/serves.urt/snapshot/1/request-response/talk
printf 'CREATE TABLE t (id INTEGER);\n' > db.sql
cd interaction
printf "POST /add -d 'name=kettle' -d 'price=12' -b 'visitor=somebody' -H 'Accept: text/html'\n" > 02-fields.request
printf "GET /debug\n" > 03-debug.request
printf "GET /err\n" > 04-err.request
printf "GET /dml\n" > 05-dml.request
printf 'GET /a-b\n' > 06.request
printf 'GET /a\n' > 06-a.request
printf 'GET /quiet\n' > 07-quiet.request
printf 'GET /a/../b\n' > 08-dotdot.request
printf 'POST /backslash -d x\\' > 09-backslash.request
cd "$root"
arun runs/serves.urt/snapshot/1/request-response/crlf-args
printf -- '-q\r\n' > runs/serves.urt/snapshot/1/request-response/crlf-args/args
# A second run, to show that runs share nothing: its server counts from one.
arun runs/serves.urt/snapshot/1/request-response/second
# Requests the parser should not let through, each sent as it stands.
proj runs serves-odd
snap runs/serves-odd.urt/snapshot/1 exitcode
arun runs/serves-odd.urt/snapshot/1/request-response/odd
cd runs/serves-odd.urt/snapshot/1/request-response/odd/interaction
printf '"" /empty-method\n' > 02-no-method.request
printf 'GET /nul\0 -d x=1\n' > 03-nul.request
printf "GET /host -H 'Host : elsewhere'\n" > 04-host-blank.request
printf "GET /host -H ' Host: elsewhere'\n" > 05-host-indented.request
printf "GET /host -H 'X-A: 1\nHost: elsewhere'\n" > 06-host-injected.request
printf 'GET /fragment#part\n' > 07-fragment.request
printf 'GET /nameless\n' > .request
cd "$root"
# A project whose server is made to fail at the start, by APP_MODE.
proj runs serves-start
snap runs/serves-start.urt/snapshot/1 exitcode
arun runs/serves-start.urt/snapshot/1/request-response/start
# A project whose server takes its time over the one request, for a
# transcript to interrupt.
proj runs serves-wait
snap runs/serves-wait.urt/snapshot/1 exitcode
arun runs/serves-wait.urt/snapshot/1/request-response/wait
printf 'GET /wait\n' > runs/serves-wait.urt/snapshot/1/request-response/wait/interaction/01.request
# A project whose server exits after the second request, and one whose
# server overwrites its database.
proj runs serves-dies
snap runs/serves-dies.urt/snapshot/1 exitcode
arun runs/serves-dies.urt/snapshot/1/request-response/dies
printf 'GET /die\n' > runs/serves-dies.urt/snapshot/1/request-response/dies/interaction/02-die.request
printf 'GET /after\n' > runs/serves-dies.urt/snapshot/1/request-response/dies/interaction/03-after.request
proj runs serves-corrupts
snap runs/serves-corrupts.urt/snapshot/1 exitcode
arun runs/serves-corrupts.urt/snapshot/1/request-response/corrupts
printf 'CREATE TABLE t (id INTEGER);\nINSERT INTO t VALUES (1);\n' > runs/serves-corrupts.urt/snapshot/1/request-response/corrupts/db.sql
printf 'GET /corrupt\n' > runs/serves-corrupts.urt/snapshot/1/request-response/corrupts/interaction/02-corrupt.request
# A request file with CRLF line ends and a continued line.
proj runs serves-cr
snap runs/serves-cr.urt/snapshot/1 exitcode
arun runs/serves-cr.urt/snapshot/1/request-response/continued
printf 'POST /fields -d "x=1" \\\r\n  -d "y=2"\r\n' > runs/serves-cr.urt/snapshot/1/request-response/continued/interaction/02-continued.request
# A project with a run that has an environment of its own, which the
# stand-in server reports back, and a run after it that has none; and one
# whose env says what it may not.
proj runs serves-env
snap runs/serves-env.urt/snapshot/1 exitcode
arun runs/serves-env.urt/snapshot/1/request-response/env
cd runs/serves-env.urt/snapshot/1/request-response/env
printf '# the greeting, then nothing\n\nAPP_GREETING=hello, run\r\nAPP_EMPTY=\n' > env
printf 'GET /env/APP_GREETING\n' > interaction/01.request
printf 'GET /env/APP_EMPTY\n' > interaction/02.request
printf 'GET /env/APP_OUTSIDE\n' > interaction/03.request
cd "$root"
arun runs/serves-env.urt/snapshot/1/request-response/plain
printf 'GET /env/APP_GREETING\n' > runs/serves-env.urt/snapshot/1/request-response/plain/interaction/01.request
printf 'GET /env/APP_EMPTY\n' > runs/serves-env.urt/snapshot/1/request-response/plain/interaction/02.request
proj runs serves-env-bad
snap runs/serves-env-bad.urt/snapshot/1 exitcode
arun runs/serves-env-bad.urt/snapshot/1/request-response/bad
printf 'TZ=Europe/Berlin\nno assignment\n1APP=x\nAPP_TWICE=1\nAPP_TWICE=2\n' \
    > runs/serves-env-bad.urt/snapshot/1/request-response/bad/env
# A project whose server does something a while after a request, which a
# run waits for; one with waits that cannot be read; and one whose server
# is gone before what is waited for comes.
proj runs serves-later
snap runs/serves-later.urt/snapshot/1 exitcode
arun runs/serves-later.urt/snapshot/1/request-response/task
cd runs/serves-later.urt/snapshot/1/request-response/task/interaction
printf 'GET /later\n' > 01.request
printf 'until a line from a task\r\n' > 02-task.wait
printf 'GET /after\n' > 03-after.request
cd "$root"
arun runs/serves-later.urt/snapshot/1/request-response/first
rm runs/serves-later.urt/snapshot/1/request-response/first/interaction/01.request
printf 'APP_LATER=1\n' > runs/serves-later.urt/snapshot/1/request-response/first/env
printf 'until a line from a task\n' > runs/serves-later.urt/snapshot/1/request-response/first/interaction/01-task.wait
# A wait for a time: the task the request leaves comes due within it.
arun runs/serves-later.urt/snapshot/1/request-response/timed
cd runs/serves-later.urt/snapshot/1/request-response/timed/interaction
printf 'GET /later\n' > 01.request
printf 'wait 2s\n' > 02-two.wait
printf 'GET /after\n' > 03-after.request
cd "$root"
proj runs serves-later-bad
snap runs/serves-later-bad.urt/snapshot/1 exitcode
arun runs/serves-later-bad.urt/snapshot/1/request-response/bad
cd runs/serves-later-bad.urt/snapshot/1/request-response/bad/interaction
printf 'GET /later\n' > 02-both.request
printf 'until a line from a task\n' > 02-both.wait
printf 'for a second\n' > 03-no-until.wait
printf 'until \n' > 04-nothing.wait
printf 'until one\nuntil two\n' > 05-two.wait
printf 'until  \n' > 06-blank.wait
printf 'wait 5\n' > 07-unitless.wait
printf 'wait\n' > 08-no-time.wait
cd "$root"
# Waits for what does not come: after the server's last task, and before
# its next one, which is due later than an until looks.
proj runs serves-never
snap runs/serves-never.urt/snapshot/1 exitcode
arun runs/serves-never.urt/snapshot/1/request-response/after
cd runs/serves-never.urt/snapshot/1/request-response/after/interaction
printf 'GET /later\n' > 01.request
printf 'until a line from a task\n' > 02-task.wait
printf 'until another line\n' > 03-more.wait
printf 'GET /after\n' > 04-after.request
cd "$root"
arun runs/serves-never.urt/snapshot/1/request-response/far
printf 'GET /year\n' > runs/serves-never.urt/snapshot/1/request-response/far/interaction/01.request
printf 'until a line from a task\n' > runs/serves-never.urt/snapshot/1/request-response/far/interaction/02-task.wait
proj runs serves-gone
snap runs/serves-gone.urt/snapshot/1 exitcode
arun runs/serves-gone.urt/snapshot/1/request-response/gone
printf 'GET /die\n' > runs/serves-gone.urt/snapshot/1/request-response/gone/interaction/02-die.request
printf 'until  never\n' > runs/serves-gone.urt/snapshot/1/request-response/gone/interaction/03-never.wait
# Waits for one of two lines printed together; for text after a NUL; and
# for text that the server's banner holds as well, before a task that
# prints it too.
proj runs serves-together
snap runs/serves-together.urt/snapshot/1 exitcode
arun runs/serves-together.urt/snapshot/1/request-response/a
cd runs/serves-together.urt/snapshot/1/request-response/a/interaction
printf 'GET /print/task%%3A%%20tick%%201%%0Atask%%3A%%20tick%%202%%0A\n' > 01.request
printf 'until tick 1\n' > 02-one.wait
printf 'GET /after\n' > 03-after.request
cd "$root"
proj runs serves-nul
snap runs/serves-nul.urt/snapshot/1 exitcode
arun runs/serves-nul.urt/snapshot/1/request-response/a
cd runs/serves-nul.urt/snapshot/1/request-response/a/interaction
printf 'GET /print/task%%3A%%20a%%00b%%0Atask%%3A%%20b%%20again%%0A\n' > 01.request
printf 'until b\n' > 02-b.wait
cd "$root"
proj runs serves-banner
snap runs/serves-banner.urt/snapshot/1 exitcode
arun runs/serves-banner.urt/snapshot/1/request-response/a
cd runs/serves-banner.urt/snapshot/1/request-response/a
rm interaction/01.request
printf 'APP_LATER=1\n' > env
printf 'until in\n' > interaction/01-in.wait
printf 'GET /die\n' > interaction/02-die.request
cd "$root"
# A request whose name ends in .wait: refused, as is any name with a dot.
proj runs serves-dotwait
snap runs/serves-dotwait.urt/snapshot/1 exitcode
arun runs/serves-dotwait.urt/snapshot/1/request-response/a
printf 'GET /after\n' > runs/serves-dotwait.urt/snapshot/1/request-response/a/interaction/02.wait.request
# Env files with a line of blanks, and with a CR that is no line end; and
# a run whose env is made too big to start a program with, by its cmd.
proj runs serves-env-blank
snap runs/serves-env-blank.urt/snapshot/1 exitcode
arun runs/serves-env-blank.urt/snapshot/1/request-response/a
printf 'APP_A=1\n \nAPP_B=2\n' > runs/serves-env-blank.urt/snapshot/1/request-response/a/env
printf 'GET /env/APP_B\n' > runs/serves-env-blank.urt/snapshot/1/request-response/a/interaction/01.request
proj runs serves-env-cr
snap runs/serves-env-cr.urt/snapshot/1 exitcode
arun runs/serves-env-cr.urt/snapshot/1/request-response/a
printf 'APP_A=1\rAPP_B=2\n' > runs/serves-env-cr.urt/snapshot/1/request-response/a/env
printf 'GET /env/APP_A\n' > runs/serves-env-cr.urt/snapshot/1/request-response/a/interaction/01.request
printf 'GET /env/APP_B\n' > runs/serves-env-cr.urt/snapshot/1/request-response/a/interaction/02.request
proj runs serves-env-big
snap runs/serves-env-big.urt/snapshot/1 exitcode
arun runs/serves-env-big.urt/snapshot/1/request-response/a
proj runs unready
snap runs/unready.urt/snapshot/1 exitcode
arun runs/unready.urt/snapshot/1/request-response/a
rm runs/unready.urt/snapshot/1/request-response/a/secret
cd runs/unready.urt/snapshot/1/request-response/a/interaction
printf 'get /\n' > 01.request
printf 'GET\n' > 02.request
printf 'GET main\n' > 03.request
printf 'GET /main -X POST\n' > 04.request
printf 'POST /main -d\n' > 05.request
printf 'POST /main -d @body\n' > 06.request
printf 'POST /main -b cookies.txt\n' > 07.request
printf "POST /main -d 'never closed\n" > 08.request
printf 'GET /main -H @headers\n' > 09.request
printf 'GET /main -H Accept\n' > 10.request
printf 'GET /main -H "host: elsewhere"\n' > 11.request
printf 'GET /main -H "User-Agent: me"\n' > 12.request
printf 'GET /main -H "Accept: text/plain" -H "If-Modified-Since: Thu, 01 Jan 1970 00:00:00 GMT"\n' > 13.request
printf 'GET /main -H "Host:"\n' > 14.request
printf "GET /main -H 'User-Agent;'\n" > 15.request
: > 16.request
printf 'GET /main # not a comment\n' > 17.request
printf 'GET /main -d a=1 extra\n' > 18.request
printf 'GET /main -d "a=1' > 19.request
cd "$root"
arun runs/unready.urt/snapshot/1/request-response/threads
printf -- '-t 4\n' > runs/unready.urt/snapshot/1/request-response/threads/args
arun runs/unready.urt/snapshot/1/request-response/threads-too
printf -- '-q -t4\n' > runs/unready.urt/snapshot/1/request-response/threads-too/args
arun runs/unready.urt/snapshot/1/request-response/threads-joined
printf -- '-qt4\n' > runs/unready.urt/snapshot/1/request-response/threads-joined/args
arun runs/unready.urt/snapshot/1/request-response/dashes
printf -- '-q --\n' > runs/unready.urt/snapshot/1/request-response/dashes/args
# Browser runs.  Their steps are read by urt-browse before there has to be
# an application, and the rest is as for the runs above.
brun() {
    mkdir -p "$1/interaction"
    : > "$1/args"
    printf 'secret signature\n' > "$1/secret"
    printf 'user: open / [target=1]\n' > "$1/interaction/01.step"
}
proj runs web
snap runs/web.urt/snapshot/1 exitcode
brun runs/web.urt/snapshot/1/browser/first
printf "user: fill 'input[name=name]' 'tea kettle'\nuser: click 'text=Add'\nBernd:\n\topen / [target=1]\n\tuntil 'text=1 item'\n" \
    > runs/web.urt/snapshot/1/browser/first/interaction/02-add.step
printf 'order user Bernd\n' > runs/web.urt/snapshot/1/browser/first/context
printf 'In a browser.\n' > runs/web.urt/snapshot/1/browser/first/README.md
mkdir -p runs/web.urt/snapshot/1/browser/first/screenshots/old runs/web.urt/snapshot/1/browser/first/recordings
: > runs/web.urt/snapshot/1/browser/first/recordings/user.1.chromium.webm
: > runs/web.urt/snapshot/1/browser/first/screenshots/01.user.1.chromium.png
: > runs/web.urt/snapshot/1/browser/first/screenshots/old/anything
proj runs webstray
snap runs/webstray.urt/snapshot/1 exitcode
brun runs/webstray.urt/snapshot/1/browser/a
: > runs/webstray.urt/snapshot/1/browser/notes.txt
: > runs/webstray.urt/snapshot/1/browser/a/shot.png
: > runs/webstray.urt/snapshot/1/browser/a/a.b.context
: > runs/webstray.urt/snapshot/1/browser/a/screenshots
: > runs/webstray.urt/snapshot/1/browser/a/recordings
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.response
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.png
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.png
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.page
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.firefox.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.page
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.page.firefox.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.firefox.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.page.opera.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.firefox.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.firefox.1.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.firefox.1.page.webkit.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.sql
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.dml.sql
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.dml.firefox.sql
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.firefox.dml.sql
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.user.1.error
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.stdout.firefox
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.firefox.stdout
: > runs/webstray.urt/snapshot/1/browser/a/interaction/01.db.sql
: > runs/webstray.urt/snapshot/1/browser/a/interaction/02.user.1.page.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/02.user.1.page.webkit.html
: > runs/webstray.urt/snapshot/1/browser/a/interaction/a.b.step
mkdir -p runs/webstray.urt/snapshot/1/browser/a/out
: > runs/webstray.urt/snapshot/1/browser/a/out/log
: > runs/webstray.urt/snapshot/1/browser/a/out/startup.stdout
: > runs/webstray.urt/snapshot/1/browser/a/out/startup.stdout.firefox
: > runs/webstray.urt/snapshot/1/browser/a/out/startup.firefox.stdout
: > runs/webstray.urt/snapshot/1/browser/a/out/db_final.sql
: > runs/webstray.urt/snapshot/1/browser/a/out/shutdown.db.sql
: > runs/webstray.urt/snapshot/1/browser/a/out/shutdown.db.firefox.sql
: > runs/webstray.urt/snapshot/1/browser/a/out/shutdown.db.opera.sql
mkdir -p runs/webstray.urt/snapshot/1/browser/b.c/interaction
proj runs webunready
snap runs/webunready.urt/snapshot/1 exitcode
brun runs/webunready.urt/snapshot/1/browser/a
rm runs/webunready.urt/snapshot/1/browser/a/secret
brun runs/webunready.urt/snapshot/1/browser/steps
cd runs/webunready.urt/snapshot/1/browser/steps/interaction
cat > 01.step <<'STEP'
user.1: click 'text=Add'
click 'text=Add'
user: click 'text=Add'
Bernd: open / [target=1]
Bernd: tab 1
	fill 'a' 'b'
STEP
cat > 02.step <<'STEP'
user: open main [target=1]
user: frob x
user: fill 'a'
user: press
user: dialog maybe
user: wait 1
user: until 'a' gone [5]
user: back now
user: click 'x' [at=here]
user: click 'x' [target=a.b]
user: click "x"
user: label "mine"
user: after user."mine"
user: after Bernd."nope"
user: wait 1s [async]
user: click 'x' (at=1,1)
user: fill 'a' "b"
STEP
printf '# nothing yet\n' > 03.step
printf "user: open /main 'never closed\n" > 04.step
printf "user:\n  click 'x'\n\tclose\n\tclose\nwait 1s\nuser: wait 1s\n" > 05.step
cd "$root"
# The run's context is read with the steps.
cat > runs/webunready.urt/snapshot/1/browser/steps/context <<'CONTEXT'
viewport big
device_scale_factor 0
frob 1
locale
color_scheme bright
has_touch yes
has_touch no
order user
tasks sometimes
rtt fast
[Bernd]
order Bernd
click 2
[Carlos]
CONTEXT
brun runs/webunready.urt/snapshot/1/browser/threads
printf -- '-t 2\n' > runs/webunready.urt/snapshot/1/browser/threads/args
brun runs/webunready.urt/snapshot/1/browser/engines-none
: > runs/webunready.urt/snapshot/1/browser/engines-none/engines
brun runs/webunready.urt/snapshot/1/browser/engines-odd
printf 'chromium\nopera\n' > runs/webunready.urt/snapshot/1/browser/engines-odd/engines
printf 'firefox\n' > runs/web.urt/snapshot/1/browser/first/engines
# A case that cannot be named stops the run, so it comes last of all.
proj zz badname
snap zz/badname.urt/snapshot/c.d exitcode
