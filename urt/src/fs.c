/* fs.c -- files, directories and child processes.  See fs.h. */

/* POSIX 2008 with the XSI extensions, and on macOS the BSD ones too,
 * which its headers hide once a strict level is asked for (mkdtemp is
 * one).  _DARWIN_C_SOURCE means nothing anywhere else. */
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "fs.h"
#include "report.h"

/* ---------------------------------------------------------------- lists */

void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1);
    if (!p) {
        die("out of memory", NULL);
    }
    return p;
}

char *xstrdup(const char *s) {
    char *p = xmalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

void sl_push(struct strlist *l, const char *s) {
    if (l->n == l->cap) {
        l->cap = l->cap ? l->cap * 2 : 16;
        l->v = realloc(l->v, (size_t) l->cap * sizeof *l->v);
        if (!l->v) {
            die("out of memory", NULL);
        }
    }
    l->v[l->n++] = xstrdup(s);
}

static int by_bytes(const void *a, const void *b) {
    return strcmp(*(char *const *) a, *(char *const *) b);
}

void sl_sort(struct strlist *l) {
    if (l->n > 1) {
        qsort(l->v, (size_t) l->n, sizeof *l->v, by_bytes);
    }
}

void sl_free(struct strlist *l) {
    int i;
    for (i = 0; i < l->n; i++) {
        free(l->v[i]);
    }
    free(l->v);
    l->v = NULL;
    l->n = l->cap = 0;
}

int sl_has(const struct strlist *l, const char *s) {
    int i;
    for (i = 0; i < l->n; i++) {
        if (!strcmp(l->v[i], s)) {
            return 1;
        }
    }
    return 0;
}

char **sl_argv(const struct strlist *l) {
    char **v = xmalloc(((size_t) l->n + 1) * sizeof *v);
    memcpy(v, l->v, (size_t) l->n * sizeof *v);
    v[l->n] = NULL;
    return v;
}

/* ---------------------------------------------------------------- paths */

void join(char *dst, const char *a, const char *b) {
    int n = !*a ? snprintf(dst, PATHLEN, "%s", b)
          : !*b ? snprintf(dst, PATHLEN, "%s", a)
          : snprintf(dst, PATHLEN, "%s/%s", a, b);
    if (n < 0 || n >= PATHLEN) {
        die(fmt("path too long: %s/%s", a, b), NULL);
    }
}

void pathf(char *dst, const char *format, ...) {
    va_list ap;
    int n;
    va_start(ap, format);
    n = vsnprintf(dst, PATHLEN, format, ap);
    va_end(ap);
    if (n < 0 || n >= PATHLEN) {
        die(fmt("path too long: %.60s...", dst), NULL);
    }
}

const char *basename_of(const char *p) {
    const char *slash = strrchr(p, '/');
    return slash ? slash + 1 : p;
}

void dirname_of(char *dst, const char *p) {
    const char *slash = strrchr(p, '/');
    if (!slash) {
        strcpy(dst, ".");
        return;
    }
    if (slash == p) {
        strcpy(dst, "/");
        return;
    }
    snprintf(dst, PATHLEN, "%.*s", (int) (slash - p), p);
}

int has_suffix(const char *s, const char *suffix) {
    size_t n = strlen(s), m = strlen(suffix);
    return n >= m && !strcmp(s + n - m, suffix);
}

int has_prefix(const char *s, const char *prefix) {
    return !strncmp(s, prefix, strlen(prefix));
}

/* --------------------------------------------------------------- files */

int is_file(const char *p) { struct stat st; return stat(p, &st) == 0 && S_ISREG(st.st_mode); }
int is_dir(const char *p)  { struct stat st; return stat(p, &st) == 0 && S_ISDIR(st.st_mode); }
int exists(const char *p)  { struct stat st; return lstat(p, &st) == 0 || stat(p, &st) == 0; }
int is_link(const char *p) { struct stat st; return lstat(p, &st) == 0 && S_ISLNK(st.st_mode); }
int can_exec(const char *p) { return access(p, X_OK) == 0; }

char *read_text(const char *p) {
    FILE *f = fopen(p, "rb");
    char *buf;
    size_t cap = 4096, n = 0, got;
    if (!f) {
        return NULL;
    }
    buf = xmalloc(cap);
    while ((got = fread(buf + n, 1, cap - n - 1, f)) > 0) {
        n += got;
        if (n + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            if (!buf) {
                die("out of memory", NULL);
            }
        }
    }
    fclose(f);
    buf[n] = 0;
    return buf;
}

int write_text(const char *p, const char *s) {
    FILE *f = fopen(p, "wb");
    if (!f) {
        return -1;
    }
    if (*s && fputs(s, f) == EOF) {
        fclose(f);
        return -1;
    }
    return fclose(f) == 0 ? 0 : -1;
}

int mkdir_p(const char *p) {
    char buf[PATHLEN], *s;
    if (strlen(p) >= sizeof buf) {
        return -1;
    }
    strcpy(buf, p);
    for (s = buf + 1; *s; s++) {
        if (*s != '/') {
            continue;
        }
        *s = 0;
        if (mkdir(buf, 0777) < 0 && errno != EEXIST) {
            return -1;
        }
        *s = '/';
    }
    if (mkdir(buf, 0777) < 0 && errno != EEXIST) {
        return -1;
    }
    return is_dir(buf) ? 0 : -1;
}

int rm_rf(const char *p) {
    struct stat st;
    if (lstat(p, &st) < 0) {
        return errno == ENOENT ? 0 : -1;
    }
    if (S_ISDIR(st.st_mode)) {
        struct strlist names = { 0, 0, 0 };
        char child[PATHLEN];
        int i, rc = 0;
        list_dir(p, LS_ALL, &names);
        for (i = 0; i < names.n; i++) {
            join(child, p, names.v[i]);
            if (rm_rf(child) < 0) {
                rc = -1;
            }
        }
        sl_free(&names);
        if (rc < 0) {
            return -1;
        }
        return rmdir(p);
    }
    return unlink(p);
}

int copy_file(const char *from, const char *to) {
    FILE *in = fopen(from, "rb"), *out;
    char buf[65536];
    size_t n;
    if (!in) {
        return -1;
    }
    out = fopen(to, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0) {
        if (fwrite(buf, 1, n, out) != n) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }
    fclose(in);
    return fclose(out) == 0 ? 0 : -1;
}

int move_file(const char *from, const char *to) {
    struct stat st;
    if (rename(from, to) == 0) {
        return 0;
    }
    /* Across file systems: a copy with the same permissions, then the
     * original goes. */
    if (errno != EXDEV || stat(from, &st) < 0 || copy_file(from, to) < 0 || chmod(to, st.st_mode & 07777) < 0) {
        return -1;
    }
    return unlink(from);
}

int same_contents(const char *a, const char *b) {
    FILE *fa = fopen(a, "rb"), *fb = fopen(b, "rb");
    char ba[65536], bb[65536];
    size_t na, nb;
    int same = 1;
    if (!fa || !fb) {
        if (fa) {
            fclose(fa);
        }
        if (fb) {
            fclose(fb);
        }
        return 0;
    }
    do {
        na = fread(ba, 1, sizeof ba, fa);
        nb = fread(bb, 1, sizeof bb, fb);
        if (na != nb || memcmp(ba, bb, na)) {
            same = 0;
        }
    } while (same && na > 0);
    fclose(fa);
    fclose(fb);
    return same;
}

/* ---------------------------------------------------------- directories */

void list_dir(const char *dir, int what, struct strlist *out) {
    DIR *d = opendir(dir);
    struct dirent *e;
    char p[PATHLEN];
    struct stat st;
    if (!d) {
        return;
    }
    while ((e = readdir(d))) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) {
            continue;
        }
        if (what != LS_ALL) {
            join(p, dir, e->d_name);
            if (lstat(p, &st) < 0) {
                continue;
            }
            if (what == LS_DIRS && !S_ISDIR(st.st_mode)) {
                continue;
            }
            if (what == LS_FILES && !S_ISREG(st.st_mode)) {
                continue;
            }
        }
        sl_push(out, e->d_name);
    }
    closedir(d);
    sl_sort(out);
}

