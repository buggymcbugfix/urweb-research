/* snapshot.c -- urt snapshot: snapshot tests for the Ur/Web compiler.
 * Run it on a project and keep its output and some intermediate state as
 * files, so that a change to the compiler shows up as a diff.  `urt
 * snapshot --help` prints the layout and the usage; usage_text below is
 * that.
 *
 * Held to the transcripts under urt/tests/snapshot by urt/check, as far
 * as a stand-in compiler goes; the runs of an application, which take a
 * real one, by the cases under tests/reproducible with the rest of the
 * compiler's own snapshots (make check-snapshot), and the runs in a
 * browser by those of demo/batch, demo/chat, demo/roundTrip, tests/alert
 * and tests/vendor.
 */

/* POSIX 2008 with the XSI extensions, and on macOS the BSD ones too,
 * which its headers hide once a strict level is asked for (mkdtemp is
 * one).  _DARWIN_C_SOURCE means nothing anywhere else. */
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "commands.h"
#include "fs.h"
#include "report.h"
#include "runs.h"
#include "tui.h"

static const char usage_text[] =
    "SNAPSHOT TESTS\n"
    "==============\n"
    "\n"
    "Run the compiler on a project and keep its output as well as some intermediate state in files,\n"
    "so that a change to the compiler shows up as a diff.\n"
    "Think of them as a _contrast agent_ that alerts us to changes.\n"
    "\n"
    "\n"
    "DESCRIPTION\n"
    "-----------\n"
    "\n"
    "A project's snapshots live beside it, under its urt directory: next to\n"
    "`hello.urp` is `hello.urt`, and the snapshots are `hello.urt/snapshot`.  Each\n"
    "case there is a run of the compiler, with the flags it was given and what it\n"
    "wrote:\n"
    "\n"
    "  hello.ur\n"
    "  hello.urp\n"
    "  hello.urt/\n"
    "  `-- snapshot/\n"
    "      `-- 1/                      a case; named, by a number unless told otherwise\n"
    "          |-- args                extra flags, if any; the project is implied\n"
    "          `-- expected/\n"
    "              |-- exitcode        the compiler's exit status\n"
    "              |-- stdout.txt      what the compiler wrote to stdout\n"
    "              |-- stderr.txt      ... and to stderr\n"
    "              |-- settings.txt    -saveSettings   the parsed and merged job\n"
    "              |-- parsetree.ur    -saveParsetree  the program as parsed\n"
    "              |-- typecheck.urs   -saveTypecheck  the signatures inferred\n"
    "              |-- schema.sql      -sql            the database schema\n"
    "              |-- client.js       -saveJs         the client-side script\n"
    "              |-- endpoints.json  -endpoints      the URLs the app serves\n"
    "              `-- server.c        -saveC          the generated C\n"
    "\n"
    "One project can have several cases (the same program under three database\n"
    "backends, say), each with its own args and the artifacts it tracks:\n"
    "\n"
    "  shop.urt/\n"
    "  `-- snapshot/\n"
    "      |-- mysql/\n"
    "      |   |-- args                -dbms mysql\n"
    "      |   `-- expected/\n"
    "      |       `-- schema.sql\n"
    "      |-- postgres/\n"
    "      |   `-- ...\n"
    "      `-- sqlite/\n"
    "          `-- ...\n"
    "\n"
    "A case can go on from the compiler to the application it compiles: under\n"
    "request-response/ are runs, each the application started afresh and sent\n"
    "requests, one after the other in the order of their names:\n"
    "\n"
    "  shop.urt/\n"
    "  `-- snapshot/\n"
    "      `-- sqlite/\n"
    "          |-- args                        -dbms sqlite\n"
    "          |-- expected/\n"
    "          |-- app.exe                     the application as last built; not for git\n"
    "          `-- request-response/\n"
    "              `-- checkout/               a run; named like a case\n"
    "                  |-- args                extra flags for the application, if any\n"
    "                  |-- env                 its environment, NAME=value a line, if any\n"
    "                  |-- secret              the key it signs with: 16 bytes or more\n"
    "                  |-- db.sql              the database it starts from, if it uses one\n"
    "                  |-- interaction/\n"
    "                  |   |-- 01-add.request  what to send\n"
    "                  |   |-- 01-add.response what came back, as `curl -i` prints it\n"
    "                  |   |-- 01-add.dml.sql  what that changed in the database, if anything\n"
    "                  |   |-- 01-add.stdout   what the server printed meanwhile, if anything\n"
    "                  |   |-- 01-add.stderr\n"
    "                  |   |-- 02-pay.request\n"
    "                  |   |-- 02-pay.response\n"
    "                  |   |-- 03-mail.wait    time to let pass, or a line to wait for\n"
    "                  |   |-- 03-mail.stdout  what the server printed meanwhile, as for a request\n"
    "                  |   |-- 04-ship.request\n"
    "                  |   `-- 04-ship.error   why the request could not be sent, while it cannot\n"
    "                  `-- out/\n"
    "                      |-- startup.dml.sql what the application did before the first\n"
    "                      |-- startup.stdout  request (task initialize), if anything\n"
    "                      |-- startup.stderr\n"
    "                      |-- shutdown.stdout what it printed after the last, if anything\n"
    "                      |-- shutdown.stderr\n"
    "                      `-- shutdown.db.sql the database it left behind\n"
    "\n"
    "A request is a method, a path, and then fields (-d), cookies (-b) and headers\n"
    "(-H) in curl's spelling, quoted as for a shell; nothing else is taken:\n"
    "\n"
    "  POST /Shop/add\n"
    "      -d 'name=kettle'\n"
    "      -d 'price=12'\n"
    "      -b 'visitor=somebody'\n"
    "      -H 'Accept: text/html'\n"
    "\n"
    "It is sent as it stands, so it says all of it: a path has what it cannot carry\n"
    "(a blank, a #, a control character) percent-encoded, a header or cookie holds\n"
    "no line break, and nothing is read from a file (-d @file, -b file, -H @file).\n"
    "\n"
    "What the server prints for a request goes beside its response, and only if\n"
    "there is any: its own lines about connections and requests are dropped, the\n"
    "program's debug output and the runtime's errors are kept.  A request that\n"
    "cannot be sent at all (the server died, or hangs), or a wait that cannot be\n"
    "done (below), stops the run: NAME.error says why, the records of the steps\n"
    "after it go, and the run fails until it gets past the step again.\n"
    "\n"
    "Runs share nothing.  Each has its own database -- SQLite, so the case's args\n"
    "need -dbms sqlite -- loaded from db.sql, which holds the schema and any rows\n"
    "to start with; within a run, the requests build on each other.  So that the\n"
    "same requests get the same answers, the application's random numbers and its\n"
    "time zone, UTC, are pinned, and so is its clock: the server's time is what urt\n"
    "says it is, and nothing happens on the wall clock.  The first request is\n"
    "served at 2000-01-01 00:00:00 UTC, each further one a second after the one\n"
    "before, and the database agrees; what the application does by itself -- a\n"
    "task periodic, a client's expiry -- happens when the clock gets to it, at its\n"
    "time, and goes with the step during which it does: tasks due when a request\n"
    "arrives run before it, or after it with URWEB_REPRODUCIBLE_TASKS=last in the\n"
    "run's env.  The heap starts at 256 MiB, so that no handler is run a second\n"
    "time for the heap to grow; every request is for the host 'localhost', from the\n"
    "user agent 'urt', and says its time in an UrWeb-Time header.\n"
    "\n"
    "A step can wait instead of send, for what the application does by itself:\n"
    "NAME.wait holds one line, either\n"
    "\n"
    "  wait 1min\n"
    "\n"
    "which lets that much time pass (ms, s, min, h and d, the unit never left\n"
    "out; a task periodic 10 runs six times), or\n"
    "\n"
    "  until still running\n"
    "\n"
    "which lets time pass until the server has printed a line holding the text\n"
    "after 'until', the blanks around it aside, on stdout or stderr.  A line counts\n"
    "as the recording has it: one of the server's own lines, which are dropped,\n"
    "does not.  What the server did meanwhile goes under NAME, as for a request\n"
    "(NAME.dml.sql, NAME.stdout, NAME.stderr); there is no response.  The clock is\n"
    "moved from one thing the server is due to do to the next, so an until ends\n"
    "with everything due at the time of the line and nothing after it, and the\n"
    "next step comes a second later; it cannot be done if the server has nothing\n"
    "more to do, or the line has not come within 365 days on the clock.  Time\n"
    "passes as fast as the server does what falls due: a month of a task every\n"
    "minute is some 43,000 runs of it, which take as long as they take.\n"
    "\n"
    "A run's env adds to the application's environment, NAME=value a line, the\n"
    "value taken as written, though it holds no control character but a tab;\n"
    "blank lines and lines starting with # are skipped.\n"
    "What urt sets itself for the above cannot be set there: URWEB_SIGFILE_PATH,\n"
    "URWEB_SQLITE_DB_PATH, URWEB_DML_LOG, URWEB_REPRODUCIBLE_EPOCH,\n"
    "URWEB_REPRODUCIBLE_PRNG_SEED, URWEB_MIN_HEAP and TZ.\n"
    "\n"
    "A case with runs is compiled all the way, so its args cannot hold -stop or\n"
    "-tc, and it fails if that does not give an application.  The compiler builds\n"
    "it in urt's scratch directory, whatever the project says (-output), and from\n"
    "there it is moved into the case as app.exe, where it stays, to be started by\n"
    "hand if need be.\n"
    "A run's args cannot hold -t: its requests are sent one after the other, and\n"
    "with more threads than one it is a matter of chance which of them answers.\n"
    "Nor --, after which the server would ignore the flags urt adds (the address,\n"
    "the port range, -d3).  Runs need curl and sqlite3 (3.37 or later) on the PATH.\n"
    "\n"
    "A case can also have runs in a browser, under browser/: the application\n"
    "started afresh as above, once per pass, and every pass sent through it step\n"
    "by step, by one user or several, each in one tab or several, each in a browser\n"
    "engine -- Chromium, Firefox or WebKit.  A pass puts every user on an engine, and\n"
    "there are as many as it takes to drive each through all of theirs, every user\n"
    "through all three unless the run's context says fewer; the passes go at the\n"
    "same time, each with its application:\n"
    "\n"
    "  shop.urt/\n"
    "  `-- snapshot/\n"
    "      `-- sqlite/\n"
    "          `-- browser/\n"
    "              `-- checkout/             a run; args, env, secret, db.sql and out/ as above\n"
    "                  |-- context             the users' order, and how their browsers are set up\n"
    "                  |-- files/              what the steps upload\n"
    "                  |-- screenshots/        every page as drawn, with --screenshot; not for git\n"
    "                  |   `-- 01-open.Alice.1.chromium.png\n"
    "                  |-- recordings/         every user filmed, with --record; not for git\n"
    "                  |   |-- Alice.chromium.webm\n"
    "                  |   |-- Alice.chromium.vtt  a cue per line: the step and the line\n"
    "                  |   `-- index.html      plays a run's films together\n"
    "                  `-- interaction/\n"
    "                      |-- 01-open.step      what each user does\n"
    "                      |-- 01-open.timeline  what happened when: every line, every request\n"
    "                      |-- 01-open.Alice.1.page.html\n"
    "                      |                     the tab's page afterwards: its path, then the DOM\n"
    "                      |                     as the browser has it, scripts and handlers left out\n"
    "                      |-- 01-open.Alice.1.requests\n"
    "                      |                     what the tab fetched, with the status of each\n"
    "                      |-- 01-open.Alice.1.dialogs\n"
    "                      |                     the alerts, confirms and prompts, with their answers\n"
    "                      |-- 01-open.Alice.1.console\n"
    "                      |                     what the tab logged, and its script's errors\n"
    "                      |-- 01-open.dml.sql   what the step changed in the database, if anything\n"
    "                      |-- 01-open.stdout    what the server printed meanwhile, if anything\n"
    "                      |-- 01-open.stderr\n"
    "                      |-- 02-add.step\n"
    "                      |-- 02-add.Alice.1.page.html\n"
    "                      |                     what the passes agree on, shared\n"
    "                      |-- 02-add.Alice.1.page.firefox.html\n"
    "                      |                     what one of them records differently, its own\n"
    "                      |-- 03-pay.step\n"
    "                      `-- 03-pay.error      why the step could not be done, while it cannot\n"
    "\n"
    "A pass is named by its engine where every user is on the same one, as when\n"
    "they all have the same engines: chromium, firefox, webkit.  Where they do not,\n"
    "the i-th pass puts every user on the i-th of their engines in that order, the\n"
    "last again for a user with fewer, and a pass that mixes them is named by the\n"
    "users' engines in the context's order, joined by +: with Alice on chromium and\n"
    "Bernd on firefox and webkit, chromium+firefox and chromium+webkit.\n"
    "The passes agree on nearly everything, and a record they agree on is one\n"
    "file; one that a pass records differently is that pass's own, beside it,\n"
    "with the pass's name after the record's kind, and what that pass is compared\n"
    "against.  Recording goes by the majority: the passes agreeing in the greatest\n"
    "number write the shared file, the others their own, an own file whose pass\n"
    "has come to agree goes, and a tie goes to the pass first in order.\n"
    "\n"
    "A step holds lines, each for a user: 'USER: ACTION', or 'USER:' alone with the\n"
    "user's lines under it, each indented with a tab.  A user is a browser of their\n"
    "own, cookies and all; their tabs are pages in it, each a client to the\n"
    "application, named by the action that opens them ([target=NAME]), and their\n"
    "lines act on the tab in front.  The users of a step go side by side, each\n"
    "through their lines in order, and the step is over when all are done and no\n"
    "page has a request open but its poll; then every tab that is open is recorded,\n"
    "as USER.TAB.\n"
    "\n"
    "  Alice: open /Shop/main [target=1]\n"
    "  Alice: fill 'input[name=name]' kettle\n"
    "  Alice: click 'text=Add'\n"
    "  Bernd:\n"
    "  \topen /Shop/main [target=1]\n"
    "  \tuntil 'text=1 item'\n"
    "\n"
    "`urt-browse --help` lists the actions, their locators, which are Playwright's,\n"
    "and their parameters, and what the context can say, OPTION VALUE a line, for\n"
    "every user before the first line [USER] and for that user under it: the users'\n"
    "order, which a run with more than one needs; whether the server's tasks due\n"
    "when a request arrives run before it or after; the engines a user is driven\n"
    "through ('engines chromium webkit'); a request's round trip; how\n"
    "long a kind of act takes; how a hidden tab's timers are held back, as Chrome\n"
    "does unless it says otherwise; and how a user's browser is set up (the window,\n"
    "the colour scheme, the locale, ...), every user's being 1024x768, en-US, UTC\n"
    "and the user agent 'urt' unless it says otherwise.  With --screenshot, every tab\n"
    "is shot after every step as well, the whole page as the engine draws it, under\n"
    "the run's screenshots/ as NAME.USER.TAB.PASS.png, the directory emptied\n"
    "first: pictures to look at when a record wants explaining, which git ignores\n"
    "and nothing compares, since no two engines draw alike; the records are the\n"
    "same with and without.  With --record, every user is filmed on the run's\n"
    "clock, under the run's recordings/ as USER.PASS.webm: their tab in front,\n"
    "under a strip with their tabs, the clock and the line they are at, the pointer\n"
    "and any dialog drawn in.  A frame is taken every T (--record=T, 16ms unless\n"
    "given) at which anything shown may have changed, the clock stopping on those\n"
    "while anything moves that will stop; a USER.PASS.vtt beside it has a cue\n"
    "per line, and an index.html plays a run's films together, each in a box to\n"
    "move and size at will.  The same rules as for the screenshots, and the records\n"
    "the same with and without, and whatever T, too.  What a driver says on its own\n"
    "account, in its lines that start urt-browse: -- a film it could not make, a\n"
    "frame it had to make again, a film slow to make -- comes under the run's\n"
    "verdict, whatever the verdict.\n"
    "There is one clock, for the pages, the server and the users.  It starts at the\n"
    "epoch and moves from one thing due to the next, as fast as they get done,\n"
    "nothing waiting on the wall clock: an act takes the time it takes a person (a\n"
    "click 400ms, a key typed 150ms), a request reaches the server half a round\n"
    "trip after the page sent it and its answer the page half a round trip later\n"
    "(100ms the trip), a page's timer fires when it is due, and so does a task of\n"
    "the server's.  What is due at the same time is done in a fixed order: the\n"
    "server's part first, then the pages' timers and the answers coming back, then\n"
    "the users' input, the users in the context's order.  So a run comes out the\n"
    "same every time, whatever the machine, and NAME.timeline says what happened\n"
    "when, in seconds since the epoch.  What moves on its own in a page follows the\n"
    "clock as well: animations, smooth scrolls, and which tab of a user's is in\n"
    "front, visible and with the focus.  A last line 'wait T' in a step, for no\n"
    "user, lets T pass for everyone before the records are taken.  No engine caches\n"
    "or fetches a favicon, so that each sends the same requests.  A request for any\n"
    "other server is refused by the driver and recorded as such: a run drives this\n"
    "application, and the network is not its business; what the pages need, the\n"
    "application serves (the 'file' directive).  One engine difference is left as\n"
    "it is: back to a page that answered a POST sends the POST again in Chromium\n"
    "and WebKit, and is refused by Firefox, where the step cannot be done and says\n"
    "so; a run meant for every engine opens the page instead.\n"
    "A step that cannot be done -- no such element, two of them, an until whose\n"
    "element does not come in time -- stops the run, as a request that cannot be\n"
    "sent does: the step's NAME.error holds the engine's complaint, beside its\n"
    "records of the pages as the engine found them, the records of the steps after\n"
    "it go, and the run fails until it gets past the step again.  A pass that\n"
    "stops where the others go on has an own NAME.error.PASS, and own files,\n"
    "empty, for what it did not record after that.\n"
    "Browser runs need urt-browse beside urt, python3 with Playwright, and the\n"
    "engines where PLAYWRIGHT_BROWSERS_PATH says; the compiler's nix-shell and its\n"
    "package with urt have all of it.\n"
    "\n"
    "Nothing else may be under snapshot/, bar a README.md beside the cases or in\n"
    "one, or beside the runs or in one: a file this program would not read is\n"
    "refused rather than left to rot.\n"
    "The urt directory itself may hold a README.md, and a 'project' file naming\n"
    "the project when the directory's name does not; other tools keep their own\n"
    "directories there.\n"
    "\n"
    "\n"
    "USAGE\n"
    "-----\n"
    "\n"
    "  urt snapshot [PATH...]                     update the snapshots below the given projects\n"
    "  urt snapshot --whine [PATH...]             fail on a diff (you want this in CI)\n"
    "  urt snapshot --screenshot [PATH...]        also shoot every page of the browser runs\n"
    "  urt snapshot --record[=T] [PATH...]        also film every user of the browser runs\n"
    "  urt snapshot -j[N] [PATH...]               N projects at a time, or one per core\n"
    "  urt snapshot --list [PATH...]              list the cases matching the given path(s)\n"
    "  urt snapshot --create [PROJECT] [CASE...]  create cases for the given project\n"
    "  urt snapshot --create-run [PATH] [RUN...]  create runs for the one case at the given path\n"
    "  urt snapshot --create-browser-run [PATH] [RUN...]  the same, for runs in a browser\n"
    "  urt snapshot --edit [PATH] [ARTIFACT...]   edit the tracked set of a case\n"
    "\n"
    "PATH is a project (demo/alert[.urp]), its urt directory (demo/alert.urt), its\n"
    "snapshots (demo/alert.urt/snapshot), one case (demo/alert.urt/snapshot/1) or a\n"
    "directory to search for snapshots.  It defaults to the current directory.\n"
    "\n"
    "With -j N (or -jN, --jobs=N), N projects go at once, each in a process of its\n"
    "own that takes the project's cases one after the other; with -j alone, as\n"
    "many as there are cores.  What is printed is what a plain run prints, in the\n"
    "same order: a project's lines come when its turn comes.  A browser run drives\n"
    "its passes at once either way, so a project can keep three browsers busy.\n"
    "\n"
    "With '--create', cases are set up for a project, named as given or by the\n"
    "next number: an empty 'args' file and an 'expected' directory of empty\n"
    "artifacts each.  On a terminal the artifacts to track are ticked in a\n"
    "checklist and a run is offered to fill them in; otherwise all of them are\n"
    "made, to be pruned by hand and filled in by a plain run.  With no PROJECT it\n"
    "takes the only .urp in the current directory.\n"
    "\n"
    "With '--create-run', runs are set up for a case, named as given or by the\n"
    "next number: an empty 'args', a 'secret', the schema as the compiler gives it\n"
    "for the case as 'db.sql' if the program has tables, and a first request to\n"
    "replace.  '--create-browser-run' does the same under browser/, with a first\n"
    "step, 'user: open / [target=1]', to replace.  A plain run fills in the rest.\n"
    "\n"
    "With '--edit', the artifacts a case tracks are ticked in a checklist.  PATH\n"
    "names the case, or a project or directory holding several, to choose from;\n"
    "the current directory otherwise.  A newly tracked artifact starts empty, and\n"
    "a run is offered to fill it in.  Named outright, the ARTIFACTs become the\n"
    "tracked set without asking, for scripts.\n"
    "\n"
    "The compiler is the one URWEB names, run with the flags in URWEB_FLAGS if\n"
    "that is set, or else the in-tree build, bin/urweb under the current\n"
    "directory or beside this program, run with -boot -noEmacs.  A Nix-packaged\n"
    "urt has URWEB set to the compiler it was built for.\n"
    "\n";

/* -------------------------------------------------------- the artifacts */

/* Everything the compiler can be asked to write.  For each, how far the
 * compiler has to run -- a rank, and the phase it may stop after once
 * that artifact is written -- and the flag that asks for it; NULL for the
 * three that every run produces.
 *
 * The ranks are the only thing this program assumes about the order of
 * the compiler's phases, and a run that ends without producing something
 * it asked for says so rather than quietly comparing an empty file, so a
 * reordering in the compiler is loud here.  The assumption would go away
 * if the compiler could be told to stop once it has written everything it
 * was asked for. */
struct artifact { const char *name; int rank; const char *phase; const char *flag; };

static const struct artifact artifacts[] = {
    { "exitcode",       0, "parseJob",  NULL },
    { "stdout.txt",     0, "parseJob",  NULL },
    { "stderr.txt",     0, "parseJob",  NULL },
    { "settings.txt",   1, "parseJob",  "-saveSettings" },
    { "parsetree.ur",   2, "parse",     "-saveParsetree" },
    { "typecheck.urs",  3, "elaborate", "-saveTypecheck" },
    { "schema.sql",     4, "sqlify",    "-sql" },
    { "client.js",      5, "jscomp",    "-saveJs" },
    { "endpoints.json", 6, "endpoints", "-endpoints" },
    { "server.c",       7, "checknest", "-saveC" },
};
#define NARTIFACTS ((int) (sizeof artifacts / sizeof artifacts[0]))

/* The artifact of that name, or NULL. */
static const struct artifact *artifact_named(const char *name) {
    int i;
    for (i = 0; i < NARTIFACTS; i++) {
        if (!strcmp(artifacts[i].name, name)) {
            return &artifacts[i];
        }
    }
    return NULL;
}

