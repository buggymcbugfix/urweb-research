/* snapshot.c -- urt snapshot: snapshot tests for the Ur/Web compiler.
 * Run it on a project and keep its output and some intermediate state as
 * files, so that a change to the compiler shows up as a diff.  `urt
 * snapshot --help` prints the layout and the usage; usage_text below is
 * that.
 *
 * Held to the transcripts under urt/tests/snapshot by urt/check, as far
 * as a stand-in compiler goes; the runs of an application, which take a
 * real one, by the cases under tests/reproducible with the rest of the
 * compiler's own snapshots (make check-snapshot).
 */

/* POSIX 2008 with the XSI extensions, and on macOS the BSD ones too,
 * which its headers hide once a strict level is asked for (mkdtemp is
 * one).  _DARWIN_C_SOURCE means nothing anywhere else. */
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    "                  |-- secret              the key it signs with: 16 bytes or more\n"
    "                  |-- db.sql              the database it starts from, if it uses one\n"
    "                  |-- interaction/\n"
    "                  |   |-- 01-add.request  what to send\n"
    "                  |   |-- 01-add.response what came back, as `curl -i` prints it\n"
    "                  |   |-- 01-add.sql      what that changed in the database, if anything\n"
    "                  |   |-- 01-add.stdout   what the server printed meanwhile, if anything\n"
    "                  |   |-- 01-add.stderr\n"
    "                  |   |-- 02-pay.request\n"
    "                  |   `-- 02-pay.response\n"
    "                  `-- out/\n"
    "                      |-- startup.sql     what the application did before the first\n"
    "                      |-- startup.stdout  request (task initialize), if anything\n"
    "                      |-- startup.stderr\n"
    "                      |-- shutdown.stdout what it printed after the last, if anything\n"
    "                      |-- shutdown.stderr\n"
    "                      `-- db_final.sql    the database it left behind\n"
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
    "What the server prints for a request goes beside its response, and only if\n"
    "there is any: its own lines about connections and requests are dropped, the\n"
    "program's debug output and the runtime's errors are kept.\n"
    "\n"
    "Runs share nothing.  Each has its own database -- SQLite, so the case's args\n"
    "need -dbms sqlite -- loaded from db.sql, which holds the schema and any rows\n"
    "to start with; within a run, the requests build on each other.  So that the\n"
    "same requests get the same answers, the application's clock is pinned (the\n"
    "first request is served at 2000-01-01 00:00:00 UTC, each further one a second\n"
    "later, and the database agrees), as are its random numbers and its time zone,\n"
    "UTC; its heap starts at 256 MiB, so that no handler is run a second time\n"
    "for the heap to grow; every request is for the host 'localhost', from the\n"
    "user agent 'urt'.\n"
    "\n"
    "A case with runs is compiled all the way, so its args cannot hold -stop,\n"
    "-stopQuiet or -tc, and it fails if that does not give an application.  The\n"
    "compiler builds it where the project has it built, and from there it is moved\n"
    "into the case as app.exe, where it stays, to be started by hand if need be.\n"
    "A run's args cannot hold -t: its requests are sent one after the other, and\n"
    "with more threads than one it is a matter of chance which of them answers.\n"
    "Runs need curl and sqlite3 on the PATH.\n"
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
    "  urt snapshot --list [PATH...]              list the cases matching the given path(s)\n"
    "  urt snapshot --create [PROJECT] [CASE...]  create cases for the given project\n"
    "  urt snapshot --create-run [PATH] [RUN...]  create runs for the one case at the given path\n"
    "  urt snapshot --edit [PATH] [ARTIFACT...]   edit the tracked set of a case\n"
    "\n"
    "PATH is a project (demo/alert[.urp]), its urt directory (demo/alert.urt), its\n"
    "snapshots (demo/alert.urt/snapshot), one case (demo/alert.urt/snapshot/1) or a\n"
    "directory to search for snapshots.  It defaults to the current directory.\n"
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
    "replace.  A plain run fills in the rest.\n"
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