/* walk() below, from dir, with rel the prefix collected so far. */
static void walk_from(const char *dir, const char *rel, struct strlist *out) {
    struct strlist names = { 0, 0, 0 };
    char p[PATHLEN], r[PATHLEN];
    struct stat st;
    int i;
    list_dir(dir, LS_ALL, &names);
    for (i = 0; i < names.n; i++) {
        join(p, dir, names.v[i]);
        join(r, rel, names.v[i]);
        sl_push(out, r);
        if (lstat(p, &st) == 0 && S_ISDIR(st.st_mode)) {
            walk_from(p, r, out);
        }
    }
    sl_free(&names);
}

void walk(const char *dir, struct strlist *out) {
    walk_from(dir, "", out);
    sl_sort(out);
}

/* find_dirs() below: dir itself is judged by the caller's loop. */
static void find_dirs_under(const char *dir, const char *suffix, struct strlist *out) {
    struct strlist names = { 0, 0, 0 };
    char p[PATHLEN];
    int i;
    list_dir(dir, LS_DIRS, &names);
    for (i = 0; i < names.n; i++) {
        join(p, dir, names.v[i]);
        if (has_suffix(names.v[i], suffix)) {
            sl_push(out, p);
        }
        find_dirs_under(p, suffix, out);
    }
    sl_free(&names);
}