/* All the names, space-separated, for a message. */
static const char *artifact_names(void) {
    static char names[256];
    int i;
    if (!*names) {
        for (i = 0; i < NARTIFACTS; i++) {
            if (i) {
                strcat(names, " ");
            }
            strcat(names, artifacts[i].name);
        }
    }
    return names;
}

/* ------------------------------------------------------------- settings */

static int opt_check = 0, opt_create = 0, opt_create_run = 0, opt_list = 0, opt_edit = 0, opt_screenshot = 0, opt_record = 0;
static int opt_jobs = 1, opt_jobs_given = 0;    /* -j: how many projects at once */
static const char *opt_frame = NULL;   /* --record=T: the time between a film's frames, as given */
static const char *me = "urt snapshot";   /* how to invoke us, for messages */
static char urweb[PATHLEN];               /* the compiler, absolute */
static struct strlist urweb_flags;        /* what it always gets */
static char tmp[PATHLEN] = "";            /* our scratch directory */
static char work[PATHLEN];                /* where a run's artifacts land */
static int diff_labels = 0;               /* GNU diff, which names the sides for us */

static int n_cases = 0, n_runs = 0, n_ok = 0, n_bad = 0, n_changed = 0, n_skipped = 0, n_nonzero = 0;
static long started;
/* The passes of a browser run go at the same time, each with its own
 * application and driver; there are as many as the most engines a user
 * has, three at most. */
#define PASSES_MAX 3

/* The applications now running, if any: a request-response run's, or a
 * browser run's, one a pass. */
static struct server servers[PASSES_MAX];

/* The browser drivers, one a pass of a run at once, started at the first
 * browser run that needs each and kept for the rest, with the engines
 * they started: an engine takes seconds to start, a run's contexts a
 * moment.  A pass goes to the driver that has its first user's engine
 * (the order of engines[]) when that one is free, so that a driver keeps
 * to one engine where the passes do.  Their stderr accumulates under tmp;
 * driver_err_at[i] is where the run underway began in it. */
static struct child drivers[PASSES_MAX];
static int driver_up[PASSES_MAX];
static long driver_err_at[PASSES_MAX];

static void drivers_stop(void) {
    struct child *up[PASSES_MAX];
    int i, n = 0;
    for (i = 0; i < PASSES_MAX; i++) {
        if (driver_up[i]) {
            up[n++] = &drivers[i];
            driver_up[i] = 0;
        }
    }
    children_stop(up, n);
}

/* The scratch directory goes with us, whichever way we leave, and so
 * do a server we started and the browsers. */
static void cleanup(void) {
    int i;
    for (i = 0; i < PASSES_MAX; i++) {
        server_stop(&servers[i]);
    }
    drivers_stop();
    if (*tmp) {
        rm_rf(tmp);
    }
    *tmp = 0;
}

/* After anything that may have taken a while: if a signal came, leave
 * the way the signal would have had us leave. */
static void bail_if_interrupted(void) {
    int sig = fs_interrupted;
    if (!sig) {
        return;
    }
    cleanup();
    signal(sig, SIG_DFL);
    raise(sig);
    exit(128 + sig);
}

/* Which compiler, into urweb, absolute: the one named, or else bin/urweb
 * under the current directory or beside this program -- an in-tree
 * build, which needs -boot to find the library in the source tree.
 * Never one found on the PATH: a packaged urt names the compiler it was
 * built for, in URWEB.  Made absolute because the project is named
 * relative to the current directory and that is where the compiler
 * runs.  Whether it is there at all is checked when a run is about to
 * happen. */
static void find_compiler(const char *named) {
    char cwd[PATHLEN], here[PATHLEN];
    const char *flags = getenv("URWEB_FLAGS");
    if (!getcwd(cwd, sizeof cwd)) {
        die("cannot tell the current directory", NULL);
    }
    if (named) {
        if (named[0] == '/') {
            snprintf(urweb, sizeof urweb, "%s", named);
        } else {
            pathf(urweb, "%s/%s", cwd, named);
        }
        if (flags) {
            split_words(flags, &urweb_flags);
        }
        return;
    }
    pathf(urweb, "%s/bin/urweb", cwd);
    if (!can_exec(urweb) && program_dir(here)) {
        pathf(urweb, "%s/../bin/urweb", here);
    }
    split_words(flags ? flags : "-boot -noEmacs", &urweb_flags);
}

/* A case name is letters, digits, _ and -, and nothing else, and does not
 * start with -: on a command line that would be an option. */
static int valid_case_name(const char *s) {
    if (!*s || *s == '-') {
        return 0;
    }
    for (; *s; s++) {
        if (!strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-", *s)) {
            return 0;
        }
    }
    return 1;
}

/* Strip one suffix from s in place, if it ends with it. */
static void strip_suffix(char *s, const char *suffix) {
    if (has_suffix(s, suffix)) {
        s[strlen(s) - strlen(suffix)] = 0;
    }
}

/* --------------------------------------------------------- finding cases */

/* What each PATH argument means -- a project, its urt directory, its
 * snapshots, one case, or a directory to search -- as a list of snapshot
 * directories (X.urt/snapshot) and case directories (X.urt/snapshot/CASE),
 * in order, the search results sorted. */
static void find_cases(int argc, char **argv, struct strlist *snaps) {
    char p[PATHLEN], parent[PATHLEN], q[PATHLEN];
    struct strlist found = { 0, 0, 0 };
    int i, j, hit;

    for (i = 0; i < argc; i++) {
        snprintf(p, sizeof p, "%s", argv[i]);
        strip_suffix(p, "/");
        hit = 0;
        if (has_suffix(p, ".urt")) {
            join(q, p, "snapshot");
            if (is_dir(q)) {
                sl_push(snaps, q);
                hit = 1;
            }
        } else if (has_suffix(p, ".urt/snapshot")) {
            if (is_dir(p)) {
                sl_push(snaps, p);
                hit = 1;
            }
        } else if (strstr(p, ".urt/snapshot/")) {
            /* One case, named by its directory. */
            dirname_of(parent, p);
            if (has_suffix(parent, ".urt/snapshot")) {
                join(q, p, "expected");
                if (is_dir(q)) {
                    sl_push(snaps, p);
                    hit = 1;
                }
            }
        }
        if (!hit) {
            snprintf(q, sizeof q, "%s", p);
            strip_suffix(q, ".urp");
            strcat(q, ".urt/snapshot");
            if (is_dir(q)) {
                sl_push(snaps, q);
                hit = 1;
            } else if (is_dir(p)) {
                find_dirs(p, ".urt", &found);
                for (j = 0; j < found.n; j++) {
                    join(q, found.v[j], "snapshot");
                    if (is_dir(q)) {
                        sl_push(snaps, has_prefix(q, "./") ? q + 2 : q);
                    }
                }
                sl_free(&found);
                hit = 1;
            }
        }
        if (!hit) {
            die(fmt("nothing to test at %s", path(p)),
                fmt("expected a project (%s), its snapshots (%s),", path(fmt("%s.urp", p)), path(fmt("%s.urt/snapshot", p))),
                fmt("one case (%s), or a directory to search for snapshots", path(fmt("%s.urt/snapshot/CASE", p))), NULL);
        }
    }
}

/* ---------------------------------------------------------------- layout */

/* The two kinds of run a case can have, by the directory they are under:
 * request-response/ for runs that send requests, browser/ for runs that
 * drive a browser.  They share args, secret, db.sql and out/; under
 * interaction/ the one has NAME.request and what came of it, the other
 * NAME.step and what came of that. */
enum run_kind { RUN_REQUESTS, RUN_BROWSER };

static const char *const run_dirname[] = { "request-response", "browser" };

/* What the inputs under interaction/ end in, per kind: a request-response
 * run's are requests, and waits beside them (NAME.wait). */
static const char *const run_input[] = { ".request", ".step" };

/* The suffix of an input of a run of the kind that file is, the one it
 * has (a request may be called X.wait.request, which the layout then
 * refuses for its dot), or NULL if it is not one. */
static const char *input_suffix(const char *file, enum run_kind kind) {
    if (has_suffix(file, run_input[kind])) {
        return run_input[kind];
    }
    return kind == RUN_REQUESTS && has_suffix(file, ".wait") ? ".wait" : NULL;
}

/* What an input with that suffix is, for a message: "a request". */
static const char *input_what(const char *suffix) {
    return !strcmp(suffix, ".wait") ? "a wait" : !strcmp(suffix, ".step") ? "a step" : "a request";
}

/* The records, by kind.  A record is NAME.KIND, for the request or the
 * step NAME; a tab's record in a browser run is NAME.USER.TAB.KIND, USER.TAB
 * being the tab's name as the steps give it; the records under out/ are
 * startup.KIND and shutdown.KIND.  A kind that is a language has its
 * extension last (NAME.dml.sql, NAME.Alice.1.page.html), and one pass's
 * own record (see settle_shared) has the pass's name after the kind
 * (NAME.dml.firefox.sql, NAME.stdout.firefox, NAME.Alice.1.page.firefox.html,
 * NAME.Alice.1.page.chromium+webkit.html), so that it sorts beside the
 * shared one.  No name has a dot in it -- not
 * a request's or a step's, a user's or a tab's -- and the kinds are
 * fixed, so a record's name is read from its end.  NAME.error is the
 * record of a request that could not be sent, or a step that could not be
 * done: the run stops there, and the record says why, until the run gets
 * past it again. */
struct record_kind {
    const char *kind;
    const char *ext;        /* ".sql", ".html" or "" */
    int tab;                /* a tab's, NAME.USER.TAB.KIND */
    int only;               /* the run kind it is for, or -1 for both */
};

static const struct record_kind record_kinds[] = {
    { "response", "",      0, RUN_REQUESTS },
    { "dml",      ".sql",  0, -1 },
    { "stdout",   "",      0, -1 },
    { "stderr",   "",      0, -1 },
    { "error",    "",      0, -1 },
    { "timeline", "",      0, RUN_BROWSER },
    { "page",     ".html", 1, RUN_BROWSER },
    { "requests", "",      1, RUN_BROWSER },
    { "dialogs",  "",      1, RUN_BROWSER },
    { "console",  "",      1, RUN_BROWSER },
    { "db",       ".sql",  0, -1 },          /* shutdown.db.sql alone: the database left behind */
    { NULL, NULL, 0, 0 }
};

/* The records of a run under out/, NAME.KIND: what the application did
 * before the first request and printed after the last, and the database
 * it left behind. */
static const char *const out_records[] = { "startup.dml", "startup.stdout", "startup.stderr",
                                           "shutdown.stdout", "shutdown.stderr", "shutdown.db", NULL };

static const struct record_kind *kind_named(const char *s) {
    int i;
    for (i = 0; record_kinds[i].kind; i++) {
        if (!strcmp(s, record_kinds[i].kind)) {
            return &record_kinds[i];
        }
    }
    return NULL;
}

/* The kind of a run, from the directory it is under. */
static enum run_kind kind_of(const char *run_dir) {
    char d[PATHLEN];
    dirname_of(d, run_dir);
    return !strcmp(basename_of(d), run_dirname[RUN_BROWSER]) ? RUN_BROWSER : RUN_REQUESTS;
}

/* The browser engines, in the order the driver names a pass by. */
static const char *const engines[] = { "chromium", "firefox", "webkit", NULL };

/* The place of the engine that s starts with, up to its end or a +, in
 * engines[]; -1 for none. */
static int engine_at(const char *s) {
    int i;
    for (i = 0; engines[i]; i++) {
        size_t n = strlen(engines[i]);
        if (!strncmp(s, engines[i], n) && (s[n] == 0 || s[n] == '+')) {
            return i;
        }
    }
    return -1;
}

/* Whether s names a pass of a browser run: an engine, every user on it,
 * or the users' engines in their order, joined by + (chromium+firefox). */
static int is_pass(const char *s) {
    for (;;) {
        int i = engine_at(s);
        if (i < 0) {
            return 0;
        }
        s += strlen(engines[i]);
        if (!*s) {
            return 1;
        }
        s++;
    }
}

/* A record's name, NAME[.USER.TAB].KIND[.PASS][EXT], into dst (PATHLEN):
 * label and pass may be empty. */
static void record_name(char *dst, const char *name, const char *label, const struct record_kind *k, const char *pass) {
    pathf(dst, "%s%s%s.%s%s%s%s", name, *label ? "." : "", label, k->kind, *pass ? "." : "", pass, k->ext);
}

/* "a" or "an" before a word. */
static const char *article(const char *word) {
    return strchr("aeiou", word[0]) ? "an" : "a";
}

/* What a record's name says, read from its end: the kind, the pass
 * when it is one pass's own, and before them NAME or NAME.USER.TAB.
 * run is the kind of run it is in, or -1 for any.  NAME goes into name,
 * USER.TAB or nothing into label, the pass or nothing into engine
 * (PATHLEN each; any may be NULL).  Returns the kind, or NULL: with why
 * (why_size) saying what is wrong when the name has a kind but not the
 * shape, or not for this kind of run, and why empty when it has no
 * kind, and so is not a record's name at all. */
static const struct record_kind *parse_record(const char *file, int run, char *name, char *label, char *engine,
                                              char *why, size_t why_size) {
    char copy[PATHLEN], *parts[9], *p;
    const char *ext = "", *eng = "";
    const struct record_kind *k;
    int n = 0;

    *why = 0;
    snprintf(copy, sizeof copy, "%s", file);
    for (p = strtok(copy, "."); p && n < 9; p = strtok(NULL, ".")) {
        parts[n++] = p;
    }
    if (n < 2 || n > 8) {
        return NULL;
    }
    if (!strcmp(parts[n - 1], "html") || !strcmp(parts[n - 1], "sql")) {
        ext = fmt(".%s", parts[--n]);
    }
    if (n >= 3 && is_pass(parts[n - 1]) && kind_named(parts[n - 2])) {
        eng = parts[--n];
    }
    k = kind_named(parts[n - 1]);
    if (!k) {
        return NULL;
    }
    n--;
    if (run != -1 && k->only != -1 && k->only != run) {
        snprintf(why, why_size, "%s %s record is a %s's", article(k->kind), k->kind, k->only == RUN_BROWSER ? "step" : "request");
        return NULL;
    }
    if (run == RUN_REQUESTS && *eng) {
        snprintf(why, why_size, "a pass's own record is a browser run's; here %s %s record is NAME.%s%s",
                 article(k->kind), k->kind, k->kind, k->ext);
        return NULL;
    }
    if (strcmp(k->ext, ext)) {
        snprintf(why, why_size, "%s %s record ends in %s%s, as in NAME%s.%s%s", article(k->kind), k->kind, k->kind, k->ext,
                 k->tab ? ".USER.TAB" : "", k->kind, k->ext);
        return NULL;
    }
    if (n != (k->tab ? 3 : 1)) {
        if (k->tab) {
            snprintf(why, why_size, "a tab's record is NAME.USER.TAB.%s%s, USER.TAB the tab's name as the steps give it, "
                     "or NAME.USER.TAB.%s.PASS%s for one pass's own", k->kind, k->ext, k->kind, k->ext);
        } else {
            snprintf(why, why_size, "%s %s record is NAME.%s%s, or NAME.%s.PASS%s for one pass's own",
                     article(k->kind), k->kind, k->kind, k->ext, k->kind, k->ext);
        }
        return NULL;
    }
    if (name) {
        snprintf(name, PATHLEN, "%s", parts[0]);
    }
    if (label) {
        if (k->tab) {
            pathf(label, "%s.%s", parts[1], parts[2]);
        } else {
            *label = 0;
        }
    }
    if (engine) {
        snprintf(engine, PATHLEN, "%s", eng);
    }
    return k;
}

/* A record the passes agree on is one file, NAME.KIND; one that a pass
 * records differently from the others is that pass's own,
 * NAME.KIND.PASS, beside it.  own_name gives the own name for a
 * record's relative path: interaction/04-send.Alice.1.page.html, firefox
 * -> interaction/04-send.Alice.1.page.firefox.html. */
static void own_name(char *dst, const char *rel, const char *pass) {
    char name[PATHLEN], label[PATHLEN], own[PATHLEN], why[512], dir[PATHLEN];
    const struct record_kind *k = parse_record(basename_of(rel), -1, name, label, NULL, why, sizeof why);
    if (!k) {
        die(fmt("%s is not a record's name", rel), NULL);   /* ours are all named by record_name */
    }
    record_name(own, name, label, k, pass);
    dirname_of(dir, rel);
    pathf(dst, "%s%s%s", strcmp(dir, ".") ? dir : "", strcmp(dir, ".") ? "/" : "", own);
}

/* Whether name is one of the records under out/ (startup.dml.sql,
 * shutdown.stdout, ...), or in a browser run a pass's own one
 * (startup.dml.firefox.sql). */
static int out_record(const char *name, enum run_kind kind) {
    char rec[PATHLEN], engine[PATHLEN], why[512];
    const struct record_kind *k = parse_record(name, kind, rec, NULL, engine, why, sizeof why);
    int i;
    if (!k) {
        return 0;
    }
    for (i = 0; out_records[i]; i++) {
        if (!strcmp(out_records[i], fmt("%s.%s", rec, k->kind))) {
            return 1;
        }
    }
    return 0;
}

/* The same strictness for the runs of a case.  e is the entry, rest what
 * follows CASE/request-response (or CASE/browser) in its path: "" for
 * the directory itself, then /RUN, /RUN/args and so on.  Returns 1 when
 * e is a directory that was refused, for the walk to pass over what it
 * holds. */
static int layout_run(const char *e, const char *rest, enum run_kind kind) {
    char part[4][PATHLEN], name[PATHLEN], label[PATHLEN], engine[PATHLEN], why[512];
    const char *suffix;
    const struct record_kind *k;
    const char *p = rest;
    int n = 0;

    while (*p == '/' && n < 4) {
        const char *end = strchr(++p, '/');
        snprintf(part[n++], PATHLEN, "%.*s", end ? (int) (end - p) : (int) strlen(p), p);
        p = end ? end : p + strlen(p);
    }
    if (*p) {
        n = 5; /* deeper than anything goes */
    }

    if (n == 0) {
        if (!is_dir(e)) {
            die_later(fmt("%s should be a directory", path(e)), NULL);
        }
    } else if (n <= 2 && !strcmp(part[n - 1], "README.md")) {
        if (!is_file(e)) {
            die_later(fmt("%s should be a file", path(e)), NULL);
        }
    } else if (n == 1) {
        if (!is_dir(e)) {
            die_later(fmt("%s does not belong among the runs", path(e)),
                      "beside the run directories there can be a README.md, nothing else", NULL);
        } else if (!valid_case_name(part[0])) {
            die_later(fmt("%s is not a name for a run", path(e)),
                      "the name of a run is letters, digits, _ and -, and nothing else, not starting with -", NULL);
            return 1;
        }
    } else if (n == 2 && (!strcmp(part[1], "args") || !strcmp(part[1], "secret") || !strcmp(part[1], "db.sql")
                          || !strcmp(part[1], "env"))) {
        if (!is_file(e)) {
            die_later(fmt("%s should be a file", path(e)), NULL);
        }
    } else if (n == 2 && kind == RUN_BROWSER && !strcmp(part[1], "files")) {
        /* What the steps upload, called by their names there: any file,
         * read by the driver. */
        if (!is_dir(e)) {
            die_later(fmt("%s should be a directory", path(e)), NULL);
        }
        return 1;
    } else if (n == 2 && kind == RUN_BROWSER && (!strcmp(part[1], "screenshots") || !strcmp(part[1], "recordings"))) {
        /* Pictures for looking at, --screenshot's and --record's, never
         * read; what is under them is passed over. */
        if (!is_dir(e)) {
            die_later(fmt("%s should be a directory", path(e)), NULL);
        }
        return 1;
    } else if (n == 2 && kind == RUN_BROWSER && !strcmp(part[1], "context")) {
        /* How the users' browsers are set up, and the run's tie rule and
         * timing; the driver reads it, and says what is wrong with it. */
        if (!is_file(e)) {
            die_later(fmt("%s should be a file", path(e)), NULL);
        }
    } else if (n == 2 && kind == RUN_BROWSER && has_suffix(part[1], ".context")) {
        die_later(fmt("%s is a user's context of before", path(e)),
                  "a run's users are set up in one file, context, each under a line [USER]; move its lines there", NULL);
    } else if (n == 2 && (!strcmp(part[1], "interaction") || !strcmp(part[1], "out"))) {
        if (!is_dir(e)) {
            die_later(fmt("%s should be a directory", path(e)), NULL);
        }
    } else if (n == 3 && !strcmp(part[1], "interaction") && is_file(e) && (suffix = input_suffix(part[2], kind)) != NULL) {
        if (strchr(part[2], '.') != strrchr(part[2], '.')) {
            /* A record's name is its dot-separated parts, NAME.USER.TAB.page.html
             * and so on, and no name has a dot in it. */
            die_later(fmt("%s is not a name for %s", path(e), input_what(suffix)),
                      fmt("the name of %s has no dot in it: its records are %s and so on", input_what(suffix),
                          kind == RUN_BROWSER ? "NAME.USER.TAB.page.html" : !strcmp(suffix, ".wait") ? "NAME.stdout" : "NAME.response"),
                      NULL);
        }
    } else if (n == 3 && !strcmp(part[1], "interaction") && is_file(e)
               && ((k = parse_record(part[2], kind, name, label, engine, why, sizeof why)) != NULL || *why)) {
        /* A record, with its request or its step: NAME.KIND, or in a
         * browser run a tab's NAME.USER.TAB.KIND, either a pass's own
         * with the pass's name after the kind. */
        char input[PATHLEN + 16], wait[PATHLEN + 16], dir[PATHLEN];
        if (!k) {
            die_later(fmt("%s is not named as a record", path(e)), why, NULL);
        } else if (!strcmp(k->kind, "db")) {
            die_later(fmt("%s is not named as a record", path(e)),
                      "the database left behind is the one db record, shutdown.db.sql under out/", NULL);
        } else {
            dirname_of(dir, e);
            pathf(input, "%s/%s%s", dir, name, run_input[kind]);
            pathf(wait, "%s/%s.wait", dir, name);
            if (kind == RUN_REQUESTS && strcmp(k->kind, "response")) {
                /* What the server did meanwhile, or why the step could not
                 * be done: a request's or a wait's. */
                if (!is_file(input) && !is_file(wait)) {
                    die_later(fmt("%s belongs to no request or wait", path(e)),
                              fmt("there is neither %s nor %s; delete it, or write one of them", path(input), path(wait)),
                              NULL);
                }
            } else if (!is_file(input)) {
                die_later(fmt("%s %s", path(e), kind == RUN_BROWSER ? "records no step" : "answers no request"),
                          fmt("there is no %s; delete the one, or write the other", path(input)), NULL);
            }
        }
    } else if (n == 3 && !strcmp(part[1], "out") && is_file(e) && out_record(part[2], kind)) {
        ;
    } else {
        die_later(fmt("%s does not belong in a run", path(e)),
                  kind == RUN_BROWSER
                      ? "a browser run holds args, env, secret, db.sql, context, files/, screenshots/, recordings/ and README.md; NAME.step, and per tab"
                      : "a run holds args, env, secret, db.sql and README.md; NAME.request, NAME.wait, NAME.response, NAME.dml.sql,",
                  kind == RUN_BROWSER
                      ? "NAME.USER.TAB.page.html, NAME.USER.TAB.requests, NAME.USER.TAB.dialogs and NAME.USER.TAB.console, and per step"
                      : "NAME.stdout, NAME.stderr and NAME.error under interaction/; startup.dml.sql, startup.stdout, startup.stderr,",
                  kind == RUN_BROWSER
                      ? "NAME.dml.sql, NAME.stdout, NAME.stderr and NAME.error under interaction/; startup.dml.sql, startup.stdout,"
                      : "shutdown.stdout, shutdown.stderr and shutdown.db.sql under out/",
                  kind == RUN_BROWSER
                      ? "startup.stderr, shutdown.stdout, shutdown.stderr and shutdown.db.sql under out/; any of them with a pass's" : NULL,
                  kind == RUN_BROWSER
                      ? "name after the kind (NAME.USER.TAB.page.firefox.html, NAME.stdout.firefox), when that pass records it differently" : NULL, NULL);
        return is_dir(e);
    }
    return 0;
}

