/* snapshot.c -- urt snapshot: snapshot tests for the Ur/Web compiler.
 * Run it on a project and keep its output and some intermediate state as
 * files, so that a change to the compiler shows up as a diff.  `urt
 * snapshot --help` prints the layout and the usage; usage_text below is
 * that.
 *
 * Held to the transcripts under urt/tests/snapshot by urt/check.
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
    "Nothing else may be under snapshot/, bar a README.md beside the cases or in\n"
    "one: a file this program would not read is refused rather than left to rot.\n"
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

static int opt_check = 0, opt_create = 0, opt_list = 0, opt_edit = 0;
static const char *me = "urt snapshot";   /* how to invoke us, for messages */
static char urweb[PATHLEN];               /* the compiler, absolute */
static struct strlist urweb_flags;        /* what it always gets */
static char tmp[PATHLEN] = "";            /* our scratch directory */
static char work[PATHLEN];                /* where a run's artifacts land */
static int diff_labels = 0;               /* GNU diff, which names the sides for us */

static int n_cases = 0, n_ok = 0, n_bad = 0, n_changed = 0, n_skipped = 0, n_nonzero = 0;
static long started;

/* The scratch directory goes with us, whichever way we leave. */
static void cleanup(void) {
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
    struct strlist tree = { 0, 0, 0 };
    char e[PATHLEN], skip[PATHLEN] = "";
    int before = report_problems, i;

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
        const char *rel = tree.v[i];
        if (*skip && has_prefix(rel, skip)) {
            continue; /* under something already refused */
        }
        join(e, snap, rel);
        if (!strcmp(rel, "README.md") || has_suffix(rel, "/README.md") || has_suffix(rel, "/args")) {
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
        } else if (strchr(rel, '/')) {
            die_later(fmt("%s does not belong in a case", path(e)),
                      "a case holds args, README.md and expected/, nothing else",
                      "delete it, or move it where it goes", NULL);
        } else if (!strcmp(rel, "expected") || !strcmp(rel, "args")) {
            die_later(fmt("%s is not in a case", path(e)),
                      "every snapshot is a named case now: move args and expected/ into one",
                      fmt("such as %s", path(fmt("%s/1", snap))), NULL);
            pathf(skip, "%s/", rel);
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

/* The tree the compiler ran in, resolved, with a trailing slash; for the
 * headers a project's own `include` directive names, which the compiler
 * writes by absolute path. */
static const char *tree_prefix(void) {
    static char prefix[PATHLEN] = "";
    if (!*prefix) {
        if (!realpath(".", prefix)) {
            strcpy(prefix, ".");
        }
        strcat(prefix, "/");
    }
    return prefix;
}

/* #include "…/include/urweb/x.h" keeps only the part from include/ on; an
 * include of a header under the tree keeps its path from the tree's root. */
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
    if (has_prefix(q, tree) && q + strlen(tree) < end) {
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
    argv[n++] = (char *) want;
    argv[n++] = (char *) now;
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

/* Run the compiler for one case and compare, or record, what it wrote. */
static void run_case(const char *case_dir, const char *project) {
    char want_dir[PATHLEN], f[PATHLEN], now[PATHLEN];
    struct strlist entries = { 0, 0, 0 }, argv = { 0, 0, 0 }, extra = { 0, 0, 0 };
    char missing[PATHLEN] = "", bad[PATHLEN] = "", changed[PATHLEN] = "", note[512] = "";
    const char *stop = "parseJob";
    char *args, **command;
    int maxrank = 0, i, status, own_stop = 0;
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
    if (entries.n == 0) {
        warn(fmt("%s asks for nothing; %s says what there is to ask for",
                 path(want_dir), cmd(fmt("%s --edit %s", me, case_dir))));
        n_skipped++;
        sl_free(&entries);
        sl_free(&argv);
        return;
    }
    n_cases++;

    /* How far to run.  A case whose own args say that is left to it. */
    join(f, case_dir, "args");
    args = is_file(f) ? read_text(f) : NULL;
    if (args) {
        split_words(args, &extra);
    }
    free(args);
    for (i = 0; i < extra.n; i++) {
        if (!strcmp(extra.v[i], "-stop") || !strcmp(extra.v[i], "-stopQuiet") || !strcmp(extra.v[i], "-tc")) {
            own_stop = 1;
        }
    }
    if (!own_stop) {
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
                      fmt("it was stopped after %s, which this runner takes to be late enough", key(stop)),
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

    if (opt_check && *bad) {
        /* The verdict, then the diffs under it, set apart by blank lines. */
        if (!at_blank) {
            putchar('\n');
        }
        say(ST_BAD, "FAIL", case_dir, fmt(" (%s)%s", bad, note));
        for (i = 0; i < entries.n; i++) {
            pathf(f, "%s/%s.diff", work, entries.v[i]);
            show_diff(f);
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
    sl_free(&entries);
    sl_free(&argv);
    sl_free(&extra);
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
    printf("%s%d case%s%s%s%s, in %ld.%lds\n", style(ST_STRONG), n_cases, n_cases == 1 ? "" : "s", style(ST_RESET),
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
    if (!opt_list && !*tmp) {
        static char *probe[] = { "diff", "-u", "--label", "a", "--label", "b", "/dev/null", "/dev/null", NULL };
        if (make_tmpdir(tmp) < 0) {
            die("cannot make a temporary directory", NULL);
        }
        join(work, tmp, "work");
        /* GNU diff labels its two sides for us; without that the header
         * names a temporary file, which is only confusing. */
        diff_labels = run(probe, "/dev/null", "/dev/null") == 0;
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

/* The cases handle() found, for --edit to pick from. */
static struct strlist collected;

static void collect_case(const char *case_dir, const char *project) {
    (void) project;
    sl_push(&collected, case_dir);
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
    return status;
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
                fmt("       %s", cmd(fmt("%s (--edit|-e) [PATH] [ARTIFACT...]", me))), NULL);
        } else {
            break;
        }
    }
    argc -= i;
    argv += i;
    if ((opt_create || opt_edit) && (opt_check || opt_list)) {
        die(fmt("%s goes with neither %s nor %s", key(opt_create ? "--create" : "--edit"), key("--whine"), key("--list")), NULL);
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
    i = run_all(argc, argv);
    sl_free(&urweb_flags);
    return i;
}