static int opt_check = 0, opt_create = 0, opt_create_run = 0, opt_list = 0, opt_edit = 0;
static const char *me = "urt snapshot";   /* how to invoke us, for messages */
static char urweb[PATHLEN];               /* the compiler, absolute */
static struct strlist urweb_flags;        /* what it always gets */
static char tmp[PATHLEN] = "";            /* our scratch directory */
static char work[PATHLEN];                /* where a run's artifacts land */
static int diff_labels = 0;               /* GNU diff, which names the sides for us */

static int n_cases = 0, n_runs = 0, n_ok = 0, n_bad = 0, n_changed = 0, n_skipped = 0, n_nonzero = 0;
static long started;
static struct server serving;             /* the application now running, if any */

/* The scratch directory goes with us, whichever way we leave, and so
 * does a server we started. */
static void cleanup(void) {
    server_stop(&serving);
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

/* A case name is letters, digits, _ and -, and nothing else. */
static int valid_case_name(const char *s) {
    if (!*s) {
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

/* The same strictness for the runs of a case.  e is the entry, rest what
 * follows CASE/request-response in its path: "" for the directory
 * itself, then /RUN, /RUN/args and so on.  Returns 1 when e is a
 * directory that was refused, for the walk to pass over what it holds. */
static int layout_run(const char *e, const char *rest) {
    char part[4][PATHLEN];
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
                      "the name of a run is letters, digits, _ and -, and nothing else", NULL);
            return 1;
        }
    } else if (n == 2 && (!strcmp(part[1], "args") || !strcmp(part[1], "secret") || !strcmp(part[1], "db.sql"))) {
        if (!is_file(e)) {
            die_later(fmt("%s should be a file", path(e)), NULL);
        }
    } else if (n == 2 && (!strcmp(part[1], "interaction") || !strcmp(part[1], "out"))) {
        if (!is_dir(e)) {
            die_later(fmt("%s should be a directory", path(e)), NULL);
        }
    } else if (n == 3 && !strcmp(part[1], "interaction") && is_file(e) && has_suffix(part[2], ".request")) {
        ;
    } else if (n == 3 && !strcmp(part[1], "interaction") && is_file(e)
               && (has_suffix(part[2], ".response") || has_suffix(part[2], ".sql")
                   || has_suffix(part[2], ".stdout") || has_suffix(part[2], ".stderr"))) {
        char request[PATHLEN + 8];
        snprintf(request, sizeof request, "%s", e);
        strcpy(strrchr(request, '.'), ".request");
        if (!is_file(request)) {
            die_later(fmt("%s answers no request", path(e)),
                      fmt("there is no %s; delete the one, or write the other", path(request)), NULL);
        }
    } else if (n == 3 && !strcmp(part[1], "out") && is_file(e)
               && (!strcmp(part[2], "startup.sql") || !strcmp(part[2], "startup.stdout") || !strcmp(part[2], "startup.stderr")
                   || !strcmp(part[2], "shutdown.stdout") || !strcmp(part[2], "shutdown.stderr")
                   || !strcmp(part[2], "db_final.sql"))) {
        ;
    } else {
        die_later(fmt("%s does not belong in a run", path(e)),
                  "a run holds args, secret, db.sql and README.md; NAME.request, NAME.response, NAME.sql, NAME.stdout",
                  "and NAME.stderr under interaction/; startup.sql, startup.stdout, startup.stderr, shutdown.stdout,",
                  "shutdown.stderr and db_final.sql under out/", NULL);
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
            if (layout_run(e, rr + 17)) {
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
                      "a case holds args, README.md, expected/, request-response/ and app.exe, nothing else",
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
                "a case name is letters, digits, _ and -, and nothing else",
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
 * (NAME.sql) and what the server printed meanwhile (NAME.stdout,
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

/* The runs of a case, as paths, sorted. */
static void runs_of(const char *case_dir, struct strlist *runs) {
    struct strlist names = { 0, 0, 0 };
    char d[PATHLEN], f[PATHLEN];
    int i;
    join(d, case_dir, "request-response");
    if (is_dir(d)) {
        list_dir(d, LS_DIRS, &names);
        for (i = 0; i < names.n; i++) {
            join(f, d, names.v[i]);
            sl_push(runs, f);
        }
    }
    sl_free(&names);
}

/* The flag in a case's args that keeps the compiler from building the
 * application its runs need, or NULL. */
static const char *no_application(const struct strlist *extra) {
    int i;
    for (i = 0; i < extra->n; i++) {
        const char *a = extra->v[i];
        if (!strcmp(a, "-stop") || !strcmp(a, "-stopQuiet") || !strcmp(a, "-tc")) {
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

/* One output of a run, from -> to, with the scratch directory taken out
 * of it as for the artifacts.  A response can be anything, so lines are
 * written by their length, and one that holds a NUL is left as it is.
 * Returns how many bytes were written. */
static long normalise_run(const char *name, const char *from, const char *to) {
    FILE *in = fopen(from, "rb"), *out = fopen(to, "wb");
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    long written = 0;
    int is_stdout = has_suffix(name, ".stdout"), after_banner = 0;
    if (!in || !out) {
        die(fmt("cannot normalise %s: %s", path(from), strerror(errno)), NULL);
    }
    while ((n = getline(&line, &cap, in)) > 0) {
        if (memchr(line, 0, (size_t) n)) {
            fwrite(line, 1, (size_t) n, out);
            written += n;
            continue;
        }
        strip_work(line);
        if (!is_stdout || keep_server_line(line, &after_banner)) {
            fputs(line, out);
            written += (long) strlen(line);
        }
    }
    free(line);
    fclose(in);
    if (fclose(out) != 0) {
        die(fmt("cannot write %s: %s", path(to), strerror(errno)), NULL);
    }
    return written;
}

/* What a stream of the server's holds since the last cut, into a file of
 * its own; nothing, and there is no file.  *at is where the last cut was,
 * and is moved to the end.  The server writes the stream itself, so this
 * reads what is there rather than what it was handed. */
static void cut_output(const char *stream, long *at, const char *to) {
    FILE *in = fopen(stream, "rb"), *out;
    char buf[8192];
    size_t n;
    long end = 0;

    if (!in) {
        die(fmt("cannot read %s: %s", path(stream), strerror(errno)), NULL);
    }
    if (fseek(in, 0, SEEK_END) < 0 || (end = ftell(in)) < 0) {
        die(fmt("cannot read %s: %s", path(stream), strerror(errno)), NULL);
    }
    if (end <= *at) {
        fclose(in);
        return;
    }
    if (fseek(in, *at, SEEK_SET) < 0 || !(out = fopen(to, "wb"))) {
        die(fmt("cannot write %s: %s", path(to), strerror(errno)), NULL);
    }
    while (*at < end && (n = fread(buf, 1, sizeof buf, in)) > 0) {
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

/* The first lines of a file, for an error message. */
static const char *head_of(const char *file) {
    char *text = is_file(file) ? read_text(file) : NULL;
    const char *r;
    size_t n;
    if (!text) {
        return "(it said nothing)";
    }
    n = strlen(text);
    while (n && text[n - 1] == '\n') {
        text[--n] = 0;
    }
    r = fmt("%.600s", *text ? text : "(it said nothing)");
    free(text);
    return r;
}

/* Send one request, the response into a file.  Returns curl's status. */
static int send_request(const char *method, const char *url_path, const struct strlist *options, int port,
                        const char *response) {
    struct strlist argv = { 0, 0, 0 };
    char err[PATHLEN], **command;
    int i, status;

    sl_push(&argv, "curl");
    sl_push(&argv, "-q");               /* no ~/.curlrc; has to come first */
    sl_push(&argv, "-sS");
    sl_push(&argv, "-i");
    sl_push(&argv, "-g");               /* [] and {} in a path are just that */
    sl_push(&argv, "--noproxy");
    sl_push(&argv, "*");
    sl_push(&argv, "--max-time");
    sl_push(&argv, "30");
    sl_push(&argv, "-A");
    sl_push(&argv, "urt");
    sl_push(&argv, "-H");
    sl_push(&argv, "Host: localhost");
    sl_push(&argv, "-X");
    sl_push(&argv, method);
    for (i = 0; i < options->n; i++) {
        sl_push(&argv, options->v[i]);
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
            if (has_prefix(words.v[i], "-t")) {
                die_later(fmt("%s asks for threads, with %s", path(f), key(words.v[i])),
                          "a run's requests are sent one after the other, so one thread serves them all;",
                          "with more, which one answers is a matter of chance, and the server's output with it",
                          fmt("take %s out", key("-t")), NULL);
            }
        }
        sl_free(&words);
        free(args);
    }

    join(interaction, run_dir, "interaction");
    list_dir(interaction, LS_FILES, &files);
    for (i = 0; i < files.n; i++) {
        char method[32], url_path[PATHLEN], err[256], *text;
        struct strlist options = { 0, 0, 0 };
        if (!has_suffix(files.v[i], ".request")) {
            continue;
        }
        join(f, interaction, files.v[i]);
        text = read_text(f);
        if (!text || parse_request(text, method, sizeof method, url_path, sizeof url_path, &options, err, sizeof err) < 0) {
            die_later(fmt("%s: %s", path(f), text ? err : strerror(errno)), NULL);
        }
        free(text);
        sl_free(&options);
    }
    sl_free(&files);
    return report_problems == before;
}

/* One run of the application exe: start it, send the requests, stop it,
 * and compare or record what came of it. */
static void run_run(const char *run_dir, const char *exe) {
    char rw[PATHLEN], f[PATHLEN], g[PATHLEN], db[PATHLEN], dml[PATHLEN], interaction[PATHLEN];
    char server_out[PATHLEN], server_err[PATHLEN];
    long out_at = 0, err_at = 0;
    char bad[PATHLEN] = "", changed[PATHLEN] = "", failed[PATHLEN] = "", note[512] = "";
    struct strlist args = { 0, 0, 0 }, files = { 0, 0, 0 }, outputs = { 0, 0, 0 }, diffs = { 0, 0, 0 };
    char *text;
    int i, has_db, before = report_problems;
    long t0 = tenths_now();

    /* The outputs land under rw as they are laid out under the run. */
    n_runs++;
    join(rw, work, "run");
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
    join(db, rw, "db");
    has_db = is_file(f);
    if (has_db) {
        static char *load[] = { "sqlite3", "-batch", "-bail", NULL, NULL };
        load[3] = db;
        if (run_in(load, f, "/dev/null", fmt("%s/sqlite.err", rw)) != 0) {
            bail_if_interrupted();
            die_later(fmt("%s does not load", path(f)), head_of(fmt("%s/sqlite.err", rw)), NULL);
            return;
        }
    }
    /* Also without a db.sql, or the application would go for the database
     * named when it was compiled; and it wants a file to be there, if only
     * an empty one, which to SQLite is a database with nothing in it. */
    if (!has_db && write_text(db, "") < 0) {
        die(fmt("cannot write %s: %s", path(db), strerror(errno)), NULL);
    }
    setenv("URWEB_SQLITE_DB_PATH", db, 1);

    join(dml, rw, "dml.sql");
    setenv("URWEB_DML_LOG", dml, 1);
    setenv("URWEB_REPRODUCIBLE_EPOCH", RUN_TIME, 1);
    setenv("URWEB_REPRODUCIBLE_PRNG_SEED", RUN_SEED, 1);
    setenv("URWEB_MIN_HEAP", RUN_MIN_HEAP, 1);
    setenv("TZ", "UTC", 1);

    join(f, run_dir, "args");
    text = is_file(f) ? read_text(f) : NULL;
    if (text) {
        split_words(text, &args);
    }
    free(text);

    join(server_out, rw, "server.stdout");
    join(server_err, rw, "server.stderr");
    if (server_start(exe, &args, server_out, server_err, &serving) < 0) {
        bail_if_interrupted();
        die_later(fmt("the application does not start for %s", test(run_dir)), head_of(server_err), NULL);
        sl_free(&args);
        return;
    }
    sl_free(&args);

    /* What the application did before the first request: the initialize
     * task's statements, and what it printed while starting. */
    if (exists(dml) && rename(dml, fmt("%s/out/startup.sql", rw)) < 0) {
        die(fmt("cannot move %s: %s", path(dml), strerror(errno)), NULL);
    }
    sl_push(&outputs, "out/startup.sql");
    cut_output(server_out, &out_at, fmt("%s/out/startup.stdout", rw));
    cut_output(server_err, &err_at, fmt("%s/out/startup.stderr", rw));
    sl_push(&outputs, "out/startup.stdout");
    sl_push(&outputs, "out/startup.stderr");

    /* The requests, in the order of their names.  One that cannot be read
     * ends the run: those after it count on what it would have done. */
    join(interaction, run_dir, "interaction");
    if (is_dir(interaction)) {
        list_dir(interaction, LS_FILES, &files);
    }
    for (i = 0; i < files.n && report_problems == before; i++) {
        char method[32], url_path[PATHLEN], err[256], name[PATHLEN];
        struct strlist options = { 0, 0, 0 };
        if (!has_suffix(files.v[i], ".request")) {
            continue;
        }
        snprintf(name, sizeof name, "%s", files.v[i]);
        strip_suffix(name, ".request");
        join(f, interaction, files.v[i]);
        text = read_text(f);
        if (!text || parse_request(text, method, sizeof method, url_path, sizeof url_path, &options, err, sizeof err) < 0) {
            die_later(fmt("%s: %s", path(f), text ? err : strerror(errno)), NULL);
        } else {
            if (send_request(method, url_path, &options, serving.port, fmt("%s/interaction/%s.response", rw, name)) != 0) {
                list_add(failed, sizeof failed, name);
            }
            sl_push(&outputs, fmt("interaction/%s.response", name));
            /* The server has written what this request changed, if
             * anything, before it answered; taken away, the file will be
             * there again only if the next request changes something. */
            if (exists(dml) && rename(dml, fmt("%s/interaction/%s.sql", rw, name)) < 0) {
                die(fmt("cannot move %s: %s", path(dml), strerror(errno)), NULL);
            }
            sl_push(&outputs, fmt("interaction/%s.sql", name));
            /* Likewise what it printed: the server flushes every line as
             * it goes, and all of a request's come before its answer. */
            cut_output(server_out, &out_at, fmt("%s/interaction/%s.stdout", rw, name));
            cut_output(server_err, &err_at, fmt("%s/interaction/%s.stderr", rw, name));
            sl_push(&outputs, fmt("interaction/%s.stdout", name));
            sl_push(&outputs, fmt("interaction/%s.stderr", name));
        }
        free(text);
        sl_free(&options);
        if (fs_interrupted) {
            break;
        }
    }
    server_stop(&serving);
    bail_if_interrupted();
    if (report_problems != before) {
        sl_free(&files);
        sl_free(&outputs);
        return;
    }

    cut_output(server_out, &out_at, fmt("%s/out/shutdown.stdout", rw));
    cut_output(server_err, &err_at, fmt("%s/out/shutdown.stderr", rw));
    sl_push(&outputs, "out/shutdown.stdout");
    sl_push(&outputs, "out/shutdown.stderr");
    if (has_db) {
        static char *dump[] = { "sqlite3", "-batch", NULL, ".dump", NULL };
        dump[2] = db;
        if (run(dump, fmt("%s/out/db_final.sql", rw), fmt("%s/sqlite.err", rw)) != 0) {
            die_later(fmt("cannot dump the database of %s", test(run_dir)), head_of(fmt("%s/sqlite.err", rw)), NULL);
        }
    }
    sl_push(&outputs, "out/db_final.sql");

    /* An output that is no longer produced -- the NAME.sql of a request
     * that changes nothing any more -- goes, or fails the check. */
    sl_sort(&outputs);
    for (i = 0; i < outputs.n; i++) {
        settle(run_dir, outputs.v[i], fmt("%s/%s", rw, outputs.v[i]), bad, changed, sizeof bad, &diffs);
    }

    if (*failed) {
        snprintf(note, sizeof note, " %scurl failed: %s%s", style(ST_AMBER), failed, style(ST_RESET));
    }
    snprintf(note + strlen(note), sizeof note - strlen(note), "%s", took(tenths_now() - t0));
    if (opt_check && *bad) {
        if (!at_blank) {
            putchar('\n');
        }
        say(ST_BAD, "FAIL", run_dir, fmt(" (%s)%s", bad, note));
        for (i = 0; i < diffs.n; i++) {
            show_diff(diffs.v[i]);
        }
        putchar('\n');
        at_blank = 1;
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
        at_blank = 0;
    }
    sl_free(&files);
    sl_free(&outputs);
    sl_free(&diffs);
}

/* Run the compiler for one case and compare, or record, what it wrote;
 * then the application, if the case has runs. */
static void run_case(const char *case_dir, const char *project) {
    char want_dir[PATHLEN], f[PATHLEN], now[PATHLEN], exe[PATHLEN], settings[PATHLEN], built[PATHLEN];
    struct strlist entries = { 0, 0, 0 }, argv = { 0, 0, 0 }, extra = { 0, 0, 0 }, runs = { 0, 0, 0 };
    char missing[PATHLEN] = "", bad[PATHLEN] = "", changed[PATHLEN] = "", note[512] = "";
    const char *stop = "parseJob", *stopper;
    char *args, **command;
    int maxrank = 0, i, status, own_stop = 0, no_exe;
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
    if (runs.n && (!on_path("curl") || !on_path("sqlite3"))) {
        die(fmt("%s has runs, and they need %s and %s on the PATH", test(case_dir), cmd("curl"), cmd("sqlite3")),
            "the one to talk to the application, the other to load and dump its database", NULL);
    }
    n_cases++;
    for (i = 0; i < extra.n; i++) {
        if (!strcmp(extra.v[i], "-stop") || !strcmp(extra.v[i], "-stopQuiet") || !strcmp(extra.v[i], "-tc")) {
            own_stop = 1;
        }
    }
    /* With runs the compiler goes all the way, and builds the application
     * where the project has it built.  From there it is moved into the
     * case, beside expected/: out of the project's way, and still there
     * afterwards, to be started by hand when a run needs looking into.
     * The one from last time goes first, lest it stand in for one that
     * does not build. */
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
    } else if (!own_stop) {
        sl_push(&argv, "-stopQuiet");
        sl_push(&argv, stop);
    }
    for (i = 0; i < extra.n; i++) {
        sl_push(&argv, extra.v[i]);
    }
    sl_push(&argv, project);

    command = sl_argv(&argv);
    t0 = tenths_now();
    status = run(command, fmt("%s/stdout.txt", work), fmt("%s/stderr.txt", work));
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
        if (!at_blank) {
            putchar('\n');
        }
        say(ST_BAD, "FAIL", case_dir,
            fmt(" (%s%s%s)%s", opt_check ? bad : "", opt_check && *bad && no_exe ? "; " : "",
                no_exe ? fmt("no application for its %d run%s", runs.n, runs.n == 1 ? "" : "s") : "", note));
        for (i = 0; opt_check && i < entries.n; i++) {
            pathf(f, "%s/%s.diff", work, entries.v[i]);
            show_diff(f);
        }
        if (no_exe) {
            join(f, work, "stderr.txt");
            printf("    %s\n", info(head_of(f)));
        }
        putchar('\n');
        at_blank = 1;
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
        at_blank = 0;
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
        need_tmp();
    }
    started = tenths_now();
    for (i = 0; i < snaps.n; i++) {
        handle(snaps.v[i], do_case);
        bail_if_interrupted();
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

/* The next number for a case: one more than the highest of the cases
 * that are numbers, or 1. */
static int next_number(const char *snap) {
    struct strlist names = { 0, 0, 0 };
    int i, highest = 0;
    list_dir(snap, LS_DIRS, &names);
    for (i = 0; i < names.n; i++) {
        char *end;
        long n = strtol(names.v[i], &end, 10);
        if (*names.v[i] && !*end && n > highest) {
            highest = (int) n;
        }
    }
    sl_free(&names);
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
        pathf(d, "%s/%d", snap, next_number(snap));
        sl_push(&cases, d);
    } else {
        for (i = 0; i < argc; i++) {
            char c[PATHLEN];
            snprintf(c, sizeof c, "%s", argv[i]);
            strip_suffix(c, "/");
            if (!valid_case_name(c)) {
                die(fmt("'%s' cannot name a case", c),
                    "a case name is letters, digits, _ and -, and nothing else", NULL);
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
        own_stop |= !strcmp(extra.v[i], "-stop") || !strcmp(extra.v[i], "-stopQuiet") || !strcmp(extra.v[i], "-tc");
    }
    if (!own_stop) {
        sl_push(&argv, "-stopQuiet");
        sl_push(&argv, artifact_named("schema.sql")->phase);
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
 * tables -- an empty args, and a first request to be replaced. */
static int create_run(int argc, char **argv) {
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
            fmt("as in %s", cmd(fmt("%s --create-run PROJECT.urt/snapshot/CASE", me))), NULL);
    }

    join(rr, collected.v[0], "request-response");
    if (exists(rr) && !is_dir(rr)) {
        die(fmt("%s is in the way, and it is not a directory", path(rr)), NULL);
    }
    if (argc <= 1) {
        pathf(d, "%s/%d", rr, next_number(rr));
        sl_push(&runs, d);
    }
    for (i = 1; i < argc; i++) {
        char r[PATHLEN];
        snprintf(r, sizeof r, "%s", argv[i]);
        strip_suffix(r, "/");
        if (!valid_case_name(r) || !strcmp(r, "README.md")) {
            die(fmt("'%s' cannot name a run", r), "the name of a run is letters, digits, _ and -, and nothing else", NULL);
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
        join(f, d, "01.request");
        if (write_text(f, "GET /\n") < 0) {
            die(fmt("cannot write %s: %s", path(f), strerror(errno)), NULL);
        }
        say(ST_NEW, "CREATED", runs.v[i],
            info(fmt(" (args, secret, %sinteraction/01.request)", has_tables ? "db.sql, " : "")));
    }
    printf("%s\n", info(fmt("write the requests under interaction/, then run %s to fill in the rest",
                            cmd(fmt("%s %s", me, collected.v[0])))));
    sl_free(&runs);
    sl_free(&collected);
    sl_free(&collected_projects);
    return 0;
}

/* ------------------------------------------------------------------ main */

int snapshot_main(int argc, char **argv) {
    const char *env;
    int i;

    fs_catch_signals();
    atexit(cleanup);

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (!strcmp(a, "--whine") || !strcmp(a, "-w")) {
            opt_check = 1;
        } else if (!strcmp(a, "--create") || !strcmp(a, "-c")) {
            opt_create = 1;
            i++;
            break;
        } else if (!strcmp(a, "--create-run")) {
            opt_create_run = 1;
            i++;
            break;
        } else if (!strcmp(a, "--edit") || !strcmp(a, "-e")) {
            opt_edit = 1;
            i++;
            break;
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
                fmt("usage: %s", cmd(fmt("%s [--whine|-w | --list|-l] [PATH...]", me))),
                fmt("       %s", cmd(fmt("%s (--create|-c) [PROJECT] [CASE...]", me))),
                fmt("       %s", cmd(fmt("%s --create-run [PATH] [RUN...]", me))),
                fmt("       %s", cmd(fmt("%s (--edit|-e) [PATH] [ARTIFACT...]", me))), NULL);
        } else {
            break;
        }
    }
    argc -= i;
    argv += i;
    if ((opt_create || opt_create_run || opt_edit) && (opt_check || opt_list)) {
        die(fmt("%s goes with neither %s nor %s", key(opt_create ? "--create" : opt_edit ? "--edit" : "--create-run"),
                key("--whine"), key("--list")), NULL);
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
        return create_run(argc, argv);
    }
    i = run_all(argc, argv);
    sl_free(&urweb_flags);
    return i;
}