/* Everything under a snapshot directory is read by this program, or it is
 * a mistake -- a file left over from an older layout, a typo for args, an
 * artifact written beside expected/ instead of in it -- and it would sit
 * there unnoticed, so it is refused.  README.md is the one thing allowed
 * through, beside the cases or in one.  The urt directory above it may
 * hold a README.md and a project file; what other tools keep there in
 * directories of their own is theirs, but a stray file is refused too.
 *
 * The tree is walked in sorted order, so a directory is judged before what
 * it holds, and a stray one is reported once, by its own name.  Returns 1
 * if all is well; the snapshot is left alone otherwise. */
static int layout(const char *urt_dir, const char *snap) {
    struct strlist tree = { 0, 0, 0 }, refused = { 0, 0, 0 };
    char e[PATHLEN];
    int before = report_problems, i, j;

    list_dir(urt_dir, LS_ALL, &tree);
    for (i = 0; i < tree.n; i++) {
        const char *name = tree.v[i];
        join(e, urt_dir, name);
        if (!strcmp(name, "README.md") || !strcmp(name, "project")) {
            if (!is_file(e)) {
                die_later(fmt("%s should be a file", path(e)), NULL);
            }
        } else if (!strcmp(name, "snapshot")) {
            if (!is_dir(e)) {
                die_later(fmt("%s should be a directory", path(e)), NULL);
            }
        } else if (!is_dir(e)) {
            die_later(fmt("%s does not belong in an urt directory", path(e)),
                      "beside snapshot/ there can be README.md and project, and other tools' directories",
                      "delete it, or move it where it goes", NULL);
        }
    }
    sl_free(&tree);

    walk(snap, &tree);
    for (i = 0; i < tree.n; i++) {
        const char *rel = tree.v[i], *rr;
        for (j = 0; j < refused.n && !has_prefix(rel, refused.v[j]); j++) {
            ;
        }
        if (j < refused.n) {
            continue; /* under something already refused */
        }
        join(e, snap, rel);
        /* A link is judged by what it points at, and then listed as
         * neither file nor directory, so a linked run or request would
         * pass and be left out without a word. */
        if (is_link(e)) {
            die_later(fmt("%s is a symbolic link", path(e)),
                      "a snapshot is plain files and directories; nothing in it is read through a link",
                      "copy what it points at into its place, or delete it", NULL);
            continue;
        }
        if ((rr = strstr(rel, "/request-response")) && rr == strchr(rel, '/') && (!rr[17] || rr[17] == '/')) {
            if (layout_run(e, rr + 17, RUN_REQUESTS)) {
                sl_push(&refused, fmt("%s/", rel));
            }
        } else if ((rr = strstr(rel, "/browser")) && rr == strchr(rel, '/') && (!rr[8] || rr[8] == '/')) {
            if (layout_run(e, rr + 8, RUN_BROWSER)) {
                sl_push(&refused, fmt("%s/", rel));
            }
        } else if (!strcmp(rel, "README.md") || has_suffix(rel, "/README.md") || has_suffix(rel, "/args")) {
            if (!is_file(e)) {
                die_later(fmt("%s should be a file", path(e)), NULL);
            }
        } else if (has_suffix(rel, "/expected")) {
            if (!is_dir(e)) {
                die_later(fmt("%s should be a directory", path(e)), NULL);
            }
        } else if (strstr(rel, "/expected/")) {
            if (!is_file(e) || !artifact_named(basename_of(rel))) {
                die_later(fmt("%s is not something the compiler produces", path(e)),
                          fmt("the artifacts are: %s", artifact_names()),
                          "delete that, or correct its name", NULL);
            }
        } else if (has_suffix(rel, "/app.exe") && strchr(rel, '/') == strrchr(rel, '/')) {
            if (!is_file(e)) {
                die_later(fmt("%s should be a file", path(e)), NULL);
            }
        } else if (strchr(rel, '/')) {
            die_later(fmt("%s does not belong in a case", path(e)),
                      "a case holds args, README.md, expected/, request-response/, browser/ and app.exe, nothing else",
                      "delete it, or move it where it goes", NULL);
        } else if (!strcmp(rel, "expected") || !strcmp(rel, "args")) {
            die_later(fmt("%s is not in a case", path(e)),
                      "every snapshot is a named case now: move args and expected/ into one",
                      fmt("such as %s", path(fmt("%s/1", snap))), NULL);
            sl_push(&refused, fmt("%s/", rel));
        } else if (!is_dir(e)) {
            die_later(fmt("%s does not belong in a snapshot", path(e)),
                      "beside the case directories there can be a README.md, nothing else",
                      "delete it, or move it where it goes", NULL);
        } else if (!valid_case_name(rel)) {
            die(fmt("%s is not a case name", path(e)),
                "a case name is letters, digits, _ and -, and nothing else, not starting with -",
                "rename it", NULL);
        }
    }
    sl_free(&tree);
    sl_free(&refused);
    return report_problems == before;
}

/* ------------------------------------------------------------ redundancy */

/* A case's args with whitespace folded: runs of it become one space, and
 * one leading and one trailing space go, so a trailing newline or a second
 * space does not tell two cases apart. */
static char *folded_args(const char *case_dir) {
    char f[PATHLEN], *raw, *out, *w;
    const char *r;
    int in_space = 0;
    join(f, case_dir, "args");
    raw = is_file(f) ? read_text(f) : NULL;
    if (!raw) {
        return xstrdup("");
    }
    out = w = xmalloc(strlen(raw) + 1);
    for (r = raw; *r; r++) {
        if (strchr(" \t\n\v\f\r", *r)) {
            if (!in_space) {
                *w++ = ' ';
            }
            in_space = 1;
        } else {
            *w++ = *r;
            in_space = 0;
        }
    }
    *w = 0;
    free(raw);
    if (*out == ' ') {
        memmove(out, out + 1, strlen(out));
    }
    if (*out && out[strlen(out) - 1] == ' ') {
        out[strlen(out) - 1] = 0;
    }
    return out;
}

struct twin { char *args; const char *name; };

static int by_args_then_name(const void *a, const void *b) {
    const struct twin *x = a, *y = b;
    int c = strcmp(x->args, y->args);
    return c ? c : strcmp(x->name, y->name);
}

/* Two cases of one project that give the compiler the same args snapshot
 * the same run; the artifacts they ask for could be asked for by one
 * case.  Say so. */
static void redundant(const char *snap, const struct strlist *subdirs) {
    struct twin *twins = xmalloc((size_t) subdirs->n * sizeof *twins);
    char cases[PATHLEN];
    int i, j;

    for (i = 0; i < subdirs->n; i++) {
        twins[i].args = folded_args(subdirs->v[i]);
        twins[i].name = basename_of(subdirs->v[i]);
    }
    qsort(twins, (size_t) subdirs->n, sizeof *twins, by_args_then_name);
    for (i = 0; i < subdirs->n; i = j) {
        for (j = i + 1; j < subdirs->n && !strcmp(twins[j].args, twins[i].args); j++) {
            ;
        }
        if (j - i < 2) {
            continue;
        }
        snprintf(cases, sizeof cases, "%s", test(twins[i].name));
        for (int k = i + 1; k < j; k++) {
            snprintf(cases + strlen(cases), sizeof cases - strlen(cases), ", %s", test(twins[k].name));
        }
        warn(fmt("%s: cases %s pass the same args (%s); one case would do",
                 test(snap), cases, *twins[i].args ? twins[i].args : "none"));
    }
    for (i = 0; i < subdirs->n; i++) {
        free(twins[i].args);
    }
    free(twins);
}

/* ------------------------------------------------------------ normalising */

/* The artifacts are written in a temporary directory, and some of them say
 * where that was -- the settings dump names every file the job was given,
 * so a project that keeps both settings.txt and endpoints.json would
 * otherwise snapshot a path that is different on every run.  The
 * directory goes, leaving the name the snapshot knows the file by.
 *
 * And two things in the generated C say more about the machine it was
 * built on than about the program: the include paths, absolute because
 * ./configure was given a prefix, and the client-side script, embedded as
 * two string literals holding a hundred kilobytes of runtime and program
 * on two lines.  client.js keeps the half of that which is the compiler's
 * work, in a form one can read.
 *
 * Each rule below takes a line and rewrites it in place; the line keeps
 * its newline, if it had one. */

/* Every occurrence of work/ goes. */
static void strip_work(char *line) {
    char prefix[PATHLEN], *hit;
    size_t n;
    pathf(prefix, "%s/", work);
    n = strlen(prefix);
    while ((hit = strstr(line, prefix))) {
        memmove(hit, hit + n, strlen(hit + n) + 1);
    }
}

/* Optional spaces, then what: the start of the rest, or NULL. */
static char *after_indent(char *line, const char *what) {
    while (*line == ' ') {
        line++;
    }
    return has_prefix(line, what) ? line + strlen(what) : NULL;
}

/* The tree the compiler ran in, resolved, with a trailing slash; NULL if that
 * does not fit.  The compiler writes a project's own `include` headers by
 * absolute path. */
static const char *tree_prefix(void) {
    static char prefix[PATHLEN];
    static int done;
    if (!done) {
        size_t n;
        done = 1;
        if (!realpath(".", prefix) || (n = strlen(prefix)) + 2 > sizeof prefix) {
            prefix[0] = 0;
        } else {
            strcpy(prefix + n, "/");
        }
    }
    return *prefix ? prefix : NULL;
}

/* #include "…/include/urweb/x.h" keeps only the part from include/ on; a header
 * under the tree keeps its path from the tree's root. */
static void fix_include_quoted(char *line) {
    char *q = after_indent(line, "#include \""), *end, *hit = NULL, *s;
    const char *tree;
    if (!q || !(end = strchr(q, '"'))) {
        return;
    }
    for (s = q; (s = strstr(s, "/include/urweb/")) && s < end; s++) {
        hit = s;
    }
    if (hit) {
        memmove(q, hit + 1, strlen(hit + 1) + 1);
        return;
    }
    tree = tree_prefix();
    if (tree && has_prefix(q, tree) && q + strlen(tree) < end) {
        memmove(q, q + strlen(tree), strlen(q + strlen(tree)) + 1);
    }
}

/* #include </…/x.h> keeps only the file's name. */
static void fix_include_angled(char *line) {
    char *q = after_indent(line, "#include <"), *end, *last;
    if (!q || *q != '/' || !(end = strchr(q, '>'))) {
        return;
    }
    *end = 0;
    last = strrchr(q + 1, '/');
    *end = '>';
    if (!last) {
        return;
    }
    memmove(q, last + 1, strlen(last + 1) + 1);
}

/* static char NAME[] = "…"; loses the literal, which is never empty. */
static void elide_literal(char *line, const char *name, const char *instead) {
    char *q = after_indent(line, fmt("static char %s[] = ", name)), *tail;
    size_t n;
    if (!q || *q != '"') {
        return;
    }
    n = strlen(q);
    while (n && (q[n - 1] == '\n' || q[n - 1] == '\r')) {
        n--;
    }
    if (n < 4 || q[n - 1] != ';' || q[n - 2] != '"') {
        return;
    }
    tail = xstrdup(q + n);
    sprintf(q, "\"%s\";%s", instead, tail);
    free(tail);
}

static void normalise(const char *name, const char *from, const char *to) {
    FILE *in = fopen(from, "rb"), *out = fopen(to, "wb");
    char *line = NULL;
    size_t cap = 0;
    int is_c = !strcmp(name, "server.c");
    if (!in || !out) {
        die(fmt("cannot normalise %s: %s", path(from), strerror(errno)), NULL);
    }
    while (getline(&line, &cap, in) > 0) {
        strip_work(line);
        if (is_c) {
            fix_include_quoted(line);
            fix_include_angled(line);
            elide_literal(line, "jslib", "*runtime elided*");
            elide_literal(line, "jsapp", "*script elided*");
        }
        fputs(line, out);
    }
    free(line);
    fclose(in);
    if (fclose(out) != 0) {
        die(fmt("cannot write %s: %s", path(to), strerror(errno)), NULL);
    }
}

/* --------------------------------------------------------------- running */

/* "1.2s" for the result line, or nothing when the run was quick. */
static const char *took(long tenths) {
    if (tenths <= 10) {
        return "";
    }
    return fmt(" %s", info(fmt("%ld.%lds", tenths / 10, tenths % 10)));
}

/* Append ", name" (or "name") to a comma-separated list. */
static void list_add(char *list, size_t size, const char *name) {
    snprintf(list + strlen(list), size - strlen(list), "%s%s", *list ? ", " : "", name);
}

/* diff -u of the recorded artifact against the new one, into a file;
 * non-zero when they differ. */
static int diff_into(const char *want, const char *now, const char *out) {
    char *argv[10];
    int n = 0;
    argv[n++] = "diff";
    argv[n++] = "-u";
    if (diff_labels) {
        argv[n++] = "--label"; argv[n++] = (char *) want;
        argv[n++] = "--label"; argv[n++] = (char *) fmt("%s (now)", want);
    }
    /* A file that is not there compares as an empty one. */
    argv[n++] = (char *) (exists(want) ? want : "/dev/null");
    argv[n++] = (char *) (exists(now) ? now : "/dev/null");
    argv[n] = NULL;
    return run(argv, out, NULL);
}

/* Whether the last line printed was blank, so that a block can be set
 * apart by one blank line and no more. */
static int at_blank = 0;

/* With -j (run_parallel), what a worker prints is printed again in the
 * main process, its stderr lines marked (report_merge), and these marks
 * at the start of a line besides: a worker cannot know what was printed
 * before its project, so whether a blank line is due is decided there. */
#define MARK_DONE  '\003'      /* the project is done; the counts follow */
#define MARK_BLANK '\004'      /* a blank line, unless the last line printed was one */
#define MARK_AT    '\005'      /* at_blank is now 0 or 1 */

static int in_worker = 0;       /* this process is one of -j's */
static char debug_lock[PATHLEN];    /* what -j's workers take turns at, compiling in debug mode */

/* A blank line before a block, unless there is one. */
static void blank_before(void) {
    if (in_worker) {
        printf("%c\n", MARK_BLANK);
    } else if (!at_blank) {
        putchar('\n');
    }
}

static void set_at_blank(int v) {
    at_blank = v;
    if (in_worker) {
        printf("%c%d\n", MARK_AT, v);
    }
}

/* A diff, indented under the verdict it explains: headers grey, what
 * went red, what came green. */
static void show_diff(const char *file) {
    FILE *f = fopen(file, "rb");
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    if (!f) {
        return;
    }
    while ((n = getline(&line, &cap, f)) > 0) {
        enum style st = ST_RESET;
        if (line[n - 1] == '\n') {
            line[n - 1] = 0;
        }
        if (has_prefix(line, "--- ") || has_prefix(line, "+++ ") || has_prefix(line, "@@ ")) {
            st = ST_INFO;
        } else if (line[0] == '-') {
            st = ST_RED;
        } else if (line[0] == '+') {
            st = ST_GREEN;
        }
        printf("    %s%s%s\n", style(st), line, style(ST_RESET));
    }
    free(line);
    fclose(f);
}

/* ------------------------------------------------------------------ runs */

/* A case can go on from the compiler's output to the application's: under
 * request-response/ are runs, each the application started afresh --
 * its own database, loaded from db.sql; its own signing key, secret;
 * its own flags, args -- and sent the requests under interaction/ one
 * after the other, in the order of their names.  What is kept is every
 * response, and beside it what the request changed in the database
 * (NAME.dml.sql) and what the server printed meanwhile (NAME.stdout,
 * NAME.stderr), each only if there was anything; under out/ the same
 * for what the application did before the first request (startup.*,
 * the initialize task's statements among them) and printed after the
 * last (shutdown.*), and the database it left behind.  The server's
 * own chatter about each connection and request is dropped, as are the
 * lines of its start: what the requests made the application say is
 * what is left.
 *
 * For the same requests to get the same answers, the application's clock
 * and its random numbers are pinned (URWEB_REPRODUCIBLE_*, see
 * src/c/reproducible.c), its time zone is UTC, and curl always introduces
 * itself the same way and asks for the same host, whatever port the
 * server found free.  Its heap starts large (URWEB_MIN_HEAP): the runtime
 * runs a handler again from the start whenever the heap has had to move
 * while growing, and whether realloc moves it is the libc's business, so
 * what a handler prints with debug would come once per attempt, and the
 * count of attempts would be the allocator's.  A heap that never grows
 * is never moved.  256 MiB of it costs nothing until it is written to;
 * an application whose heap limit is below that refuses to start, and
 * the run fails with its message. */

#define RUN_TIME "946684800"    /* 2000-01-01 00:00:00 UTC, the first request; a second more for every further one */
#define RUN_SEED "0"
#define RUN_MIN_HEAP "268435456"    /* 256 MiB */

/* What urt sets in the application's environment itself, above and in
 * run_run, and a run's env therefore cannot. */
static const char *const run_env_reserved[] = {
    "URWEB_SIGFILE_PATH", "URWEB_SQLITE_DB_PATH", "URWEB_DML_LOG", "URWEB_REPRODUCIBLE_EPOCH",
    "URWEB_REPRODUCIBLE_PRNG_SEED", "URWEB_MIN_HEAP", "TZ",
};

/* Whether s can name an environment variable: letters, digits and _, not
 * starting with a digit, as the shell has it. */
static int valid_env_name(const char *s) {
    if (!*s || isdigit((unsigned char) *s)) {
        return 0;
    }
    for (; *s; s++) {
        if (!isalnum((unsigned char) *s) && *s != '_') {
            return 0;
        }
    }
    return 1;
}

/* A run's env file f, NAME=value a line, into names and values: blank
 * lines (nothing, or blanks) and lines starting with # are skipped, a line
 * may end in CRLF, and the value is the rest of the line as it stands.  A
 * value holds no control character but a tab: a lone CR would be one,
 * from a file whose lines end in CR alone, which this would otherwise
 * read as one long line.  What is wrong with it is complained about, and
 * then -1. */
static int read_run_env(const char *f, struct strlist *names, struct strlist *values) {
    char *text = read_text(f), *line, *next, *eq, *c, dir[PATHLEN];
    int lineno = 0, before = report_problems, browser;
    size_t i, n;
    if (!text) {
        die_later(fmt("%s: %s", path(f), strerror(errno)), NULL);
        return -1;
    }
    dirname_of(dir, f);
    browser = kind_of(dir) == RUN_BROWSER;
    if ((long) strlen(text) != file_size(f)) {
        die_later(fmt("%s holds a NUL byte", path(f)), "an env file is text", NULL);
        free(text);
        return -1;
    }
    for (line = text; *line; line = next) {
        lineno++;
        next = strchr(line, '\n');
        if (next) {
            *next++ = 0;
        } else {
            next = line + strlen(line);
        }
        n = strlen(line);
        if (n && line[n - 1] == '\r') {
            line[--n] = 0;
        }
        if (strspn(line, " \t") == n || line[0] == '#') {
            continue;
        }
        eq = strchr(line, '=');
        if (!eq) {
            die_later(fmt("%s:%d is not NAME=value", path(f), lineno), NULL);
            continue;
        }
        *eq = 0;
        if (!valid_env_name(line)) {
            die_later(fmt("%s:%d: %s cannot name an environment variable", path(f), lineno, key(line)),
                      "a name is letters, digits and _, not starting with a digit", NULL);
            continue;
        }
        for (c = eq + 1; *c && (*c == '\t' || ((unsigned char) *c >= 0x20 && *c != 0x7f)); c++) {
            ;
        }
        if (*c) {
            die_later(fmt("%s:%d: the value of %s holds a control character", path(f), lineno, key(line)),
                      "a line ends in LF or CRLF, and a value holds no other control character than a tab", NULL);
            continue;
        }
        for (i = 0; i < sizeof run_env_reserved / sizeof run_env_reserved[0]; i++) {
            if (!strcmp(line, run_env_reserved[i])) {
                die_later(fmt("%s:%d sets %s, which urt sets itself", path(f), lineno, key(line)),
                          "it is part of what makes a run repeat, or of what urt reads back", NULL);
                break;
            }
        }
        if (i < sizeof run_env_reserved / sizeof run_env_reserved[0]) {
            continue;
        }
        if (!strcmp(line, "URWEB_REPRODUCIBLE_TASKS") && browser) {
            die_later(fmt("%s:%d sets %s, which a browser run's context says", path(f), lineno, key(line)),
                      "the driver places the tasks due at a time before or after the requests then, as 'tasks first'",
                      "or 'tasks last' in the run's context says, and urt starts the server for that", NULL);
            continue;
        }
        if (sl_has(names, line)) {
            die_later(fmt("%s:%d sets %s a second time", path(f), lineno, key(line)), NULL);
            continue;
        }
        sl_push(names, line);
        sl_push(values, eq + 1);
    }
    free(text);
    return report_problems == before ? 0 : -1;
}

/* The runs of a case, as paths: the request-response ones sorted, then
 * the browser ones sorted. */
static void runs_of(const char *case_dir, struct strlist *runs) {
    struct strlist names = { 0, 0, 0 };
    char d[PATHLEN], f[PATHLEN];
    int i, kind;
    for (kind = RUN_REQUESTS; kind <= RUN_BROWSER; kind++) {
        join(d, case_dir, run_dirname[kind]);
        if (is_dir(d)) {
            list_dir(d, LS_DIRS, &names);
            for (i = 0; i < names.n; i++) {
                join(f, d, names.v[i]);
                sl_push(runs, f);
            }
            sl_free(&names);
        }
    }
}

/* The browser driver's path into dst, the urt-browse beside this
 * program; 0 when there is none. */
static int find_browser(char *dst) {
    char dir[PATHLEN];
    if (!program_dir(dir)) {
        return 0;
    }
    pathf(dst, "%s/urt-browse", dir);
    return can_exec(dst);
}

/* The flag in a case's args that keeps the compiler from building the
 * application its runs need, or NULL. */
static const char *no_application(const struct strlist *extra) {
    int i;
    for (i = 0; i < extra->n; i++) {
        const char *a = extra->v[i];
        if (!strcmp(a, "-stop") || !strcmp(a, "-tc")) {
            return a;
        }
    }
    return NULL;
}