void find_dirs(const char *dir, const char *suffix, struct strlist *out) {
    if (has_suffix(dir, suffix)) {
        sl_push(out, dir);
    }
    find_dirs_under(dir, suffix, out);
    sl_sort(out);
}

/* ------------------------------------------------------------ processes */

volatile int fs_interrupted = 0;

static void on_signal(int sig) {
    fs_interrupted = sig;
}

void fs_catch_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_signal;      /* no SA_RESTART: waitpid returns */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

/* In the child: stdout or stderr onto a file. */
static void redirect(int fd, const char *path) {
    int f;
    if (!path) {
        return;
    }
    f = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (f < 0 || dup2(f, fd) < 0) {
        _exit(127);
    }
    close(f);
}

int run(char *const argv[], const char *out_path, const char *err_path) {
    return run_in(argv, NULL, out_path, err_path);
}

int run_in(char *const argv[], const char *in_path, const char *out_path, const char *err_path) {
    pid_t pid;
    int st;
    fflush(stdout);
    fflush(stderr);
    pid = fork();
    if (pid < 0) {
        return 127;
    }
    if (pid == 0) {
        if (in_path) {
            int f = open(in_path, O_RDONLY);
            if (f < 0 || dup2(f, STDIN_FILENO) < 0) {
                _exit(127);
            }
            close(f);
        }
        redirect(STDOUT_FILENO, out_path);
        redirect(STDERR_FILENO, err_path);
        execvp(argv[0], argv);
        _exit(127);
    }
    while (waitpid(pid, &st, 0) < 0) {
        if (errno != EINTR) {
            return 127;
        }
    }
    if (WIFEXITED(st)) {
        return WEXITSTATUS(st);
    }
    if (WIFSIGNALED(st)) {
        return 128 + WTERMSIG(st);
    }
    return 127;
}