/* Where the compiler says it puts the application: the Exe line of the
 * job as -saveSettings dumped it, which knows of the project's name, an
 * exe directive and -output alike.  "" if the file does not say. */
static void exe_of(const char *settings, char *exe) {
    char *text = is_file(settings) ? read_text(settings) : NULL, *line, *end;
    *exe = 0;
    for (line = text; line && *line; line = end ? end + 1 : NULL) {
        end = strchr(line, '\n');
        if (has_prefix(line, "Exe: ")) {
            snprintf(exe, PATHLEN, "%.*s", end ? (int) (end - line - 5) : (int) strlen(line + 5), line + 5);
            break;
        }
    }
    free(text);
}

/* What the server prints while it starts is no use in a snapshot, and
 * worse: it comes from two processes at once.  The server forks to go to
 * the background, the one that stays says which process the other is, the
 * other which port it listens on, both write out the banner they had
 * buffered, and the threads of the one that serves each announce their
 * database connection, whenever they get to it.  In what order all that
 * lands in the file differs from run to run, and the two numbers anyway.
 * So the lines of the start go, wherever they turn up, and with them the
 * runtime's note that realloc moved its heap, which is the allocator's
 * business.  What is left is what the requests made the server say, the
 * program's own debug output included, and that comes in order, since
 * the requests do.
 *
 * Returns 0 when the line is to be dropped.  The banner ends in a blank
 * line, which goes only there: *after_banner says that its last line has
 * just been seen.  The server's lines about each connection and request
 * ("Accepted connection.", "Serving URI ...", "Processed request for
 * messages ...") go the same way: they narrate what the runner did. */
static int keep_server_line(const char *line, int *after_banner) {
    static const char *const start[] = {
        "Database connection initialized.\n",
        "Starting the Ur/Web native HTTP server, which is intended for use\n",
        "ONLY DURING DEVELOPMENT.  You probably want to use one of the other backends,\n",
    };
    static const char banner_end[] = "behind a production-quality HTTP server, for a real deployment.\n";
    int was_after = *after_banner;
    size_t i;

    /* The .msgs line ends in a blank one as well. */
    *after_banner = !strcmp(line, banner_end) || has_prefix(line, "Processed request for messages by client ");
    if (*after_banner || (was_after && !strcmp(line, "\n"))) {
        return 0;
    }
    for (i = 0; i < sizeof start / sizeof start[0]; i++) {
        if (!strcmp(line, start[i])) {
            return 0;
        }
    }
    return !has_prefix(line, "Forking to PID ") && !has_prefix(line, "Listening on port ")
        && !has_prefix(line, "Error triggers unlimited retry: Couldn't allocate new ")
        && strcmp(line, "Accepted connection.\n") != 0 && !has_prefix(line, "Handling connection with thread #")
        && !has_prefix(line, "Serving URI ");
}

/* A line of n bytes of an output of a run as it is recorded: rewritten in
 * place, with the scratch directory taken out of it as for the artifacts,
 * and on stdout dropped if it is one of the server's own (keep_server_line,
 * with *after_banner, which starts at 0 for each file).  One that holds a
 * NUL is left as it is.  Returns its length now, or -1 if it is dropped.
 * A wait looks at lines through this too, so that it is satisfied only by
 * what the recording shows. */
static ssize_t recorded_line(char *line, ssize_t n, int is_stdout, int *after_banner) {
    if (memchr(line, 0, (size_t) n)) {
        return n;
    }
    strip_work(line);
    if (is_stdout && !keep_server_line(line, after_banner)) {
        return -1;
    }
    return (ssize_t) strlen(line);
}

/* One output of a run, from -> to, its lines as recorded_line has them.
 * A response can be anything, so lines are written by their length.
 * Returns how many bytes were written. */
static long normalise_run(const char *name, const char *from, const char *to) {
    FILE *in, *out;
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    long written = 0;
    int is_stdout = has_suffix(name, ".stdout"), after_banner = 0;
    in = fopen(from, "rb");
    out = fopen(to, "wb");
    if (!in || !out) {
        die(fmt("cannot normalise %s: %s", path(from), strerror(errno)), NULL);
    }
    while ((n = getline(&line, &cap, in)) > 0) {
        if ((n = recorded_line(line, n, is_stdout, &after_banner)) >= 0) {
            fwrite(line, 1, (size_t) n, out);
            written += (long) n;
        }
    }
    free(line);
    fclose(in);
    if (fclose(out) != 0) {
        die(fmt("cannot write %s: %s", path(to), strerror(errno)), NULL);
    }
    return written;
}

/* What a stream of the server's holds since the last cut, up to byte end
 * or, if end is negative, to where the file ends now, into a file of its
 * own; nothing, and there is no file.  *at is where the last cut was, and
 * is moved to where this one is.  The server writes the stream itself, so
 * this reads what is there rather than what it was handed, and no further
 * than end, though the file may have grown since. */
static void cut_output(const char *stream, long *at, long end, const char *to) {
    FILE *in = fopen(stream, "rb"), *out;
    char buf[8192];
    size_t n;
    long size = 0;

    if (!in) {
        die(fmt("cannot read %s: %s", path(stream), strerror(errno)), NULL);
    }
    if (fseek(in, 0, SEEK_END) < 0 || (size = ftell(in)) < 0) {
        die(fmt("cannot read %s: %s", path(stream), strerror(errno)), NULL);
    }
    if (end < 0 || end > size) {
        end = size;
    }
    if (end <= *at) {
        fclose(in);
        return;
    }
    if (fseek(in, *at, SEEK_SET) < 0 || !(out = fopen(to, "wb"))) {
        die(fmt("cannot write %s: %s", path(to), strerror(errno)), NULL);
    }
    while (*at < end && (n = fread(buf, 1, end - *at < (long) sizeof buf ? (size_t) (end - *at) : sizeof buf, in)) > 0) {
        fwrite(buf, 1, n, out);
        *at += (long) n;
    }
    fclose(in);
    if (fclose(out) != 0) {
        die(fmt("cannot write %s: %s", path(to), strerror(errno)), NULL);
    }
}

/* One output of a run, compared or recorded.  rel names it under the
 * run's directory; now is the file the run produced, which may not be
 * there (a request that changed nothing has no NAME.sql).  Its name goes
 * on the list it belongs on, and a diff on the pile to show. */
static void settle(const char *run_dir, const char *rel, const char *now, char *bad, char *changed, size_t size,
                   struct strlist *diffs) {
    char want[PATHLEN], clean[PATHLEN], d[PATHLEN];
    join(want, run_dir, rel);
    pathf(clean, "%s.now", now);
    if (exists(now) && normalise_run(rel, now, clean) == 0
        && (has_suffix(rel, ".stdout") || has_suffix(rel, ".stderr"))) {
        /* The server printed nothing for this, or nothing worth keeping. */
        unlink(clean);
    }
    if (opt_check) {
        pathf(d, "%s.diff", now);
        if (diff_into(want, clean, d) != 0) {
            list_add(bad, size, rel);
            sl_push(diffs, d);
        }
    } else if (!exists(clean)) {
        if (exists(want)) {
            if (unlink(want) < 0) {
                die(fmt("cannot delete %s: %s", path(want), strerror(errno)), NULL);
            }
            list_add(changed, size, fmt("-%s", rel));
        }
    } else if (!same_contents(want, clean)) {
        dirname_of(d, want);
        if (mkdir_p(d) < 0 || copy_file(clean, want) < 0) {
            die(fmt("cannot write %s: %s", path(want), strerror(errno)), NULL);
        }
        list_add(changed, size, rel);
    }
}

/* The first lines of a file, for an error message; or nothing if there
 * is nothing in it, as the message's explanation lines end there. */
static const char *head_of(const char *file, const char *if_nothing) {
    char *text = is_file(file) ? read_text(file) : NULL;
    const char *r;
    size_t n;
    if (!text) {
        return if_nothing;
    }
    n = strlen(text);
    while (n && text[n - 1] == '\n') {
        text[--n] = 0;
    }
    r = *text ? fmt("%.600s", text) : if_nothing;
    free(text);
    return r;
}

/* What a status of curl's means when it is one that says the request
 * never reached a server, and so is nothing to record; NULL for the
 * others, where what curl said is the record of what the server did. */
static const char *never_sent(int status) {
    switch (status) {
    case 2: return "curl refused its command line";
    case 6: return "curl could not resolve the host";
    case 7: return "curl could not connect";
    case 28: return "curl waited 30 seconds for the whole answer and gave up";
    case 127: return "curl could not be run";
    default: return NULL;
    }
}

/* Send one request, the response into a file, the request saying it is
 * made at time_ms on the server's clock (milliseconds since the Unix
 * epoch, in the UrWeb-Time header).  Returns curl's status. */
static int send_request(const char *method, const char *url_path, const struct strlist *options, int port,
                        const char *response, long long time_ms) {
    struct strlist argv = { 0, 0, 0 };
    char err[PATHLEN], **command, *cookies = NULL;
    int i, status;

    sl_push(&argv, "curl");
    sl_push(&argv, "-q");               /* no ~/.curlrc; has to come first */
    sl_push(&argv, "-sS");
    sl_push(&argv, "-i");
    sl_push(&argv, "-g");               /* [] and {} in a path are just that */
    sl_push(&argv, "--path-as-is");     /* and so are . and .. */
    sl_push(&argv, "--noproxy");
    sl_push(&argv, "*");
    sl_push(&argv, "--max-time");
    sl_push(&argv, "30");
    sl_push(&argv, "-A");
    sl_push(&argv, "urt");
    sl_push(&argv, "-H");
    sl_push(&argv, "Host: localhost");
    sl_push(&argv, "-H");
    sl_push(&argv, fmt("UrWeb-Time: %lld", time_ms));
    sl_push(&argv, "-X");
    sl_push(&argv, method);
    /* Several cookies go as one -b, joined here: curl joins them itself
     * otherwise, with "; " or with ";" by its version, and the bytes
     * sent are what the responses answer to. */
    for (i = 0; i + 1 < options->n; i += 2) {
        if (!strcmp(options->v[i], "-b")) {
            const char *v = options->v[i + 1];
            if (cookies) {
                char *joined = xmalloc(strlen(cookies) + strlen(v) + 3);
                sprintf(joined, "%s; %s", cookies, v);
                free(cookies);
                cookies = joined;
            } else {
                cookies = xstrdup(v);
            }
        } else {
            sl_push(&argv, options->v[i]);
            sl_push(&argv, options->v[i + 1]);
        }
    }
    if (cookies) {
        sl_push(&argv, "-b");
        sl_push(&argv, cookies);
        free(cookies);
    }
    sl_push(&argv, fmt("http://127.0.0.1:%d%s", port, url_path));

    pathf(err, "%s.curl", response);
    command = sl_argv(&argv);
    status = run(command, response, err);
    free(command);
    sl_free(&argv);

    if (status != 0) {
        /* No response, or half of one: what curl said goes where it would
         * have been, less the port, which differs from run to run. */
        char *said = read_text(err), *hit;
        const char *port_s = fmt("%d", port);
        FILE *f = fopen(response, "ab");
        if (f) {
            for (hit = said; said && (hit = strstr(hit, port_s)); hit += strlen(port_s)) {
                memset(hit, '#', strlen(port_s));
            }
            fprintf(f, "%s", said && *said ? said : fmt("curl: exit %d\n", status));
            fclose(f);
        }
        free(said);
    }
    return status;
}

/* Whether the word at *i of a server's args is, or holds, -t, read as
 * the server's getopt reads it: the letters of a word after one - are
 * options, and one that takes a value (the server's are p P a A u t T
 * m) takes the rest of the word, or the next word if there is none
 * left, and ends the word; -d takes the rest of the word if any, and
 * never the next.  *i is moved past a value taken from the next word. */
static int asks_for_threads(const struct strlist *words, int *i) {
    const char *w = words->v[*i], *c;
    if (w[0] != '-' || w[1] == '-' || !w[1]) {
        return 0;
    }
    for (c = w + 1; *c; c++) {
        if (*c == 't') {
            return 1;
        }
        if (*c == 'd') {
            return 0;
        }
        if (strchr("pPaAuTm", *c)) {
            if (!c[1] && *i + 1 < words->n) {
                (*i)++;
            }
            return 0;
        }
    }
    return 0;
}

/* The inputs under interaction/, by name (NAME of NAME.request, or of
 * NAME.wait, or of NAME.step), in the order of their names: that of the
 * bytes, as the help text says, which is not quite the order of the file
 * names, since - sorts before the . of the suffix.  The driver takes the
 * steps in the same order.  A name that is a request and a wait comes
 * twice, for run_ready to refuse. */
static void inputs_of(const char *interaction, enum run_kind kind, struct strlist *names) {
    struct strlist files = { 0, 0, 0 };
    char name[PATHLEN];
    int i;
    if (is_dir(interaction)) {
        list_dir(interaction, LS_FILES, &files);
    }
    for (i = 0; i < files.n; i++) {
        const char *suffix = input_suffix(files.v[i], kind);
        if (suffix) {
            snprintf(name, sizeof name, "%s", files.v[i]);
            strip_suffix(name, suffix);
            sl_push(names, name);
        }
    }
    sl_free(&files);
    sl_sort(names);
}

#define UNTIL_DAYS 365      /* how far an until goes on the server's clock */

/* A wait file's text: one line, `wait T` or `until TEXT`.  For the one,
 * T in milliseconds into *ms and what empty; for the other, *ms -1 and
 * TEXT into what, without the blanks around it, which cannot be seen and
 * would make it match nearly any line, and not empty.  0, or -1 with the
 * complaint in err. */
static int parse_wait(const char *text, long long *ms, char *what, size_t what_size, char *err, size_t err_size) {
    size_t n = strlen(text), s, e;
    if (n && text[n - 1] == '\n') {
        n--;
    }
    if (n && text[n - 1] == '\r') {
        n--;
    }
    if (memchr(text, '\n', n) || memchr(text, '\r', n)) {
        snprintf(err, err_size, "a wait is one line");
        return -1;
    }
    *what = 0;
    *ms = -1;
    if (n >= 5 && (!strncmp(text, "wait ", 5) || !strncmp(text, "wait\t", 5))) {
        char t[64];
        for (s = 5; s < n && (text[s] == ' ' || text[s] == '\t'); s++) {
            ;
        }
        for (e = n; e > s && (text[e - 1] == ' ' || text[e - 1] == '\t'); e--) {
            ;
        }
        snprintf(t, sizeof t, "%.*s", (int) (e - s), text + s);
        if (s == e || e - s >= sizeof t || parse_duration(t, ms) < 0) {
            snprintf(err, err_size, "'%.*s' is not a time: a number and its unit, 500ms, 2s, 3min, 1h, 30d", (int) (e - s), text + s);
            return -1;
        }
        return 0;
    }
    for (s = 6; s < n && (text[s] == ' ' || text[s] == '\t'); s++) {
        ;
    }
    for (e = n; e > s && (text[e - 1] == ' ' || text[e - 1] == '\t'); e--) {
        ;
    }
    if (n < 6 || (strncmp(text, "until ", 6) != 0 && strncmp(text, "until\t", 6) != 0) || s == e) {
        snprintf(err, err_size, "a wait is 'wait T', T a time, or 'until TEXT', TEXT being what the server is to print");
        return -1;
    }
    if (e - s >= what_size) {
        snprintf(err, err_size, "the text to wait for is too long");
        return -1;
    }
    memcpy(what, text + s, e - s);
    what[e - s] = 0;
    return 0;
}

/* Where a search of one of the server's streams has got to: the byte
 * after the last whole line looked at, and recorded_line's *after_banner
 * there. */
struct scan { long at; int after_banner; };

/* Whether a whole line from where sc stands on in stream holds what, as
 * it is recorded (recorded_line, NUL bytes and all), sc moving on over
 * the lines looked at, so that the next search starts after them.
 * is_stdout says which stream it is, for the lines that are dropped. */
static int recorded_line_with(const char *stream, struct scan *sc, int is_stdout, const char *what) {
    FILE *in = fopen(stream, "rb");
    char *line = NULL;
    size_t cap = 0, w = strlen(what), i;
    ssize_t n;
    int found = 0;
    if (!in) {
        return 0;
    }
    if (fseek(in, sc->at, SEEK_SET) == 0) {
        while (!found && (n = getline(&line, &cap, in)) > 0 && line[n - 1] == '\n') {
            sc->at += (long) n;
            n = recorded_line(line, n, is_stdout, &sc->after_banner);
            for (i = 0; n >= 0 && i + w <= (size_t) n && !found; i++) {
                found = !memcmp(line + i, what, w);
            }
        }
    }
    free(line);
    fclose(in);
    return found;
}

/* A wait, on the server's clock, which stands at *clock (milliseconds):
 * for ms milliseconds, or, if ms is -1, until the server prints a line
 * holding what, as recorded, on either stream, after where the last
 * step's output was cut (out_at, err_at).  The clock is moved from one
 * thing the server is due to do by itself to the next, a tag at a time,
 * so that it does nothing past where the wait ends and every call is
 * short.  0 with *clock where the next step starts: the wait's end, or a
 * second after the tag that printed the line, as after a request; or -1
 * with why the wait cannot be done in why (why_size). */
static int wait_on_clock(const struct server *s, long long *clock, long long ms, const char *what,
                         const char *out, long out_at, const char *err, long err_at, char *why, size_t why_size) {
    long long next, tag, end = ms >= 0 ? *clock + ms : *clock + UNTIL_DAYS * 86400000LL;
    struct scan out_scan = { out_at, 0 }, err_scan = { err_at, 0 };
    int r = server_clock(s, -1, &next, why, why_size);
    for (;;) {
        if (r < 0) {
            return -1;
        }
        if (fs_interrupted) {
            snprintf(why, why_size, "interrupted");
            return -1;
        }
        if (r == 0 || next >= end) {
            if (ms >= 0) {
                *clock = end;
                return 0;
            }
            if (r == 0) {
                snprintf(why, why_size, "the server has nothing more to do by itself, and printed no line holding '%.200s'", what);
            } else {
                snprintf(why, why_size, "the server printed no line holding '%.200s' within %d days on its clock", what, UNTIL_DAYS);
            }
            return -1;
        }
        /* Everything due at that tag, and nothing after it. */
        tag = next;
        r = server_clock(s, tag + 1, &next, why, why_size);
        if (r >= 0 && ms < 0
            && (recorded_line_with(out, &out_scan, 1, what) || recorded_line_with(err, &err_scan, 0, what))) {
            if (*clock < tag + 1000) {
                *clock = tag + 1000;
            }
            return 0;
        }
    }
}

/* The tabs a step has records for under dir: where the driver wrote
 * this run's (rw/interaction), or the run's own interaction/, where the
 * earlier records are.  Labels not there yet go on labels. */
static void tab_labels(const char *dir, const char *step_name, struct strlist *labels) {
    struct strlist files = { 0, 0, 0 };
    char step[PATHLEN], label[PATHLEN], why[512];
    int i;
    if (is_dir(dir)) {
        list_dir(dir, LS_FILES, &files);
    }
    for (i = 0; i < files.n; i++) {
        if (has_prefix(files.v[i], fmt("%s.", step_name))
            && parse_record(files.v[i], RUN_BROWSER, step, label, NULL, why, sizeof why)
            && !strcmp(step, step_name) && *label && !sl_has(labels, label)) {
            sl_push(labels, label);
        }
    }
    sl_free(&files);
}

/* What a request or a step produces, onto outputs, as relative paths:
 * its own records, every kind a run of this kind has that is not a
 * tab's; and in a browser run a set per tab, for every tab the step has
 * records for now (under rw) or had before (under the run), so that a
 * tab's records go when it does. */
static void step_outputs(const char *run_dir, const char *rw, enum run_kind kind, const char *name, struct strlist *outputs) {
    struct strlist labels = { 0, 0, 0 };
    char rec[PATHLEN];
    int i, j;
    for (i = 0; record_kinds[i].kind; i++) {
        if (!record_kinds[i].tab && (record_kinds[i].only == -1 || record_kinds[i].only == (int) kind)
            && strcmp(record_kinds[i].kind, "db")) {
            record_name(rec, name, "", &record_kinds[i], "");
            sl_push(outputs, fmt("interaction/%s", rec));
        }
    }
    if (kind != RUN_BROWSER) {
        return;
    }
    tab_labels(fmt("%s/interaction", rw), name, &labels);
    tab_labels(fmt("%s/interaction", run_dir), name, &labels);
    sl_sort(&labels);
    for (i = 0; i < labels.n; i++) {
        for (j = 0; record_kinds[j].kind; j++) {
            if (record_kinds[j].tab) {
                record_name(rec, name, labels.v[i], &record_kinds[j], "");
                sl_push(outputs, fmt("interaction/%s", rec));
            }
        }
    }
    sl_free(&labels);
}

/* A run that stops at an input -- a request that cannot be sent, a step
 * that cannot be done -- records nothing for those after it, and what
 * they recorded before goes: their outputs are put on the list, with
 * nothing produced for them. */
static void stale_after(const char *run_dir, const char *rw, enum run_kind kind, const char *name, struct strlist *outputs) {
    struct strlist names = { 0, 0, 0 };
    int i;
    inputs_of(fmt("%s/interaction", run_dir), kind, &names);
    for (i = 0; i < names.n; i++) {
        if (strcmp(names.v[i], name) > 0) {
            step_outputs(run_dir, rw, kind, names.v[i], outputs);
        }
    }
    sl_free(&names);
}

/* Whether a run has what it takes -- a secret, args that leave the
 * threads alone, and requests that can be read -- which is looked into
 * before the compiler is kept busy. */
static int run_ready(const char *run_dir) {
    char f[PATHLEN], interaction[PATHLEN], *args;
    struct strlist files = { 0, 0, 0 };
    int i, before = report_problems;

    join(f, run_dir, "secret");
    if (!is_file(f)) {
        die_later(fmt("%s has no secret", test(run_dir)),
                  "the key the application signs with: any 16 bytes or more, such as the line 'secret signature'", NULL);
    }
    /* More threads serve nothing here, where requests are sent one at a
     * time; all they do is make it a matter of chance which of them
     * answers, and with it what the server prints. */
    join(f, run_dir, "args");
    if (is_file(f) && (args = read_text(f))) {
        struct strlist words = { 0, 0, 0 };
        split_words(args, &words);
        for (i = 0; i < words.n; i++) {
            if (!strcmp(words.v[i], "--")) {
                /* After it the server takes no options, ours included. */
                die_later(fmt("%s ends the options with %s", path(f), key("--")),
                          "the flags urt adds after a run's args -- the address, the port range and -d3, for the",
                          "handshake -- would be ignored, and the server would listen where it likes and never say so",
                          fmt("take %s out", key("--")), NULL);
            } else if (asks_for_threads(&words, &i)) {
                die_later(fmt("%s asks for threads, with %s", path(f), key(words.v[i])),
                          "a run's requests are sent one after the other, so one thread serves them all;",
                          "with more, which one answers is a matter of chance, and the server's output with it",
                          fmt("take %s out", key("-t")), NULL);
            }
        }
        sl_free(&words);
        free(args);
    }

    join(f, run_dir, "env");
    if (is_file(f)) {
        struct strlist names = { 0, 0, 0 }, values = { 0, 0, 0 };
        read_run_env(f, &names, &values);
        sl_free(&names);
        sl_free(&values);
    }

    join(interaction, run_dir, "interaction");
    if (kind_of(run_dir) == RUN_BROWSER) {
        /* The driver reads the steps and the users' contexts, and says
         * what is wrong with them a line at a time, as NAME.step:LINE:
         * ..., or else the run's passes; python3 is all it needs for
         * that. */
        char browser[PATHLEN], said[PATHLEN], *text, *line, *end;
        char *argv[4];
        int status;
        if (!find_browser(browser)) {
            die_later(fmt("%s is a browser run, and there is no %s beside %s", test(run_dir), path("urt-browse"), path("urt")),
                      "it drives the browser; it is a script in the urt directory, installed with urt", NULL);
            return 0;
        }
        argv[0] = browser;
        argv[1] = "--check";
        argv[2] = (char *) run_dir;
        argv[3] = NULL;
        join(said, work, "check.txt");   /* run_case has made work by now */
        status = run(argv, said, said);
        bail_if_interrupted();
        if (status == 127) {
            die_later(fmt("%s cannot run", path(browser)),
                      "it is a python3 script; is python3 on the PATH?", NULL);
        } else if (status != 0) {
            text = read_text(said);
            die_later(fmt("%s has steps, or contexts, that cannot be read", test(run_dir)), NULL);
            for (line = text; line && *line; line = end ? end + 1 : NULL) {
                end = strchr(line, '\n');
                if (end) {
                    *end = 0;
                }
                report_err("    %s\n", info(line));
            }
            free(text);
        }
        return report_problems == before;
    }
    inputs_of(interaction, RUN_REQUESTS, &files);
    for (i = 0; i < files.n; i++) {
        char method[32], url_path[PATHLEN], err[256], what[1024], *text;
        struct strlist options = { 0, 0, 0 };
        long long ms;
        int is_wait;
        pathf(f, "%s/%s.wait", interaction, files.v[i]);
        is_wait = is_file(f);
        if (i + 1 < files.n && !strcmp(files.v[i], files.v[i + 1])) {
            die_later(fmt("%s is a request and a wait", path(fmt("%s/%s", interaction, files.v[i]))),
                      "a step is the one or the other; what it leads to is filed under its name", NULL);
            i++;
            continue;
        }
        if (!is_wait) {
            pathf(f, "%s/%s.request", interaction, files.v[i]);
        }
        if (!*files.v[i]) {
            die_later(fmt("%s has no name", path(f)),
                      is_wait ? "a wait is NAME.wait, and NAME is what the server printed meanwhile is filed under"
                              : "a request is NAME.request, and NAME is what its answer is filed under", NULL);
            continue;
        }
        text = read_text(f);
        if (!text) {
            die_later(fmt("%s: %s", path(f), strerror(errno)), NULL);
        } else if ((long) strlen(text) != file_size(f)) {
            /* Read as a string, it would end at the NUL and not say so. */
            die_later(fmt("%s holds a NUL byte", path(f)), is_wait ? "a wait is text" : "a request is text", NULL);
        } else if (is_wait ? parse_wait(text, &ms, what, sizeof what, err, sizeof err) < 0
                           : parse_request(text, method, sizeof method, url_path, sizeof url_path, &options, err, sizeof err) < 0) {
            die_later(fmt("%s: %s", path(f), err), NULL);
        }
        free(text);
        sl_free(&options);
    }
    sl_free(&files);
    return report_problems == before;
}

/* What a browser run's driver reports a line at a time, and what is done
 * with it: after every step (`step NAME`), what the server did meanwhile
 * is cut out beside the step's records, as it is after every request;
 * `fail NAME` is a step that could not be done, whose complaint the
 * driver left in NAME.error with the records of the page as it found it,
 * and the run stops there. */
struct browsing {
    const char *run_dir, *rw, *dml, *server_out, *server_err;
    long *out_at, *err_at;
    struct strlist *outputs;
    int steps;              /* how many the driver reported done, or failed */
    char stopped[PATHLEN];  /* the step that could not be done, if one */
};

static void on_browse_line(const char *line, void *data) {
    struct browsing *b = data;
    const char *name;
    int fail = has_prefix(line, "fail ");

    if (!fail && !has_prefix(line, "step ")) {
        return;
    }
    name = line + 5;
    b->steps++;
    if (exists(b->dml) && rename(b->dml, fmt("%s/interaction/%s.dml.sql", b->rw, name)) < 0) {
        die(fmt("cannot move %s: %s", path(b->dml), strerror(errno)), NULL);
    }
    cut_output(b->server_out, b->out_at, -1, fmt("%s/interaction/%s.stdout", b->rw, name));
    cut_output(b->server_err, b->err_at, -1, fmt("%s/interaction/%s.stderr", b->rw, name));
    step_outputs(b->run_dir, b->rw, RUN_BROWSER, name, b->outputs);
    if (fail) {
        snprintf(b->stopped, sizeof b->stopped, "%s", name);
    }
}

/* Under tmp rather than work, which goes with every case. */
static const char *driver_err_path(int slot) {
    return fmt("%s/browse-%d.stderr", tmp, slot);
}

/* The driver in a slot, started if it is not running: urt-browse --serve,
 * which keeps the engines it starts and does a pass of a run per line.
 * NULL, with the problem reported, if it cannot be started. */
static struct child *driver_for(int slot) {
    char browser[PATHLEN];
    char *argv[6];
    int n = 0;
    long size;
    if (driver_up[slot]) {
        return &drivers[slot];
    }
    find_browser(browser);                  /* run_ready has seen it */
    argv[n++] = browser;
    argv[n++] = "--serve";
    argv[n++] = "--epoch";
    argv[n++] = RUN_TIME;
    argv[n] = NULL;
    if (child_start(argv, driver_err_path(slot), &drivers[slot]) < 0) {
        die_later(fmt("cannot start %s: %s", path("urt-browse"), strerror(errno)), NULL);
        return NULL;
    }
    driver_up[slot] = 1;
    /* Its stderr goes on where the one before it, if any, left off. */
    size = file_size(driver_err_path(slot));
    driver_err_at[slot] = size > 0 ? size : 0;
    return &drivers[slot];
}

/* s quoted for a shell, and for the driver, which splits a line as one. */
static const char *shell_quoted(const char *s) {
    char *b = xmalloc(strlen(s) * 4 + 3), *q = b;
    const char *r;
    *q++ = '\'';
    for (r = s; *r; r++) {
        if (*r == '\'') {
            memcpy(q, "'\\''", 4);
            q += 4;
        } else {
            *q++ = *r;
        }
    }
    *q++ = '\'';
    *q = 0;
    r = fmt("%s", b);
    free(b);
    return r;
}

/* One start of the application, and where what came of it goes. */
struct app {
    const char *rw;
    char db[PATHLEN], dml[PATHLEN], server_out[PATHLEN], server_err[PATHLEN];
    long out_at, err_at;
    int has_db;
    struct server *server;
};

/* The application exe started into rw, as server: its own database,
 * loaded from the run's db.sql, its secret and args, its environment;
 * what it did before the first request cut out under rw/out.  browser
 * says whether it is for a browser run, whose driver places the server's
 * tasks among the requests.  What came of it lands under rw as it is laid
 * out under the run, and goes on outputs, as relative paths.  0, or -1
 * with the problem reported. */
static int app_start(const char *run_dir, const char *exe, int browser, const char *rw, struct server *server,
                     struct app *a, struct strlist *outputs) {
    char f[PATHLEN], g[PATHLEN], why[256];
    struct strlist args = { 0, 0, 0 };
    struct strlist env_names = { 0, 0, 0 }, env_values = { 0, 0, 0 };
    char *text, **env_saved;
    int i, started;

    a->rw = rw;
    a->out_at = a->err_at = 0;
    a->server = server;
    rm_rf(rw);
    if (mkdir_p(fmt("%s/interaction", rw)) < 0 || mkdir_p(fmt("%s/out", rw)) < 0) {
        die(fmt("cannot make %s", path(rw)), NULL);
    }

    /* What the application is started with; run_ready has seen to it
     * that it is there. */
    join(f, run_dir, "secret");
    join(g, rw, "secret");
    if (copy_file(f, g) < 0) {
        die(fmt("cannot write %s: %s", path(g), strerror(errno)), NULL);
    }
    setenv("URWEB_SIGFILE_PATH", g, 1);

    join(f, run_dir, "db.sql");
    join(a->db, rw, "db");
    a->has_db = is_file(f);
    if (a->has_db) {
        /* -safe: a seed is SQL, and runs no dot-command that reaches
         * outside the database (.shell, .output, .read) and no ATTACH. */
        static char *load[] = { "sqlite3", "-safe", "-batch", "-bail", NULL, NULL };
        load[4] = a->db;
        if (run_in(load, f, "/dev/null", fmt("%s/sqlite.err", rw)) != 0) {
            bail_if_interrupted();
            die_later(fmt("%s does not load", path(f)), head_of(fmt("%s/sqlite.err", rw), "(sqlite3 said nothing)"), NULL);
            return -1;
        }
    }
    /* Also without a db.sql, or the application would go for the database
     * named when it was compiled; and it wants a file to be there, if only
     * an empty one, which to SQLite is a database with nothing in it. */
    if (!a->has_db && write_text(a->db, "") < 0) {
        die(fmt("cannot write %s: %s", path(a->db), strerror(errno)), NULL);
    }
    setenv("URWEB_SQLITE_DB_PATH", a->db, 1);

    join(a->dml, rw, "dml.sql");
    setenv("URWEB_DML_LOG", a->dml, 1);
    setenv("URWEB_REPRODUCIBLE_EPOCH", RUN_TIME, 1);
    setenv("URWEB_REPRODUCIBLE_PRNG_SEED", RUN_SEED, 1);
    setenv("URWEB_MIN_HEAP", RUN_MIN_HEAP, 1);
    setenv("TZ", "UTC", 1);
    /* In a browser run the driver places the tasks due at a time among the
     * requests arriving then, by its context's tie rule: the server runs
     * none of them on its own when a request comes. */
    if (browser) {
        setenv("URWEB_REPRODUCIBLE_TASKS", "last", 1);
    } else {
        unsetenv("URWEB_REPRODUCIBLE_TASKS");
    }

    join(f, run_dir, "args");
    text = is_file(f) ? read_text(f) : NULL;
    if (text) {
        split_words(text, &args);
    }
    free(text);

    /* The run's own environment, for the server only: what was there
     * before is put back once it has started, for the next run. */
    join(f, run_dir, "env");
    if (is_file(f)) {
        read_run_env(f, &env_names, &env_values);  /* run_ready has seen that it reads */
    }
    env_saved = xmalloc(sizeof *env_saved * (size_t) (env_names.n + 1));
    for (i = 0; i < env_names.n; i++) {
        const char *old = getenv(env_names.v[i]);
        env_saved[i] = old ? xstrdup(old) : NULL;
        setenv(env_names.v[i], env_values.v[i], 1);
    }

    join(a->server_out, rw, "server.stdout");
    join(a->server_err, rw, "server.stderr");
    started = server_start(exe, &args, a->server_out, a->server_err, server, why, sizeof why);
    for (i = 0; i < env_names.n; i++) {
        if (env_saved[i]) {
            setenv(env_names.v[i], env_saved[i], 1);
            free(env_saved[i]);
        } else {
            unsetenv(env_names.v[i]);
        }
    }
    free(env_saved);
    sl_free(&env_names);
    sl_free(&env_values);
    sl_free(&args);
    if (started < 0) {
        bail_if_interrupted();
        die_later(fmt("the application does not start for %s: %s", test(run_dir), why), head_of(a->server_err, NULL), NULL);
        return -1;
    }

    /* What the application did before the first request: the initialize
     * task's statements, and what it printed while starting.  Nothing else
     * happens before then: the server's clock stands still until a step
     * moves it. */
    if (exists(a->dml) && rename(a->dml, fmt("%s/out/startup.dml.sql", rw)) < 0) {
        die(fmt("cannot move %s: %s", path(a->dml), strerror(errno)), NULL);
    }
    cut_output(a->server_out, &a->out_at, -1, fmt("%s/out/startup.stdout", rw));
    cut_output(a->server_err, &a->err_at, -1, fmt("%s/out/startup.stderr", rw));
    sl_push(outputs, "out/startup.dml.sql");
    sl_push(outputs, "out/startup.stdout");
    sl_push(outputs, "out/startup.stderr");
    return 0;
}

/* What the application printed last, once stopped, and the database it
 * left behind, dumped: 0, or -1 with the problem reported, and nothing
 * of the run is to be recorded then. */
static int app_end(const char *run_dir, struct app *a, struct strlist *outputs) {
    cut_output(a->server_out, &a->out_at, -1, fmt("%s/out/shutdown.stdout", a->rw));
    cut_output(a->server_err, &a->err_at, -1, fmt("%s/out/shutdown.stderr", a->rw));
    sl_push(outputs, "out/shutdown.stdout");
    sl_push(outputs, "out/shutdown.stderr");
    if (a->has_db) {
        static char *dump[] = { "sqlite3", "-safe", "-batch", NULL, ".dump", NULL };
        dump[3] = a->db;
        if (run(dump, fmt("%s/out/shutdown.db.sql", a->rw), fmt("%s/sqlite.err", a->rw)) != 0) {
            /* The run failed, then, and nothing of it is recorded. */
            die_later(fmt("cannot dump the database of %s", test(run_dir)),
                      head_of(fmt("%s/sqlite.err", a->rw), "(sqlite3 said nothing)"), NULL);
            return -1;
        }
    }
    sl_push(outputs, "out/shutdown.db.sql");
    return 0;
}

/* A request-response run: the application started once into rw, sent the
 * requests, stopped, and its database dumped.  What came of it goes on
 * outputs; what curl failed on goes on failed.  Returns 0, or -1 with the
 * problem reported, and nothing of the run is to be recorded then.  A
 * request that cannot be sent is not that kind of problem: the run stops
 * there, with NAME.error saying why, and is recorded as far as it got;
 * stopped (PATHLEN) names the input it stopped at, or is empty. */
static int run_app(const char *run_dir, const char *exe, const char *rw,
                   struct strlist *outputs, char *failed, size_t failed_size, char *stopped) {
    char f[PATHLEN], g[PATHLEN], interaction[PATHLEN], why[256];
    long long clock_ms = atoll(RUN_TIME) * 1000;    /* the server's clock, when the next step starts */
    struct strlist files = { 0, 0, 0 };
    struct app a;
    char *text;
    int i, before = report_problems;

    *stopped = 0;
    if (app_start(run_dir, exe, 0, rw, &servers[0], &a, outputs) < 0) {
        return -1;
    }
    join(interaction, run_dir, "interaction");
    inputs_of(interaction, RUN_REQUESTS, &files);
    /* The steps, requests and waits, in the order of their names, on the
     * server's clock: a request is served at the clock, which then moves
     * on by a second; a wait moves it on by its time, or until what it
     * waits for.  One that cannot be read ends the run: those after it
     * count on what it would have done.  One that cannot be done stops
     * it, NAME.error saying why, and the run is recorded as far as it
     * got. */
    for (i = 0; i < files.n && report_problems == before; i++) {
        char method[32], url_path[PATHLEN], err[256], what[1024], cannot[512] = "";
        struct strlist options = { 0, 0, 0 };
        const char *name = files.v[i];
        long long ms;
        int is_wait;
        pathf(f, "%s/%s.wait", interaction, name);
        is_wait = is_file(f);
        if (!is_wait) {
            pathf(f, "%s/%s.request", interaction, name);
        }
        text = read_text(f);
        if (!text || (is_wait ? parse_wait(text, &ms, what, sizeof what, err, sizeof err)
                              : parse_request(text, method, sizeof method, url_path, sizeof url_path, &options, err, sizeof err)) < 0) {
            die_later(fmt("%s: %s", path(f), text ? err : strerror(errno)), NULL);
        } else if (is_wait) {
            /* What the server does by itself meanwhile: its tasks, its
             * clients' expiries.  Each is over when the clock is moved
             * past it, so all of what they print and change is there by
             * the time the wait is done. */
            wait_on_clock(a.server, &clock_ms, ms, what, a.server_out, a.out_at, a.server_err, a.err_at, cannot,
                          sizeof cannot);
        } else {
            int status = send_request(method, url_path, &options, a.server->port,
                                      fmt("%s/interaction/%s.response", rw, name), clock_ms);
            if (!never_sent(status)) {
                /* What else is due at the request's tag -- the tasks, when
                 * they come after the requests (URWEB_REPRODUCIBLE_TASKS)
                 * -- is part of this step.  A server that cannot be asked
                 * any more is the next step's to report: this one has its
                 * answer. */
                long long next;
                server_clock(a.server, clock_ms + 1, &next, why, sizeof why);
            }
            clock_ms += 1000;
            if (never_sent(status)) {
                /* What curl said is not a response (and says how long it
                 * waited). */
                snprintf(cannot, sizeof cannot, "%s (exit %d)", never_sent(status), status);
                pathf(g, "%s/interaction/%s.response", rw, name);
                if (unlink(g) < 0 && errno != ENOENT) {
                    die(fmt("cannot delete %s: %s", path(g), strerror(errno)), NULL);
                }
            } else if (status != 0) {
                list_add(failed, failed_size, name);
            }
            sl_push(outputs, fmt("interaction/%s.response", name));
        }
        if (text && report_problems == before) {
            if (*cannot) {
                /* The run stops here: the steps after this one count on
                 * it.  NAME.error says why, beside whatever the server
                 * printed meanwhile. */
                pathf(g, "%s/interaction/%s.error", rw, name);
                if (write_text(g, fmt("%s\n", cannot)) < 0) {
                    die(fmt("cannot write %s: %s", path(g), strerror(errno)), NULL);
                }
                strcpy(stopped, name);
            }
            sl_push(outputs, fmt("interaction/%s.error", name));
            /* The server has written what this step changed, if anything,
             * before it answered; taken away, the file will be there again
             * only if the next step changes something. */
            if (exists(a.dml) && rename(a.dml, fmt("%s/interaction/%s.dml.sql", rw, name)) < 0) {
                die(fmt("cannot move %s: %s", path(a.dml), strerror(errno)), NULL);
            }
            sl_push(outputs, fmt("interaction/%s.dml.sql", name));
            /* Likewise what it printed: the server flushes every line as
             * it goes, and all of a step's come before its answer. */
            cut_output(a.server_out, &a.out_at, -1, fmt("%s/interaction/%s.stdout", rw, name));
            cut_output(a.server_err, &a.err_at, -1, fmt("%s/interaction/%s.stderr", rw, name));
            sl_push(outputs, fmt("interaction/%s.stdout", name));
            sl_push(outputs, fmt("interaction/%s.stderr", name));
        }
        free(text);
        sl_free(&options);
        if (*stopped) {
            stale_after(run_dir, rw, RUN_REQUESTS, stopped, outputs);
            break;
        }
        if (fs_interrupted) {
            break;
        }
    }
    sl_free(&files);
    server_stop(a.server);
    bail_if_interrupted();
    if (report_problems != before) {
        return -1;
    }
    return app_end(run_dir, &a, outputs);
}

/* The passes a browser run is driven through, in order, as the driver
 * reads them from the run's context; none if it cannot (run_ready has
 * said why). */
static void passes_of(const char *run_dir, struct strlist *out) {
    char browser[PATHLEN], said[PATHLEN], *text, *line, *end;
    char *argv[4];
    if (!find_browser(browser)) {
        return;
    }
    argv[0] = browser;
    argv[1] = "--check";
    argv[2] = (char *) run_dir;
    argv[3] = NULL;
    join(said, work, "passes.txt");
    if (run(argv, said, "/dev/null") != 0) {
        return;
    }
    text = read_text(said);
    for (line = text; line && *line; line = end ? end + 1 : line + strlen(line)) {
        end = strchr(line, '\n');
        if (end) {
            *end = 0;
        }
        if (has_prefix(line, "pass ") && is_pass(line + 5) && out->n < PASSES_MAX) {
            sl_push(out, line + 5);
        }
    }
    free(text);
}

/* What the drivers say, a line at a time, to the pass it is for. */
static void on_pass_line(int i, const char *line, void *data) {
    struct browsing *b = data;
    on_browse_line(line, &b[i]);
}

/* A browser run: the application started once per pass, passes->v[i]
 * into rws[i], and every pass driven through at once, each by a driver of
 * its own, which leaves the records under rws[i] and goes on outputs[i];
 * then every application stopped, and its database dumped.  Returns 0,
 * or -1 with the problem reported, and nothing of the run is to be
 * recorded then.  A step that cannot be done is not that kind of problem:
 * that pass stops there, with NAME.error saying why, and is recorded as
 * far as it got; stopped[i] (PATHLEN) names the step it stopped at, or is
 * empty. */
static int run_passes(const char *run_dir, const char *exe, const struct strlist *passes, char rws[][PATHLEN],
                      struct strlist outputs[], char stopped[][PATHLEN]) {
    struct app apps[PASSES_MAX];
    struct browsing b[PASSES_MAX];
    struct child *ds[PASSES_MAX], *going[PASSES_MAX] = { NULL };
    char browse_err[PASSES_MAX][PATHLEN];
    int slot[PASSES_MAX], taken[PASSES_MAX] = { 0 }, done[PASSES_MAX] = { 0 }, said[PASSES_MAX] = { 0 };
    int i, j, n = passes->n, before = report_problems, ok = 1;

    for (i = 0; i < n; i++) {
        *stopped[i] = 0;
        if (app_start(run_dir, exe, 1, rws[i], &servers[i], &apps[i], &outputs[i]) < 0) {
            while (--i >= 0) {
                server_stop(&servers[i]);
            }
            return -1;
        }
    }
    /* A pass to the driver of its first user's engine, if no pass before
     * it took that one, or else to the first one free. */
    for (i = 0; i < n; i++) {
        int want = engine_at(passes->v[i]);
        slot[i] = want >= 0 && want < PASSES_MAX && !taken[want] ? want : -1;
        for (j = 0; slot[i] < 0 && j < PASSES_MAX; j++) {
            if (!taken[j]) {
                slot[i] = j;
            }
        }
        taken[slot[i]] = 1;
    }
    /* The driver takes the steps from the run and leaves the records
     * under rws[i], saying after each step that it has; the server's
     * output is cut on those lines, as after a request.  The screenshots
     * and the recordings, when asked for, go straight into the run,
     * emptied of the last ones by run_run. */
    for (i = 0; i < n; i++) {
        struct browsing one = { run_dir, rws[i], apps[i].dml, apps[i].server_out, apps[i].server_err,
                                &apps[i].out_at, &apps[i].err_at, &outputs[i], 0, "" };
        b[i] = one;
        join(browse_err[i], rws[i], "browse.stderr");
        ds[i] = driver_for(slot[i]);
        said[i] = ds[i] != NULL
                  && child_say(ds[i], fmt("run %s %s %s pass=%s%s%s%s\n",
                                          shell_quoted(fmt("http://127.0.0.1:%d", servers[i].port)),
                                          shell_quoted(run_dir), shell_quoted(fmt("%s/interaction", rws[i])),
                                          shell_quoted(passes->v[i]),
                                          opt_screenshot ? fmt(" screenshots=%s", shell_quoted(fmt("%s/screenshots", run_dir))) : "",
                                          opt_record ? fmt(" recordings=%s", shell_quoted(fmt("%s/recordings", run_dir))) : "",
                                          opt_frame ? fmt(" frame=%s", shell_quoted(opt_frame)) : "")) == 0;
    }
    {
        /* Those that took their pass, heard together. */
        int at[PASSES_MAX], heard[PASSES_MAX];
        struct browsing bs[PASSES_MAX];
        for (i = 0, j = 0; i < n; i++) {
            if (said[i]) {
                going[j] = ds[i];
                bs[j] = b[i];
                at[j++] = i;
            }
        }
        children_lines(going, j, "done", on_pass_line, bs, heard);
        while (--j >= 0) {
            b[at[j]] = bs[j];
            done[at[j]] = heard[j];
        }
    }
    for (i = 0; i < n; i++) {
        /* What the driver said to stderr during this pass, beside its
         * records, where head_of finds it. */
        if (ds[i]) {
            cut_output(driver_err_path(slot[i]), &driver_err_at[slot[i]], -1, browse_err[i]);
        }
        if (*b[i].stopped) {
            /* The driver ends the pass at the step it could not do. */
            strcpy(stopped[i], b[i].stopped);
            stale_after(run_dir, rws[i], RUN_BROWSER, stopped[i], &outputs[i]);
        } else if (!fs_interrupted && (!done[i] || !b[i].steps)) {
            die_later(fmt("%s does not run %s for %s", path("urt-browse"), passes->v[i], test(run_dir)),
                      head_of(browse_err[i], "(it said nothing)"), NULL);
        }
        if (!done[i] && ds[i]) {
            /* The driver is gone, or not to be trusted; the next run
             * starts it afresh. */
            child_stop(ds[i]);
            driver_up[slot[i]] = 0;
        }
    }
    for (i = 0; i < n; i++) {
        server_stop(&servers[i]);
    }
    bail_if_interrupted();
    if (report_problems != before) {
        return -1;
    }
    for (i = 0; i < n && ok; i++) {
        ok = app_end(run_dir, &apps[i], &outputs[i]) == 0;
    }
    return ok ? 0 : -1;
}