char *run_fd3(char *const argv[], const char *out_path, const char *err_path, int *status) {
    int fds[2], st;
    pid_t pid;
    char *text;
    size_t len = 0, cap = 256;
    ssize_t n;

    fflush(stdout);
    fflush(stderr);
    if (pipe(fds) < 0) {
        return NULL;
    }
    pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return NULL;
    }
    if (pid == 0) {
        close(fds[0]);
        redirect(STDOUT_FILENO, out_path);
        redirect(STDERR_FILENO, err_path);
        if (fds[1] != 3) {
            if (dup2(fds[1], 3) < 0) {
                _exit(127);
            }
            close(fds[1]);
        }
        execvp(argv[0], argv);
        _exit(127);
    }
    close(fds[1]);
    text = xmalloc(cap);
    for (;;) {
        if (len + 1 >= cap) {
            char *bigger = xmalloc(cap *= 2);
            memcpy(bigger, text, len);
            free(text);
            text = bigger;
        }
        n = read(fds[0], text + len, cap - len - 1);
        if (n < 0 && errno == EINTR && !fs_interrupted) {
            continue;
        }
        if (n <= 0) {
            break;
        }
        len += (size_t) n;
    }
    text[len] = 0;
    close(fds[0]);
    while (waitpid(pid, &st, 0) < 0) {
        if (errno != EINTR) {
            st = 127 << 8;
            break;
        }
    }
    *status = WIFEXITED(st) ? WEXITSTATUS(st) : WIFSIGNALED(st) ? 128 + WTERMSIG(st) : 127;
    return text;
}

/* The path comes back canonical -- no doubled or trailing slashes, no
 * symlinks -- because the compiler prints the paths it is given after
 * OS.Path.mkCanonical, and what it prints has to match what we strip.
 * macOS sets TMPDIR with a trailing slash and keeps /var behind a
 * symlink, so both matter. */
static const char *argv0 = "";

void fs_set_argv0(const char *a) { argv0 = a; }

/* The first executable called name on the PATH, into dst; 0 if none. */
static int find_on_path(const char *name, char *dst) {
    const char *p = getenv("PATH"), *colon;
    if (!p) {
        return 0;
    }
    for (;;) {
        colon = strchr(p, ':');
        if (!colon) {
            colon = p + strlen(p);
        }
        if (colon > p && snprintf(dst, PATHLEN, "%.*s/%s", (int) (colon - p), p, name) < PATHLEN && can_exec(dst)) {
            return 1;
        }
        if (!*colon) {
            return 0;
        }
        p = colon + 1;
    }
}

int on_path(const char *name) {
    char found[PATHLEN];
    return find_on_path(name, found);
}

int program_dir(char *dst) {
    char found[PATHLEN], dir[PATHLEN];
    const char *program = argv0;
    if (!strchr(argv0, '/')) {
        /* a bare name: the shell found it on the PATH, so can we */
        if (!find_on_path(argv0, found)) {
            return 0;
        }
        program = found;
    }
    dirname_of(dir, program);
    return realpath(dir, dst) != NULL;
}

int make_tmpdir(char *dst) {
    const char *base = getenv("TMPDIR");
    char made[PATHLEN];
    size_t n;
    if (!base || !*base) {
        base = "/tmp";
    }
    n = strlen(base);
    while (n > 1 && base[n - 1] == '/') {
        n--;
    }
    if (snprintf(made, sizeof made, "%.*s/urt.XXXXXX", (int) n, base) >= (int) sizeof made) {
        return -1;
    }
    if (!mkdtemp(made)) {
        return -1;
    }
    if (!realpath(made, dst)) {
        rmdir(made);
        return -1;
    }
    return 0;
}

long tenths_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 10 + ts.tv_nsec / 100000000;
}

void split_words(const char *s, struct strlist *out) {
    char *copy = xstrdup(s), *tok, *save = NULL;
    for (tok = strtok_r(copy, " \t\n", &save); tok; tok = strtok_r(NULL, " \t\n", &save)) {
        sl_push(out, tok);
    }
    free(copy);
}