/* Whether two of a pass's records are the same, a record that is not
 * there counting as an empty one: the driver writes a record only when
 * there is anything, and an empty server stream is dropped as well. */
static int same_record(const char *a, const char *b) {
    int ea = exists(a), eb = exists(b);
    char *text;
    int empty;
    if (ea && eb) {
        return same_contents(a, b);
    }
    if (!ea && !eb) {
        return 1;
    }
    text = read_text(ea ? a : b);
    empty = !text || !*text;
    free(text);
    return empty;
}

/* One record of a browser run, compared or recorded for every pass it
 * was driven through.  rws[i] is where pass i left what came of it; rel
 * names the record under the run.  The passes agreeing on it share
 * NAME.R; a pass that records it differently has its own NAME.PASS.R
 * beside that, which is what it is compared against.  Recording goes by
 * the majority: the largest group of agreeing passes writes the shared
 * file, the others their own, and an own file whose pass has come to
 * agree with the shared one goes, so that the files converge; a tie
 * goes to the pass first in order. */
static void settle_shared(const char *run_dir, const char *rel, const struct strlist *engs, char rws[][PATHLEN],
                          char *bad, char *changed, size_t size, struct strlist *diffs) {
    char clean[8][PATHLEN], now[PATHLEN], want[PATHLEN], own[PATHLEN], d[PATHLEN];
    int group[8], count[8], i, j, major = 0;

    for (i = 0; i < engs->n; i++) {
        pathf(now, "%s/%s", rws[i], rel);
        pathf(clean[i], "%s.now", now);
        if (exists(now) && normalise_run(rel, now, clean[i]) == 0
            && (has_suffix(rel, ".stdout") || has_suffix(rel, ".stderr"))) {
            unlink(clean[i]);               /* nothing worth keeping */
        }
    }
    if (opt_check) {
        /* Passes that share the expected file and recorded the same
         * thing are reported once, as the record; the rest each. */
        int alike = 1;
        for (i = 0; i < engs->n && alike; i++) {
            own_name(own, rel, engs->v[i]);
            alike = !exists(fmt("%s/%s", run_dir, own)) && (i == 0 || same_record(clean[0], clean[i]));
        }
        for (i = 0; i < (alike ? 1 : engs->n); i++) {
            own_name(own, rel, engs->v[i]);
            join(want, run_dir, exists(fmt("%s/%s", run_dir, own)) ? own : rel);
            pathf(d, "%s.diff", clean[i]);
            if (diff_into(want, clean[i], d) != 0) {
                list_add(bad, size, alike ? rel : fmt("%s [%s]", rel, engs->v[i]));
                sl_push(diffs, d);
            }
        }
        return;
    }
    /* Group the passes by what they recorded; the largest group, the
     * earliest among equals, is the majority. */
    for (i = 0; i < engs->n; i++) {
        group[i] = i;
        count[i] = 0;
        for (j = 0; j < i; j++) {
            if (group[j] == j && same_record(clean[j], clean[i])) {
                group[i] = j;
                break;
            }
        }
        count[group[i]]++;
    }
    for (i = 1; i < engs->n; i++) {
        if (group[i] == i && count[i] > count[major]) {
            major = i;
        }
    }
    join(want, run_dir, rel);
    if (!exists(clean[major])) {
        if (exists(want)) {
            if (unlink(want) < 0) {
                die(fmt("cannot delete %s: %s", path(want), strerror(errno)), NULL);
            }
            list_add(changed, size, fmt("-%s", rel));
        }
    } else if (!same_contents(want, clean[major])) {
        dirname_of(d, want);
        if (mkdir_p(d) < 0 || copy_file(clean[major], want) < 0) {
            die(fmt("cannot write %s: %s", path(want), strerror(errno)), NULL);
        }
        list_add(changed, size, rel);
    }
    for (i = 0; i < engs->n; i++) {
        own_name(own, rel, engs->v[i]);
        join(want, run_dir, own);
        if (group[i] == major) {
            if (exists(want)) {
                if (unlink(want) < 0) {
                    die(fmt("cannot delete %s: %s", path(want), strerror(errno)), NULL);
                }
                list_add(changed, size, fmt("-%s [%s]", rel, engs->v[i]));
            }
        } else if (!exists(clean[i])) {
            /* This pass records nothing where the others record
             * something: its own file, empty, says so. */
            if (!is_file(want) || !same_record(want, clean[i])) {
                if (write_text(want, "") < 0) {
                    die(fmt("cannot write %s: %s", path(want), strerror(errno)), NULL);
                }
                list_add(changed, size, fmt("%s [%s]", rel, engs->v[i]));
            }
        } else if (!same_contents(want, clean[i])) {
            if (copy_file(clean[i], want) < 0) {
                die(fmt("cannot write %s: %s", path(want), strerror(errno)), NULL);
            }
            list_add(changed, size, fmt("%s [%s]", rel, engs->v[i]));
        }
    }
}

/* Where a run stopped, under its verdict: the request that could not be
 * sent, the wait or the step that could not be done, and what its
 * NAME.error under rw says.  tag names the pass, for a run driven
 * through several. */
static void say_stopped(enum run_kind kind, const char *run_dir, const char *rw, const char *name, const char *tag) {
    char *text = read_text(fmt("%s/interaction/%s.error", rw, name)), *l, *end;
    char nothing[] = "(nothing was said)\n";
    char *why = text && *text ? text : nothing;
    int first = 1, is_wait = kind == RUN_REQUESTS && is_file(fmt("%s/interaction/%s.wait", run_dir, name));
    for (l = why; *l; l = end ? end + 1 : l + strlen(l)) {
        end = strchr(l, '\n');
        if (end) {
            *end = 0;
        }
        if (first) {
            printf("    %s %s %s%s: %s\n", kind == RUN_BROWSER ? "step" : is_wait ? "wait" : "request", key(name),
                   kind == RUN_REQUESTS && !is_wait ? "could not be sent" : "cannot be done", tag, info(l));
            first = 0;
        } else {
            printf("        %s\n", info(l));
        }
    }
    if (kind == RUN_REQUESTS && (strstr(why, "(exit 7)") || strstr(why, "(exit 28)"))) {
        int said = exists(fmt("%s/interaction/%s.stderr", rw, name));
        printf("        %s\n", info(fmt("the server may have died, or be hung%s",
                                         said ? fmt("; what it printed meanwhile is in %s.stderr", name) : "")));
    }
    free(text);
}

/* What the drivers of a browser run said that is no failure to run it
 * -- a film they could not make, a frame they had to make again -- as
 * notes under its verdict, pass by pass: their lines that start
 * "urt-browse:", the rest of what goes to their stderr being Python's
 * and Playwright's. */
static void driver_notes(int n, char rws[][PATHLEN]) {
    int i;
    for (i = 0; i < n; i++) {
        char *text = read_text(fmt("%s/browse.stderr", rws[i])), *line, *end;
        for (line = text; line && *line; line = end ? end + 1 : NULL) {
            end = strchr(line, '\n');
            if (end) {
                *end = 0;
            }
            if (has_prefix(line, "urt-browse:")) {
                printf("    %s\n", info(line));
            }
        }
        free(text);
    }
}

/* One run: the application started once and sent the requests, or once
 * per pass and driven through, and what came of it compared or
 * recorded.  A run that stopped at a request or a step is recorded as
 * far as it got, and fails. */
static void run_run(const char *run_dir, const char *exe) {
    char rws[PASSES_MAX][PATHLEN], stopped[PASSES_MAX][PATHLEN];
    char bad[PATHLEN] = "", changed[PATHLEN] = "", failed[PATHLEN] = "", note[512] = "";
    struct strlist engs = { 0, 0, 0 }, outputs = { 0, 0, 0 }, shared = { 0, 0, 0 }, diffs = { 0, 0, 0 };
    struct strlist mine[PASSES_MAX];
    int i, kind = kind_of(run_dir), before = report_problems, any_stopped = 0;
    long t0 = tenths_now();

    n_runs++;
    if (kind == RUN_BROWSER) {
        /* The passes, under the name engs: what records differently is
         * a pass's own. */
        passes_of(run_dir, &engs);
        if (engs.n == 0) {
            die_later(fmt("%s cannot say the passes of %s", path("urt-browse"), test(run_dir)), NULL);
            return;
        }
        for (i = 0; i < 2; i++) {
            /* This run's pictures, and none of an earlier run's. */
            char pictures[PATHLEN];
            if (!(i == 0 ? opt_screenshot : opt_record)) {
                continue;
            }
            join(pictures, run_dir, i == 0 ? "screenshots" : "recordings");
            rm_rf(pictures);
            if (mkdir_p(pictures) < 0) {
                die(fmt("cannot make %s: %s", path(pictures), strerror(errno)), NULL);
            }
        }
        for (i = 0; i < engs.n; i++) {
            struct strlist none = { 0, 0, 0 };
            mine[i] = none;
            pathf(rws[i], "%s/run-%s", work, engs.v[i]);
        }
        if (run_passes(run_dir, exe, &engs, rws, mine, stopped) < 0) {
            for (i = 0; i < engs.n; i++) {
                sl_free(&mine[i]);
            }
            sl_free(&engs);
            return;
        }
        for (i = 0; i < engs.n; i++) {
            /* Every record may be shared between the passes. */
            int j;
            for (j = 0; j < mine[i].n; j++) {
                if (!sl_has(&shared, mine[i].v[j])) {
                    sl_push(&shared, mine[i].v[j]);
                }
            }
            sl_free(&mine[i]);
        }
        sl_sort(&shared);
        for (i = 0; i < shared.n; i++) {
            settle_shared(run_dir, shared.v[i], &engs, rws, bad, changed, sizeof bad, &diffs);
        }
    } else {
        join(rws[0], work, "run");
        if (run_app(run_dir, exe, rws[0], &outputs, failed, sizeof failed, stopped[0]) < 0) {
            sl_free(&outputs);
            return;
        }
        /* An output that is no longer produced -- the NAME.sql of a
         * request that changes nothing any more -- goes, or fails the
         * check. */
        sl_sort(&outputs);
        for (i = 0; i < outputs.n; i++) {
            settle(run_dir, outputs.v[i], fmt("%s/%s", rws[0], outputs.v[i]), bad, changed, sizeof bad, &diffs);
        }
    }
    (void) before;
    for (i = 0; i < (kind == RUN_BROWSER ? engs.n : 1); i++) {
        any_stopped |= *stopped[i] != 0;
    }

    if (*failed) {
        snprintf(note, sizeof note, " %scurl failed: %s%s", style(ST_AMBER), failed, style(ST_RESET));
    }
    snprintf(note + strlen(note), sizeof note - strlen(note), "%s", took(tenths_now() - t0));
    if ((opt_check && *bad) || any_stopped) {
        /* What differs, or, when recording, what was written: a run that
         * stopped fails either way, with the complaint under the verdict. */
        const char *list = opt_check ? bad : changed;
        blank_before();
        say(ST_BAD, "FAIL", run_dir, fmt("%s%s", *list ? fmt(" (%s)", list) : "", note));
        for (i = 0; i < diffs.n; i++) {
            show_diff(diffs.v[i]);
        }
        for (i = 0; i < (kind == RUN_BROWSER ? engs.n : 1); i++) {
            /* Passes that stopped at the same step with the same complaint
             * are reported together, and not at all by name when they are
             * all of them. */
            char who[256] = "";
            int j, n = 0, reported = 0;
            for (j = 0; j < engs.n && *stopped[i]; j++) {
                if (*stopped[j] && !strcmp(stopped[j], stopped[i])
                    && same_record(fmt("%s/interaction/%s.error", rws[i], stopped[i]),
                                   fmt("%s/interaction/%s.error", rws[j], stopped[j]))) {
                    reported |= j < i;
                    list_add(who, sizeof who, engs.v[j]);
                    n++;
                }
            }
            if (*stopped[i] && !reported) {
                say_stopped(kind, run_dir, rws[i], stopped[i], kind == RUN_BROWSER && n < engs.n ? fmt(" [%s]", who) : "");
            }
        }
        if (kind == RUN_BROWSER) {
            driver_notes(engs.n, rws);
        }
        putchar('\n');
        set_at_blank(1);
        n_bad++;
        report_failed = 1;
    } else {
        if (*changed) {
            say(ST_WARN, "UPDATED", run_dir, fmt(" (%s)%s", changed, note));
            n_changed++;
        } else {
            say(ST_OK, "OK", run_dir, note);
            n_ok++;
        }
        if (kind == RUN_BROWSER) {
            driver_notes(engs.n, rws);
        }
        set_at_blank(0);
    }
    sl_free(&engs);
    sl_free(&outputs);
    sl_free(&shared);
    sl_free(&diffs);
}

/* Whether the project is built in debug mode: its .urp says debug before
 * its first blank line, or the case's args -debug. */
static int is_debug(const char *project, const struct strlist *extra) {
    char *text = read_text(fmt("%s.urp", project)), *line, *end;
    int i, found = 0;
    for (i = 0; i < extra->n; i++) {
        found |= !strcmp(extra->v[i], "-debug");
    }
    for (line = text; !found && line && *line && *line != '\n'; line = end ? end + 1 : NULL) {
        end = strchr(line, '\n');
        found = !strncmp(line, "debug", 5) && (line[5] == '\n' || line[5] == 0 || line[5] == ' ' || line[5] == '\r');
    }
    free(text);
    return found;
}

/* Run the compiler for one case and compare, or record, what it wrote;
 * then the application, if the case has runs. */
static void run_case(const char *case_dir, const char *project) {
    char want_dir[PATHLEN], f[PATHLEN], now[PATHLEN], exe[PATHLEN], settings[PATHLEN], built[PATHLEN];
    struct strlist entries = { 0, 0, 0 }, argv = { 0, 0, 0 }, extra = { 0, 0, 0 }, runs = { 0, 0, 0 };
    char missing[PATHLEN] = "", bad[PATHLEN] = "", changed[PATHLEN] = "", note[512] = "";
    const char *stop = "parseJob", *stopper;
    char *args, **command;
    int maxrank = 0, i, status, own_stop = 0, no_exe, lock = -1;
    long t0, elapsed;

    join(want_dir, case_dir, "expected");
    rm_rf(work);
    if (mkdir_p(work) < 0) {
        die(fmt("cannot make %s", path(work)), NULL);
    }

    /* What this case asks for, and how far the compiler has to go for it.
     * The compiler comes first on the command line, then its standing
     * flags, then one flag per artifact, then the case's own args. */
    sl_push(&argv, urweb);
    for (i = 0; i < urweb_flags.n; i++) {
        sl_push(&argv, urweb_flags.v[i]);
    }
    list_dir(want_dir, LS_FILES, &entries);
    for (i = 0; i < entries.n; i++) {
        const struct artifact *a = artifact_named(entries.v[i]);
        if (!a) {                           /* layout() has seen to it; belt and braces */
            die(fmt("%s is not something the compiler produces", path(fmt("%s/%s", want_dir, entries.v[i]))),
                fmt("the artifacts are: %s", artifact_names()), NULL);
        }
        if (a->flag) {
            join(f, work, a->name);
            sl_push(&argv, a->flag);
            sl_push(&argv, f);
        }
        if (a->rank > maxrank) {
            maxrank = a->rank;
            stop = a->phase;
        }
    }
    runs_of(case_dir, &runs);
    if (entries.n == 0 && runs.n == 0) {
        warn(fmt("%s asks for nothing; %s says what there is to ask for",
                 path(want_dir), cmd(fmt("%s --edit %s", me, case_dir))));
        n_skipped++;
        sl_free(&entries);
        sl_free(&argv);
        return;
    }

    /* How far to run.  A case whose own args say that is left to it; one
     * with runs goes all the way, to an application, and so cannot say. */
    join(f, case_dir, "args");
    args = is_file(f) ? read_text(f) : NULL;
    if (args) {
        split_words(args, &extra);
    }
    free(args);
    if (runs.n && (stopper = no_application(&extra))) {
        die_later(fmt("%s has runs, and %s in its args", test(case_dir), key(stopper)),
                  "the runs need the application, so the compiler has to run to the end and write it where it is looked for",
                  fmt("take %s out of %s, or the runs out of the case", key(stopper), path(f)), NULL);
        sl_free(&entries);
        sl_free(&argv);
        sl_free(&extra);
        sl_free(&runs);
        return;
    }
    for (i = 0, no_exe = 0; i < runs.n; i++) {
        no_exe |= !run_ready(runs.v[i]);
    }
    if (no_exe) {
        sl_free(&entries);
        sl_free(&argv);
        sl_free(&extra);
        sl_free(&runs);
        return;
    }
    n_cases++;
    for (i = 0; i < extra.n; i++) {
        if (!strcmp(extra.v[i], "-stop") || !strcmp(extra.v[i], "-tc")) {
            own_stop = 1;
        }
    }
    /* With runs the compiler goes all the way, and builds the application
     * in the work directory, whatever the project says: an exe directive
     * can name a file another project names too (/tmp/webapp, in tests/),
     * and two cases built at once would swap applications.  From there it
     * is moved into the case, beside expected/: out of the project's way,
     * and still there afterwards, to be started by hand when a run needs
     * looking into.  The one from last time goes first, lest it stand in
     * for one that does not build. */
    join(exe, case_dir, "app.exe");
    join(settings, work, "settings.txt");
    if (runs.n) {
        if (unlink(exe) < 0 && errno != ENOENT) {
            die(fmt("cannot delete %s: %s", path(exe), strerror(errno)), NULL);
        }
        if (!sl_has(&entries, "settings.txt")) {
            sl_push(&argv, "-saveSettings");
            sl_push(&argv, settings);
        }
        sl_push(&argv, "-output");
        sl_push(&argv, fmt("%s/app.exe", work));
    } else if (!own_stop) {
        sl_push(&argv, "-stop");
        sl_push(&argv, stop);
        sl_push(&argv, "-quiet");
    }
    for (i = 0; i < extra.n; i++) {
        sl_push(&argv, extra.v[i]);
    }
    sl_push(&argv, project);

    command = sl_argv(&argv);
    t0 = tenths_now();
    if (in_worker && runs.n && is_debug(project, &extra)) {
        /* In debug mode the compiler writes the C to /tmp/webapp.c and
         * builds it from there, whoever compiles: with -j, one worker at
         * a time.  The lock goes with the descriptor. */
        lock = open(debug_lock, O_RDWR | O_CREAT, 0600);
        while (lock >= 0 && lockf(lock, F_LOCK, 0) < 0 && errno == EINTR && !fs_interrupted) {
            ;
        }
    }
    status = run(command, fmt("%s/stdout.txt", work), fmt("%s/stderr.txt", work));
    if (lock >= 0) {
        close(lock);
    }
    elapsed = tenths_now() - t0;
    free(command);
    bail_if_interrupted();
    join(f, work, "exitcode");
    write_text(f, fmt("%d\n", status));
    if (runs.n && status == 0) {
        exe_of(settings, built);
        if (*built && is_file(built) && move_file(built, exe) < 0) {
            die(fmt("cannot move %s to %s: %s", path(built), path(exe), strerror(errno)), NULL);
        }
    }

    /* What was asked for and not written.  After a failed run that is to
     * be expected, and the result line names them so that an empty
     * artifact can be told from one the compiler never got to.  If the
     * compiler was content and still did not write something, the fault
     * is here rather than in the program under test. */
    for (i = 0; i < entries.n; i++) {
        const struct artifact *a = artifact_named(entries.v[i]);
        join(f, work, a->name);
        if (!a->flag || is_file(f)) {
            continue;
        }
        if (status == 0) {
            die_later(fmt("urweb was asked for %s, exited 0, and wrote no such file", a->name),
                      runs.n ? "and it ran to the end"
                             : fmt("it was stopped after %s, which this runner takes to be late enough", key(stop)),
                      fmt("if the compiler's phases have moved, the artifact table in %s needs to know", path(me)), NULL);
        }
        list_add(missing, sizeof missing, a->name);
    }

    /* The compiler's exit status and how long it took, after the verdict. */
    if (status != 0) {
        n_nonzero++;
        snprintf(note, sizeof note, " %sexit %d%s", style(ST_AMBER), status, style(ST_RESET));
        if (*missing) {
            snprintf(note + strlen(note), sizeof note - strlen(note), ", not written: %s", missing);
        }
    }
    snprintf(note + strlen(note), sizeof note - strlen(note), "%s", took(elapsed));

    for (i = 0; i < entries.n; i++) {
        const char *a = entries.v[i];
        char want[PATHLEN];
        join(f, work, a);
        join(want, want_dir, a);
        pathf(now, "%s.now", f);
        if (!is_file(f)) {
            write_text(f, "");
        }
        normalise(a, f, now);
        if (opt_check) {
            if (diff_into(want, now, fmt("%s.diff", f)) != 0) {
                list_add(bad, sizeof bad, a);
            }
        } else {
            if (!same_contents(want, now)) {
                list_add(changed, sizeof changed, a);
            }
            if (copy_file(now, want) < 0) {
                die(fmt("cannot write %s: %s", path(want), strerror(errno)), NULL);
            }
        }
    }

    /* Runs without an application to run are a failure in either mode:
     * there is nothing to record for them, and saying OK would hide it. */
    no_exe = runs.n && (status != 0 || !can_exec(exe));
    if ((opt_check && *bad) || no_exe) {
        /* The verdict, then the diffs under it, set apart by blank lines. */
        blank_before();
        say(ST_BAD, "FAIL", case_dir,
            fmt(" (%s%s%s)%s", opt_check ? bad : "", opt_check && *bad && no_exe ? "; " : "",
                no_exe ? fmt("no application for its %d run%s", runs.n, runs.n == 1 ? "" : "s") : "", note));
        for (i = 0; opt_check && i < entries.n; i++) {
            pathf(f, "%s/%s.diff", work, entries.v[i]);
            show_diff(f);
        }
        if (no_exe) {
            join(f, work, "stderr.txt");
            printf("    %s\n", info(head_of(f, "(it said nothing)")));
        }
        putchar('\n');
        set_at_blank(1);
        n_bad++;
        report_failed = 1;
    } else {
        if (!opt_check && *changed) {
            say(ST_WARN, "UPDATED", case_dir, fmt(" (%s)%s", changed, note));
            n_changed++;
        } else {
            say(ST_OK, "OK", case_dir, note);
            n_ok++;
        }
        set_at_blank(0);
    }
    for (i = 0; !no_exe && i < runs.n; i++) {
        run_run(runs.v[i], exe);
    }
    sl_free(&entries);
    sl_free(&argv);
    sl_free(&extra);
    sl_free(&runs);
}

/* One line for all of it, once the last case has had its say. */
static void summary(void) {
    char s[512] = "";
    long elapsed = tenths_now() - started;
    if (n_ok) {
        list_add(s, sizeof s, fmt("%s%d ok%s", style(ST_GREEN), n_ok, style(ST_RESET)));
    }
    if (n_bad) {
        list_add(s, sizeof s, fmt("%s%d failed%s", style(ST_RED), n_bad, style(ST_RESET)));
    }
    if (n_changed) {
        list_add(s, sizeof s, fmt("%s%d updated%s", style(ST_AMBER), n_changed, style(ST_RESET)));
    }
    if (n_skipped) {
        list_add(s, sizeof s, fmt("%d asking for nothing", n_skipped));
    }
    if (n_nonzero) {
        list_add(s, sizeof s, fmt("%d with non-zero exit", n_nonzero));
    }
    printf("%s%d case%s%s%s%s%s, in %ld.%lds\n", style(ST_STRONG), n_cases, n_cases == 1 ? "" : "s",
           n_runs ? fmt(" and %d run%s", n_runs, n_runs == 1 ? "" : "s") : "", style(ST_RESET),
           *s ? ": " : "", s, elapsed / 10, elapsed % 10);
}

/* With --list, a case is named and left alone. */
static void do_case(const char *case_dir, const char *project) {
    if (opt_list) {
        printf("%s\n", subject(case_dir));
    } else {
        run_case(case_dir, project);
    }
}

/* One entry of the list find_cases made: a snapshot directory, or one
 * case of one, which is checked with its snapshot and then taken alone.
 * fn gets each case that passes, with its project. */
static void handle(const char *entry, void (*fn)(const char *case_dir, const char *project)) {
    char snap[PATHLEN], only[PATHLEN] = "", urt_dir[PATHLEN], base[PATHLEN], project[PATHLEN], f[PATHLEN];
    struct strlist subdirs = { 0, 0, 0 };
    char *named;
    int i;

    snprintf(snap, sizeof snap, "%s", entry);
    if (!has_suffix(snap, ".urt/snapshot")) {
        snprintf(only, sizeof only, "%s", entry);
        dirname_of(snap, entry);
    }
    dirname_of(urt_dir, snap);

    /* The project is the urt directory without its suffix, unless a
     * project file says otherwise -- which it must when the compiler is
     * given a directory rather than a project, as in -demo mode. */
    snprintf(base, sizeof base, "%s", urt_dir);
    strip_suffix(base, ".urt");
    join(f, urt_dir, "project");
    if (is_file(f) && (named = read_text(f))) {
        char dir[PATHLEN];
        while (*named && named[strlen(named) - 1] == '\n') {
            named[strlen(named) - 1] = 0;
        }
        dirname_of(dir, urt_dir);
        if (!strcmp(dir, ".") && !strchr(urt_dir, '/')) {
            snprintf(project, sizeof project, "%s", named);
        } else {
            join(project, dir, named);
        }
        free(named);
    } else {
        snprintf(project, sizeof project, "%s", base);
        if (!is_file(fmt("%s.urp", base)) && !is_file(fmt("%s.ur", base))) {
            die_later(fmt("%s is named for a project '%s' that is not beside it", test(snap), basename_of(base)),
                      fmt("expected %s or %s", path(fmt("%s.urp", base)), path(fmt("%s.ur", base))),
                      fmt("write the project's name in %s if it goes by another", path(fmt("%s/project", urt_dir))), NULL);
            return;
        }
    }

    if (!layout(urt_dir, snap)) {
        return;
    }

    {
        struct strlist names = { 0, 0, 0 };
        list_dir(snap, LS_DIRS, &names);
        for (i = 0; i < names.n; i++) {
            join(f, snap, names.v[i]);
            sl_push(&subdirs, f);
        }
        sl_free(&names);
    }
    if (!subdirs.n) {
        die_later(fmt("%s holds no cases", test(snap)),
                  fmt("%s makes one", cmd(fmt("%s --create %s", me, base))), NULL);
    } else {
        redundant(snap, &subdirs);
        for (i = 0; i < subdirs.n; i++) {
            const char *c = subdirs.v[i];
            if (*only && strcmp(c, only)) {
                continue;
            }
            join(f, c, "expected");
            if (!is_dir(f)) {
                die_later(fmt("%s has no expected directory", test(c)),
                          fmt("every case under %s needs one; %s makes them", test(snap), cmd(fmt("%s --create", me))), NULL);
                continue;
            }
            fn(c, project);
        }
    }
    sl_free(&subdirs);
}

/* --------------------------------------------------------------- run_all */

/* The scratch directory, made when first needed. */
static void need_tmp(void) {
    static char *probe[] = { "diff", "-u", "--label", "a", "--label", "b", "/dev/null", "/dev/null", NULL };
    if (*tmp) {
        return;
    }
    if (make_tmpdir(tmp) < 0) {
        die("cannot make a temporary directory", NULL);
    }
    join(work, tmp, "work");
    /* GNU diff labels its two sides for us; without that the header
     * names a temporary file, which is only confusing. */
    diff_labels = run(probe, "/dev/null", "/dev/null") == 0;
}

/* The first case among the snapshots that has runs, or NULL: the runs
 * need curl and sqlite3, and are looked for before anything is done. */
static const char *first_with_runs(const struct strlist *snaps) {
    static char found[PATHLEN];
    struct strlist cases = { 0, 0, 0 };
    int i, j;
    for (i = 0; i < snaps->n; i++) {
        const char *snap = snaps->v[i];
        if (!has_suffix(snap, ".urt/snapshot")) {
            sl_push(&cases, snap);
        } else {
            struct strlist names = { 0, 0, 0 };
            list_dir(snap, LS_DIRS, &names);
            for (j = 0; j < names.n; j++) {
                sl_push(&cases, fmt("%s/%s", snap, names.v[j]));
            }
            sl_free(&names);
        }
    }
    for (i = 0; i < cases.n; i++) {
        if (is_dir(fmt("%s/request-response", cases.v[i]))) {
            snprintf(found, sizeof found, "%s", cases.v[i]);
            sl_free(&cases);
            return found;
        }
    }
    sl_free(&cases);
    return NULL;
}

/* -------------------------------------------------------------------- -j */

/* With -j the projects go N at a time.  Once the list is known, N
 * workers are forked, each with a scratch directory, servers and drivers
 * of its own; each is handed a project, does it as a plain run would,
 * says it is done and how the counts stand, and is handed the next.
 * What it prints, its stderr lines marked among its stdout's, comes here
 * on one pipe and is printed in the order of the list: the first project
 * not printed yet as it comes, the others when their turn comes.  So the
 * output is a plain run's, blank lines and all, and so is the summary,
 * from the workers' counts.  A worker that dies in a project, as die()
 * makes it, ends the run there, as die() ends a plain one: the projects
 * before it are printed, those after it are not, and its exit status is
 * ours. */

enum { PO_TODO, PO_UNDERWAY, PO_DONE, PO_LOST };

struct project_out {            /* what a worker printed for one project */
    char *buf;
    size_t len, cap, shown, scanned;
    int state;                  /* PO_* */
};

struct worker_proc {
    pid_t pid;
    int out, cmd;               /* its output, read here; its next project, written here; -1 once closed */
    int at;                     /* the project it is on, or -1 */
    int counts[8];              /* as its last project left them */
    int status;
};

/* A worker: a project at a time, by its number in the list, until the
 * list is done; a project's counts after it, in a marked line. */
static void worker(const struct strlist *snaps, int cmd, int out) {
    char line[64];
    FILE *in;
    in_worker = 1;
    /* The parent's scratch directory is the parent's to remove. */
    *tmp = 0;
    need_tmp();
    if (dup2(out, STDOUT_FILENO) < 0 || dup2(out, STDERR_FILENO) < 0) {
        _exit(2);
    }
    close(out);
    report_merge();
    in = fdopen(cmd, "r");
    while (in && fgets(line, sizeof line, in)) {
        handle(snaps->v[atoi(line)], do_case);
        printf("%c%d %d %d %d %d %d %d %d\n", MARK_DONE, n_cases, n_runs, n_ok, n_bad, n_changed, n_skipped,
               n_nonzero, report_failed);
        fflush(stdout);
        bail_if_interrupted();
    }
    bail_if_interrupted();
    exit(0);
}

/* Whether worker w took project k: one that is gone does not, and says
 * so by its output's end. */
static int give(struct worker_proc *w, struct project_out *p, int k) {
    char line[32];
    int n = snprintf(line, sizeof line, "%d\n", k);
    if (w->cmd < 0 || write(w->cmd, line, (size_t) n) != n) {
        return 0;
    }
    w->at = k;
    p[k].state = PO_UNDERWAY;
    return 1;
}

static void no_more(struct worker_proc *w) {
    if (w->cmd >= 0) {
        close(w->cmd);
        w->cmd = -1;
    }
}

/* One line of a worker's, put where it belongs; a line it did not end
 * is ended. */
static void put_line(const char *line, size_t n) {
    int ended = n > 0 && line[n - 1] == '\n';
    switch (n ? line[0] : 0) {
    case REPORT_STDERR_MARK:
        fflush(stdout);
        fwrite(line + 1, 1, n - 1, stderr);
        if (!ended) {
            fputc('\n', stderr);
        }
        break;
    case MARK_DONE:
        break;
    case MARK_BLANK:
        if (!at_blank) {
            putchar('\n');
        }
        break;
    case MARK_AT:
        at_blank = n > 1 && line[1] == '1';
        break;
    default:
        fwrite(line, 1, n, stdout);
        if (!ended) {
            putchar('\n');
        }
    }
}

/* What can be printed now, in order, from project k on: the lines of
 * the first project not done yet, and every project done before it.  The
 * first project not printed in full, or the lost one, after which
 * nothing is. */
static int put_ready(struct project_out *p, int n, int k) {
    for (; k < n; k++) {
        struct project_out *o = &p[k];
        char *nl;
        while (o->shown < o->len && (nl = memchr(o->buf + o->shown, '\n', o->len - o->shown))) {
            size_t len = (size_t) (nl - (o->buf + o->shown)) + 1;
            put_line(o->buf + o->shown, len);
            o->shown += len;
        }
        if (o->state == PO_LOST && o->shown < o->len) {
            put_line(o->buf + o->shown, o->len - o->shown);
            o->shown = o->len;
        }
        if (o->state != PO_DONE) {
            break;
        }
        free(o->buf);
        o->buf = NULL;
    }
    return k;
}

static void take(struct project_out *o, const char *data, size_t n) {
    if (o->len + n > o->cap) {
        o->cap = (o->len + n) * 2;
        o->buf = realloc(o->buf, o->cap);
        if (!o->buf) {
            die("out of memory", NULL);
        }
    }
    memcpy(o->buf + o->len, data, n);
    o->len += n;
}

static void run_parallel(const struct strlist *snaps) {
    int n = snaps->n, nw = opt_jobs < n ? opt_jobs : n, i, j, next = 0, printed = 0, live = 0, lost = -1, lost_by = -1;
    int told = 0;
    struct project_out *p = calloc((size_t) n, sizeof *p);
    struct worker_proc *ws = calloc((size_t) nw, sizeof *ws);
    struct pollfd *fds = calloc((size_t) nw, sizeof *fds);
    char chunk[65536];
    struct sigaction ign, old_pipe;

    if (!p || !ws || !fds) {
        die("out of memory", NULL);
    }
    pathf(debug_lock, "%s/debug.lock", tmp);
    fflush(stdout);
    fflush(stderr);
    for (i = 0; i < nw; i++) {
        int out[2], cmd[2];
        if (pipe(out) < 0 || pipe(cmd) < 0) {
            die(fmt("cannot make a pipe: %s", strerror(errno)), NULL);
        }
        ws[i].pid = fork();
        if (ws[i].pid < 0) {
            die(fmt("cannot fork: %s", strerror(errno)), NULL);
        }
        if (ws[i].pid == 0) {
            for (j = 0; j < i; j++) {
                close(ws[j].out);
                close(ws[j].cmd);
            }
            close(out[0]);
            close(cmd[1]);
            worker(snaps, cmd[0], out[1]);
        }
        close(out[1]);
        close(cmd[0]);
        ws[i].out = out[0];
        ws[i].cmd = cmd[1];
        ws[i].at = -1;
        live++;
    }
    /* A worker gone, its pipe with it, is noticed by its output's end,
     * not by a SIGPIPE. */
    memset(&ign, 0, sizeof ign);
    ign.sa_handler = SIG_IGN;
    sigemptyset(&ign.sa_mask);
    sigaction(SIGPIPE, &ign, &old_pipe);
    for (i = 0; i < nw && next < n; i++) {
        next += give(&ws[i], p, next);
    }

    while (live > 0) {
        if (fs_interrupted && !told) {
            /* Every worker is told, of a signal from elsewhere than the
             * terminal too, and their output read to its end. */
            told = 1;
            next = n;
            for (i = 0; i < nw; i++) {
                no_more(&ws[i]);
                kill(ws[i].pid, fs_interrupted);
            }
        }
        for (i = 0; i < nw; i++) {
            fds[i].fd = ws[i].out;
            fds[i].events = POLLIN;
            fds[i].revents = 0;
        }
        if (poll(fds, (nfds_t) nw, -1) < 0) {
            if (errno == EINTR) {
                continue;
            }
            die(fmt("cannot wait for the workers: %s", strerror(errno)), NULL);
        }
        for (i = 0; i < nw; i++) {
            struct worker_proc *w = &ws[i];
            struct project_out *o;
            ssize_t got;
            if (w->out < 0 || !fds[i].revents) {
                continue;
            }
            got = read(w->out, chunk, sizeof chunk);
            if (got < 0 && errno == EINTR) {
                continue;
            }
            if (got <= 0) {
                close(w->out);
                w->out = -1;
                live--;
                no_more(w);
                if (w->at >= 0) {
                    /* Died in a project: nothing more is begun. */
                    p[w->at].state = PO_LOST;
                    if (lost < 0 || w->at < lost) {
                        lost = w->at;
                        lost_by = i;
                    }
                    w->at = -1;
                    next = n;
                    for (j = 0; j < nw; j++) {
                        if (ws[j].at < 0) {
                            no_more(&ws[j]);
                        }
                    }
                }
                continue;
            }
            if (w->at < 0) {
                /* Said between projects, which nothing does: as it comes. */
                struct project_out stray = { 0, 0, 0, 0, 0, PO_LOST };
                take(&stray, chunk, (size_t) got);
                put_ready(&stray, 1, 0);
                free(stray.buf);
                continue;
            }
            o = &p[w->at];
            take(o, chunk, (size_t) got);
            while (o->scanned < o->len) {
                char *line = o->buf + o->scanned, *nl = memchr(line, '\n', o->len - o->scanned);
                if (!nl) {
                    break;
                }
                o->scanned += (size_t) (nl - line) + 1;
                if (*line == MARK_DONE) {
                    int *c = w->counts;
                    sscanf(line + 1, "%d %d %d %d %d %d %d %d", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7]);
                    o->state = PO_DONE;
                    w->at = -1;
                    if (next < n && give(w, p, next)) {
                        next++;
                    } else {
                        no_more(w);
                    }
                    break;
                }
            }
        }
        printed = put_ready(p, n, printed);
    }
    for (i = 0; i < nw; i++) {
        while (waitpid(ws[i].pid, &ws[i].status, 0) < 0 && errno == EINTR) {
            ;
        }
    }
    sigaction(SIGPIPE, &old_pipe, NULL);
    put_ready(p, n, printed);
    fflush(stdout);
    bail_if_interrupted();
    for (i = j = 0; lost < 0 && i < n; i++) {
        j += p[i].state != PO_DONE;
    }
    if (j) {
        die(fmt("%d of the projects were not done: the workers were gone", j), NULL);
    }
    if (lost >= 0) {
        int st = ws[lost_by].status;
        exit(WIFEXITED(st) && WEXITSTATUS(st) != 0 ? WEXITSTATUS(st) : 2);
    }
    for (i = 0; i < nw; i++) {
        n_cases += ws[i].counts[0];
        n_runs += ws[i].counts[1];
        n_ok += ws[i].counts[2];
        n_bad += ws[i].counts[3];
        n_changed += ws[i].counts[4];
        n_skipped += ws[i].counts[5];
        n_nonzero += ws[i].counts[6];
        report_failed |= ws[i].counts[7];
    }
    for (i = 0; i < n; i++) {
        free(p[i].buf);
    }
    free(p);
    free(ws);
    free(fds);
}

/* A plain run, --whine or --list over the PATHs, or over "." with none:
 * what the main modes do, and what --create and --edit offer at the end.
 * Returns the exit status. */
static int run_all(int argc, char **argv) {
    static char *dot[] = { "." };
    struct strlist snaps = { 0, 0, 0 };
    char all[PATHLEN] = "";
    int i;

    if (!opt_list && !can_exec(urweb)) {
        die(fmt("no compiler at %s", path(urweb)),
            fmt("build one with %s, or set %s to name another", cmd("make"), key("URWEB")), NULL);
    }
    if (argc == 0) {
        argc = 1;
        argv = dot;
    }
    find_cases(argc, argv, &snaps);
    if (snaps.n == 0) {
        for (i = 0; i < argc; i++) {
            snprintf(all + strlen(all), sizeof all - strlen(all), "%s%s", i ? " " : "", argv[i]);
        }
        warn(fmt("no snapshots under %s", path(all)));
        sl_free(&snaps);
        return 0;
    }
    if (!opt_list) {
        const char *with_runs = first_with_runs(&snaps);
        if (with_runs && (!on_path("curl") || !on_path("sqlite3"))) {
            die(fmt("%s has runs, and they need %s and %s on the PATH", test(with_runs), cmd("curl"), cmd("sqlite3")),
                "the one to talk to the application, the other to load and dump its database", NULL);
        }
        need_tmp();
    }
    started = tenths_now();
    if (opt_jobs > 1 && !opt_list && snaps.n > 1) {
        run_parallel(&snaps);
    } else {
        for (i = 0; i < snaps.n; i++) {
            handle(snaps.v[i], do_case);
            bail_if_interrupted();
        }
    }
    if (!opt_list) {
        summary();
    }
    sl_free(&snaps);
    return report_failed;
}

/* ---------------------------------------------------------------- screens */

/* The tracked set of a case as flags, one per artifact, by whether its
 * file is under expected/. */
static void tracked_set(const char *case_dir, int on[]) {
    char f[PATHLEN];
    int i;
    for (i = 0; i < NARTIFACTS; i++) {
        pathf(f, "%s/expected/%s", case_dir, artifacts[i].name);
        on[i] = is_file(f);
    }
}

/* Which of several tests, from a list that says how much each tracks.
 * 0 when the user leaves. */
static int choose_case(const struct strlist *cases, char *chosen) {
    struct list l;
    struct strlist notes = { 0, 0, 0 };
    int on[NARTIFACTS], i, j, n, r = 0;
    enum action a;
    for (i = 0; i < cases->n; i++) {
        tracked_set(cases->v[i], on);
        for (n = 0, j = 0; j < NARTIFACTS; j++) {
            n += on[j];
        }
        sl_push(&notes, fmt("%d of %d tracked", n, NARTIFACTS));
    }
    list_init(&l, (const char *const *) cases->v, (const char *const *) notes.v, cases->n);
    for (;;) {
        static const struct hint hints[] = { { A_SCROLL, "scroll" }, { A_CONFIRM, "edit" }, { A_BACK, "quit" }, { A_NONE, 0 } };
        tui_line("%s", "");
        tui_line("Which test would you like to edit?");
        tui_line("%s", "");
        list_render(&l, NULL);
        tui_footer(M_LIST, hints);
        tui_draw();
        a = tui_next(M_LIST, NULL);
        if (a == A_CONFIRM) {
            snprintf(chosen, PATHLEN, "%s", cases->v[l.cur]);
            r = 1;
            break;
        }
        if (a == A_BACK || a == A_QUIT) {
            break;
        }
        list_update(&l, a);
    }
    sl_free(&notes);
    return r;
}

/* The checklist of artifacts under a title, starting from on.  1 when
 * saved, with the new set in on; 0 when the user went back; -1 when
 * they left. */
static int set_screen(const char *title, int on[], int can_go_back) {
    static const char *names[NARTIFACTS];
    struct check c;
    int i, r;
    enum action a;
    for (i = 0; i < NARTIFACTS; i++) {
        names[i] = artifacts[i].name;
    }
    check_init(&c, names, NARTIFACTS, on);
    for (;;) {
        struct hint hints[] = { { A_SCROLL, "scroll" }, { A_TOGGLE, "" }, { A_SAVE, "save" },
                                { A_BACK, can_go_back ? "back" : "quit" }, { A_TOGGLE_ALL, "" }, { A_NONE, 0 } };
        hints[1].meaning = c.on[c.l.cur] ? "deselect" : "select";
        hints[4].meaning = check_all_label(&c);
        tui_line("%s", "");
        tui_line("%s", title);
        tui_line("%s", "");
        check_render(&c);
        tui_footer(M_CHECK, hints);
        tui_draw();
        a = tui_next(M_CHECK, NULL);
        if (a == A_SAVE) {
            memcpy(on, c.on, NARTIFACTS * sizeof on[0]);
            r = 1;
            break;
        }
        if (a == A_BACK) {
            r = can_go_back ? 0 : -1;
            break;
        }
        if (a == A_QUIT) {
            r = -1;
            break;
        }
        check_update(&c, a);
    }
    return r;
}

/* Whether to run the tests now, to fill in what they track: a yes or no.
 * The last frame stays on the screen, under whatever was printed. */
static int offer_run(const struct strlist *cases) {
    static const char *const items[] = { "yes", "no, later" };
    struct list l;
    char command[PATHLEN];
    int i, yes = 0;
    enum action a;
    snprintf(command, sizeof command, "%s", me);
    for (i = 0; i < cases->n; i++) {
        snprintf(command + strlen(command), sizeof command - strlen(command), " %s", cases->v[i]);
    }
    list_init(&l, items, NULL, 2);
    for (;;) {
        static const struct hint hints[] = { { A_SCROLL, "scroll" }, { A_CONFIRM, "choose" }, { A_BACK, "no" }, { A_NONE, 0 } };
        tui_line("%s", "");
        tui_line("Run %s%s%s now, to fill in what is tracked?", TUI_BOLD, command, TUI_RESET);
        tui_line("%s", "");
        list_render(&l, NULL);
        tui_footer(M_LIST, hints);
        tui_draw();
        a = tui_next(M_LIST, NULL);
        if (a == A_CONFIRM) {
            yes = l.cur == 0;
            break;
        }
        if (a == A_BACK || a == A_QUIT) {
            break;
        }
        list_update(&l, a);
    }
    tui_break();
    return yes;
}

/* How to fill in what is newly tracked, when that is left for later. */
static void run_hint(const struct strlist *cases) {
    char command[PATHLEN];
    int i;
    snprintf(command, sizeof command, "%s", me);
    for (i = 0; i < cases->n; i++) {
        snprintf(command + strlen(command), sizeof command - strlen(command), " %s", cases->v[i]);
    }
    printf("%s\n", info(fmt("what is newly tracked starts empty; run %s to fill it in", cmd(command))));
}

/* ------------------------------------------------------------ --create */

/* The only .urp file in the current directory names the project.  With
 * none or several, the caller has to say. */
static void only_project_here(char *project) {
    struct strlist names = { 0, 0, 0 };
    char here[PATHLEN], found[PATHLEN] = "";
    int i, count = 0;
    list_dir(".", LS_ALL, &names);
    for (i = 0; i < names.n; i++) {
        if (names.v[i][0] == '.' || !has_suffix(names.v[i], ".urp")) {
            continue;
        }
        snprintf(found, sizeof found, "%s", names.v[i]);
        count++;
    }
    sl_free(&names);
    if (count != 1) {
        if (!realpath(".", here)) {
            strcpy(here, ".");
        }
        if (count == 0) {
            die(fmt("no .urp file in %s", path(here)),
                fmt("name the project: %s", cmd(fmt("%s --create PROJECT", me))), NULL);
        }
        die(fmt("%d .urp files in %s", count, path(here)),
            fmt("name the one you mean: %s", cmd(fmt("%s --create PROJECT", me))), NULL);
    }
    strip_suffix(found, ".urp");
    strcpy(project, found);
}

/* The next number for a case (or a run) under snap: one more than the
 * highest of the names there that are numbers, or 1.  A name that is a
 * number too large for a long is not counted as one; the largest long is
 * refused as a name. */
static long next_number(const char *snap) {
    struct strlist names = { 0, 0, 0 };
    long highest = 0;
    int i;
    list_dir(snap, LS_DIRS, &names);
    for (i = 0; i < names.n; i++) {
        char *end;
        long n;
        errno = 0;
        n = strtol(names.v[i], &end, 10);
        if (*names.v[i] && !*end && errno == 0 && n > highest) {
            highest = n;
        }
    }
    sl_free(&names);
    if (highest == LONG_MAX) {
        die(fmt("%s is in the way of the next number", path(fmt("%s/%ld", snap, highest))),
            "there is no number after it: name the new one", NULL);
    }
    return highest + 1;
}

/* Set up cases for a project under PROJECT.urt/snapshot, named as given
 * or by the next number: an empty args and an expected/ of empty
 * artifacts each -- all of them, or on a terminal the ones ticked in a
 * checklist, with a run offered at the end to fill them in. */
static int create(int argc, char **argv) {
    char project[PATHLEN], snap[PATHLEN], d[PATHLEN], f[PATHLEN];
    struct strlist cases = { 0, 0, 0 };
    int (*sets)[NARTIFACTS];
    int i, a, interactive, tracked, status = 0;

    if (argc > 0) {
        snprintf(project, sizeof project, "%s", argv[0]);
        argc--;
        argv++;
    } else {
        *project = 0;
    }
    if (!*project) {
        only_project_here(project);
    }
    strip_suffix(project, "/");
    strip_suffix(project, "/snapshot");
    strip_suffix(project, ".urt");
    strip_suffix(project, ".urp");

    if (!is_file(fmt("%s.urp", project)) && !is_file(fmt("%s.ur", project))) {
        die(fmt("no project called %s", path(project)),
            fmt("expected %s or %s to be there", path(fmt("%s.urp", project)), path(fmt("%s.ur", project))), NULL);
    }

    pathf(snap, "%s.urt/snapshot", project);
    if (exists(snap) && !is_dir(snap)) {
        die(fmt("%s is in the way, and it is not a directory", path(snap)), NULL);
    }
    join(d, snap, "expected");
    if (is_dir(d)) {
        die(fmt("%s is not in a case", path(d)),
            "every snapshot is a named case now: move args and expected/ into one",
            fmt("such as %s", path(fmt("%s/1", snap))), NULL);
    }

    if (argc == 0) {
        pathf(d, "%s/%ld", snap, next_number(snap));
        sl_push(&cases, d);
    } else {
        for (i = 0; i < argc; i++) {
            char c[PATHLEN];
            snprintf(c, sizeof c, "%s", argv[i]);
            strip_suffix(c, "/");
            if (!valid_case_name(c)) {
                die(fmt("'%s' cannot name a case", c),
                    "a case name is letters, digits, _ and -, and nothing else, not starting with -", NULL);
            }
            if (!strcmp(c, "expected") || !strcmp(c, "args") || !strcmp(c, "README.md")) {
                die(fmt("'%s' cannot name a case", c),
                    fmt("%s is part of the layout; call the case something else", path(fmt("%s/%s", snap, c))), NULL);
            }
            join(d, snap, c);
            if (exists(d)) {
                die(fmt("%s is in the way", path(d)), "delete it first if you meant to start again", NULL);
            }
            sl_push(&cases, d);
        }
    }

    /* What each case will track: everything, unless there is a terminal
     * to ask on.  Nothing is written until every checklist is saved, so
     * leaving one leaves the tree as it was. */
    sets = xmalloc((size_t) cases.n * sizeof *sets);
    for (i = 0; i < cases.n; i++) {
        for (a = 0; a < NARTIFACTS; a++) {
            sets[i][a] = 1;
        }
    }
    interactive = tui_open() == 0;
    if (interactive) {
        for (i = 0; i < cases.n; i++) {
            int r = set_screen(fmt("Defining the tracked set of %s%s%s.", TUI_BOLD, cases.v[i], TUI_RESET), sets[i], i > 0);
            if (r < 0) {
                tui_close();
                printf("%s\n", info("nothing created"));
                free(sets);
                sl_free(&cases);
                return 0;
            }
            if (r == 0) {
                i -= 2; /* back: the previous case again */
            }
        }
        tui_break();
    }

    for (i = 0, tracked = 0; i < cases.n; i++) {
        int n = 0;
        join(d, cases.v[i], "expected");
        if (mkdir_p(d) < 0) {
            die(fmt("cannot make %s: %s", path(d), strerror(errno)), NULL);
        }
        join(f, cases.v[i], "args");
        if (write_text(f, "") < 0) {
            die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
        }
        for (a = 0; a < NARTIFACTS; a++) {
            if (!sets[i][a]) {
                continue;
            }
            join(f, d, artifacts[a].name);
            if (write_text(f, "") < 0) {
                die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
            }
            n++;
        }
        say(ST_NEW, "CREATED", cases.v[i],
            info(n ? fmt(" (args, and %d empty artifact%s under expected/)", n, n == 1 ? "" : "s")
                   : " (args, and an empty expected/)"));
        tracked += n;
    }
    free(sets);

    if (interactive) {
        if (tracked && offer_run(&cases)) {
            tui_close();
            status = run_all(cases.n, cases.v);
        } else {
            tui_close();
            if (tracked) {
                run_hint(&cases);
            }
        }
    } else {
        printf("%s\n", info(fmt("delete what this project has no opinion about, then run %s to fill in the rest", cmd(me))));
    }
    sl_free(&cases);
    return status;
}

/* ---------------------------------------------------------------- --edit */

/* Make the tracked set of a case be `on`: an artifact newly tracked starts
 * as an empty file, for a run to fill in; one dropped loses its file.
 * Says what changed, and returns how many are new. */
static int apply_set(const char *case_dir, const int on[]) {
    char f[PATHLEN], changes[PATHLEN] = "";
    int was[NARTIFACTS], i, added = 0;
    tracked_set(case_dir, was);
    for (i = 0; i < NARTIFACTS; i++) {
        if (on[i] == was[i]) {
            continue;
        }
        pathf(f, "%s/expected/%s", case_dir, artifacts[i].name);
        if (on[i]) {
            if (write_text(f, "") < 0) {
                die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
            }
            added++;
        } else if (unlink(f) < 0) {
            die(fmt("cannot delete %s: %s", path(f), strerror(errno)), NULL);
        }
        list_add(changes, sizeof changes, fmt("%c%s", on[i] ? '+' : '-', artifacts[i].name));
    }
    if (!*changes) {
        say(ST_OK, "INFO", case_dir, info(" (the tracked set remains unchanged)"));
    } else {
        say(ST_NEW, "EDITED", case_dir, fmt(" (%s)", changes));
    }
    return added;
}

/* The cases handle() found, for --edit to pick from, and their projects. */
static struct strlist collected, collected_projects;

static void collect_case(const char *case_dir, const char *project) {
    sl_push(&collected, case_dir);
    sl_push(&collected_projects, project);
}

/* --edit [PATH] [ARTIFACT...]: the tracked set of one test, chosen from a
 * list when PATH holds several, ticked in a checklist, then filled in on
 * request -- or set outright to the ARTIFACTs named, which takes no
 * terminal. */
static int edit(int argc, char **argv) {
    static char *dot[] = { "." };
    struct strlist snaps = { 0, 0, 0 }, one = { 0, 0, 0 };
    char chosen[PATHLEN];
    int on[NARTIFACTS], i, r = 0, added, status = 0;

    find_cases(1, argc ? argv : dot, &snaps);
    for (i = 0; i < snaps.n; i++) {
        handle(snaps.v[i], collect_case);
    }
    sl_free(&snaps);
    if (report_failed) {                    /* put the tests right first */
        sl_free(&collected);
        return 1;
    }
    if (collected.n == 0) {
        die(fmt("nothing to edit at %s", path(argc ? argv[0] : ".")), NULL);
    }

    if (argc > 1) {
        /* Named outright. */
        if (collected.n != 1) {
            die(fmt("%s holds %d tests; name the one to edit", test(argv[0]), collected.n), NULL);
        }
        memset(on, 0, sizeof on);
        for (i = 1; i < argc; i++) {
            const struct artifact *a = artifact_named(argv[i]);
            if (!a) {
                die(fmt("%s is not something the compiler produces", argv[i]),
                    fmt("the artifacts are: %s", artifact_names()), NULL);
            }
            on[a - artifacts] = 1;
        }
        sl_push(&one, collected.v[0]);
        if (apply_set(collected.v[0], on)) {
            run_hint(&one);
        }
    } else {
        if (tui_open() < 0) {
            die(fmt("%s needs a terminal", key("--edit")),
                fmt("or the artifacts named outright: %s", cmd(fmt("%s --edit TEST ARTIFACT...", me))), NULL);
        }
        snprintf(chosen, sizeof chosen, "%s", collected.v[0]);
        for (;;) {
            if (collected.n > 1 && !choose_case(&collected, chosen)) {
                r = -1;
                break;
            }
            tracked_set(chosen, on);
            r = set_screen(fmt("Editing the tracked set of %s%s%s.", TUI_BOLD, chosen, TUI_RESET), on, collected.n > 1);
            if (r != 0) {
                break; /* saved, or left; 0 is back to the list */
            }
        }
        bail_if_interrupted();
        if (r < 0) {
            tui_close();
            printf("%s\n", info("nothing changed"));
        } else {
            tui_break();
            added = apply_set(chosen, on);
            sl_push(&one, chosen);
            if (added && offer_run(&one)) {
                tui_close();
                status = run_all(1, one.v);
            } else {
                tui_close();
                if (added) {
                    run_hint(&one);
                }
            }
        }
    }
    sl_free(&one);
    sl_free(&collected);
    sl_free(&collected_projects);
    return status;
}

/* ------------------------------------------------------------ --create-run */

/* The schema of a case's application, as the compiler writes it for the
 * case's args, into the file named; 0 if the compiler would not. */
static int schema_of(const char *case_dir, const char *project, const char *schema) {
    struct strlist argv = { 0, 0, 0 }, extra = { 0, 0, 0 };
    char f[PATHLEN], **command, *args;
    int i, status, own_stop = 0;

    sl_push(&argv, urweb);
    for (i = 0; i < urweb_flags.n; i++) {
        sl_push(&argv, urweb_flags.v[i]);
    }
    sl_push(&argv, "-sql");
    sl_push(&argv, schema);
    join(f, case_dir, "args");
    args = is_file(f) ? read_text(f) : NULL;
    if (args) {
        split_words(args, &extra);
    }
    free(args);
    for (i = 0; i < extra.n; i++) {
        own_stop |= !strcmp(extra.v[i], "-stop") || !strcmp(extra.v[i], "-tc");
    }
    if (!own_stop) {
        sl_push(&argv, "-stop");
        sl_push(&argv, artifact_named("schema.sql")->phase);
        sl_push(&argv, "-quiet");
    }
    for (i = 0; i < extra.n; i++) {
        sl_push(&argv, extra.v[i]);
    }
    sl_push(&argv, project);
    command = sl_argv(&argv);
    status = run(command, "/dev/null", "/dev/null");
    free(command);
    sl_free(&argv);
    sl_free(&extra);
    bail_if_interrupted();
    return status == 0 && is_file(schema);
}

/* --create-run [PATH] [RUN...]: set up runs for the one case PATH names,
 * called as given or by the next number.  Each gets what a run cannot do
 * without -- a secret, and the schema as db.sql if the application has
 * tables -- an empty args, and a first request to be replaced.
 * --create-browser-run does the same under browser/, with a first step. */
static int create_run(int argc, char **argv, enum run_kind kind) {
    const char *first = kind == RUN_BROWSER ? "01.step" : "01.request";
    static char *dot[] = { "." };
    struct strlist snaps = { 0, 0, 0 }, runs = { 0, 0, 0 };
    char rr[PATHLEN], d[PATHLEN], f[PATHLEN], schema[PATHLEN];
    char *text;
    int i, has_tables;

    find_cases(1, argc ? argv : dot, &snaps);
    for (i = 0; i < snaps.n; i++) {
        handle(snaps.v[i], collect_case);
    }
    sl_free(&snaps);
    if (report_failed) {                    /* put the tests right first */
        return 1;
    }
    if (collected.n != 1) {
        die(fmt("%s holds %d cases; name the one the run is for", test(argc ? argv[0] : "."), collected.n),
            fmt("as in %s", cmd(fmt("%s --create-%srun PROJECT.urt/snapshot/CASE", me, kind == RUN_BROWSER ? "browser-" : ""))), NULL);
    }

    join(rr, collected.v[0], run_dirname[kind]);
    if (exists(rr) && !is_dir(rr)) {
        die(fmt("%s is in the way, and it is not a directory", path(rr)), NULL);
    }
    if (argc <= 1) {
        pathf(d, "%s/%ld", rr, next_number(rr));
        sl_push(&runs, d);
    }
    for (i = 1; i < argc; i++) {
        char r[PATHLEN];
        snprintf(r, sizeof r, "%s", argv[i]);
        strip_suffix(r, "/");
        if (!valid_case_name(r) || !strcmp(r, "README.md")) {
            die(fmt("'%s' cannot name a run", r), "the name of a run is letters, digits, _ and -, and nothing else, not starting with -", NULL);
        }
        join(d, rr, r);
        if (exists(d)) {
            die(fmt("%s is in the way", path(d)), "delete it first if you meant to start again", NULL);
        }
        sl_push(&runs, d);
    }

    /* The schema, if the compiler can be asked and the program has one:
     * SQLite's always sets two pragmas, which alone make no database. */
    has_tables = 0;
    if (can_exec(urweb)) {
        need_tmp();
        rm_rf(work);
        if (mkdir_p(work) < 0) {
            die(fmt("cannot make %s", path(work)), NULL);
        }
        join(schema, work, "schema.sql");
        if (schema_of(collected.v[0], collected_projects.v[0], schema) && (text = read_text(schema))) {
            has_tables = strstr(text, "CREATE ") != NULL;
            free(text);
        } else {
            warn(fmt("%s does not compile as %s has it, so there is no schema to start db.sql from",
                     path(collected_projects.v[0]), test(collected.v[0])));
        }
    } else {
        warn(fmt("no compiler at %s, so there is no schema to start db.sql from", path(urweb)));
    }

    for (i = 0; i < runs.n; i++) {
        join(d, runs.v[i], "interaction");
        if (mkdir_p(d) < 0) {
            die(fmt("cannot make %s: %s", path(d), strerror(errno)), NULL);
        }
        join(f, runs.v[i], "args");
        if (write_text(f, "") < 0) {
            die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
        }
        join(f, runs.v[i], "secret");
        if (write_text(f, "secret signature\n") < 0) {
            die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
        }
        join(f, runs.v[i], "db.sql");
        if (has_tables && copy_file(schema, f) < 0) {
            die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
        }
        join(f, d, first);
        if (write_text(f, kind == RUN_BROWSER ? "user: open / [target=1]\n" : "GET /\n") < 0) {
            die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
        }
        say(ST_NEW, "CREATED", runs.v[i],
            info(fmt(" (args, secret, %sinteraction/%s)", has_tables ? "db.sql, " : "", first)));
    }
    printf("%s\n", info(fmt("write the %s under interaction/, then run %s to fill in the rest",
                            kind == RUN_BROWSER ? "steps" : "requests", cmd(fmt("%s %s", me, collected.v[0])))));
    sl_free(&runs);
    sl_free(&collected);
    sl_free(&collected_projects);
    return 0;
}

/* ------------------------------------------------------------------ main */

/* Whether s is a count: digits, at least one. */
static int is_count(const char *s) {
    if (!*s) {
        return 0;
    }
    for (; *s; s++) {
        if (!isdigit((unsigned char) *s)) {
            return 0;
        }
    }
    return 1;
}

/* -j's count: how many projects at once. */
static int jobs_of(const char *s) {
    long n = 0;
    if (is_count(s)) {
        errno = 0;
        n = strtol(s, NULL, 10);
        if (errno || n > INT_MAX) {
            n = 0;
        }
    }
    if (n < 1) {
        die(fmt("%s takes how many projects to go at once, a number above 0, not %s", key("-j"), key(s)),
            fmt("%s alone takes as many as there are cores", key("-j")), NULL);
    }
    return (int) n;
}

/* The cores we may run on: -j alone. */
static int cores(void) {
    long n = sysconf(_SC_NPROCESSORS_ONLN);
    return n > 0 && n < INT_MAX ? (int) n : 1;
}

int snapshot_main(int argc, char **argv) {
    const char *env;
    int i;

    fs_catch_signals();
    atexit(cleanup);

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--whine") || !strcmp(a, "-w")) {
            opt_check = 1;
        } else if (!strcmp(a, "--screenshot")) {
            opt_screenshot = 1;
        } else if (!strcmp(a, "--record")) {
            opt_record = 1;
        } else if (!strncmp(a, "--record=", 9)) {
            long long ms;
            if (parse_duration(a + 9, &ms) < 0 || ms < 1) {
                die(fmt("%s takes the time between a film's frames, a number and its unit, not %s", key("--record=T"),
                        key(a + 9)), "as in --record=16ms (the default) or --record=33ms", NULL);
            }
            opt_record = 1;
            opt_frame = a + 9;
        } else if (!strcmp(a, "--create") || !strcmp(a, "-c")) {
            opt_create = 1;
            i++;
            break;
        } else if (!strcmp(a, "--create-run")) {
            opt_create_run = 1 + RUN_REQUESTS;
            i++;
            break;
        } else if (!strcmp(a, "--create-browser-run")) {
            opt_create_run = 1 + RUN_BROWSER;
            i++;
            break;
        } else if (!strcmp(a, "--edit") || !strcmp(a, "-e")) {
            opt_edit = 1;
            i++;
            break;
        } else if (!strcmp(a, "-j") || !strcmp(a, "--jobs")) {
            /* Alone, a project per core; a count may follow. */
            opt_jobs_given = 1;
            opt_jobs = i + 1 < argc && is_count(argv[i + 1]) ? jobs_of(argv[++i]) : cores();
        } else if ((!strncmp(a, "-j", 2) && a[2] != 0) || !strncmp(a, "--jobs=", 7)) {
            opt_jobs_given = 1;
            opt_jobs = jobs_of(a + (a[1] == 'j' ? 2 : 7));
        } else if (!strcmp(a, "--list") || !strcmp(a, "-l")) {
            opt_list = 1;
        } else if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
            fputs(usage_text, stdout);
            return 0;
        } else if (!strcmp(a, "--")) {
            i++;
            break;
        } else if (a[0] == '-') {
            die(fmt("unrecognised option %s", key(a)),
                fmt("usage: %s", cmd(fmt("%s [--whine|-w | --list|-l] [--screenshot] [--record[=T]] [-j[N]] [PATH...]", me))),
                fmt("       %s", cmd(fmt("%s (--create|-c) [PROJECT] [CASE...]", me))),
                fmt("       %s", cmd(fmt("%s --create-run [PATH] [RUN...]", me))),
                fmt("       %s", cmd(fmt("%s --create-browser-run [PATH] [RUN...]", me))),
                fmt("       %s", cmd(fmt("%s (--edit|-e) [PATH] [ARTIFACT...]", me))), NULL);
        } else {
            break;
        }
    }
    argc -= i;
    argv += i;
    if ((opt_create || opt_create_run || opt_edit) && (opt_check || opt_list || opt_screenshot || opt_record || opt_jobs_given)) {
        die(fmt("%s goes with neither %s, %s, %s, %s nor %s",
                key(opt_create ? "--create" : opt_edit ? "--edit"
                    : opt_create_run == 1 + RUN_BROWSER ? "--create-browser-run" : "--create-run"),
                key("--whine"), key("--list"), key("--screenshot"), key("--record"), key("-j")), NULL);
    }

    env = getenv("URWEB");
    find_compiler(env && *env ? env : NULL);

    /* The compiler stamps what it reads into what it writes -- the
     * generated code serves its static files with a Last-Modified taken
     * from the newest source it read -- so pin that.  Outright, rather
     * than deferring to a value already in the environment, because
     * nix-shell exports one of its own and the snapshots would then
     * depend on being inside one. */
    setenv("SOURCE_DATE_EPOCH", "0", 1);

    if (opt_create) {
        return create(argc, argv);
    }
    if (opt_edit) {
        return edit(argc, argv);
    }
    if (opt_create_run) {
        return create_run(argc, argv, opt_create_run - 1);
    }
    i = run_all(argc, argv);
    sl_free(&urweb_flags);
    return i;
}
