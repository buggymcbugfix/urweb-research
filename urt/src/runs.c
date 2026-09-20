/* runs.c -- reading a request file; starting and stopping a server.
 * See runs.h.
 */

#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "runs.h"

/* ------------------------------------------------------------- requests */

/* The words of s, split as runs.h says.  0, or -1 with the complaint. */
static int shell_words(const char *s, struct strlist *out, char *err, size_t err_size) {
    char *word = xmalloc(strlen(s) + 1), *w = word;
    int in_word = 0;
    char quote = 0;

    for (; *s; s++) {
        if (quote == '\'') {
            if (*s == '\'') {
                quote = 0;
            } else {
                *w++ = *s;
            }
        } else if (quote == '"') {
            if (*s == '"') {
                quote = 0;
            } else if (*s == '\\' && (s[1] == '"' || s[1] == '\\')) {
                *w++ = *++s;
            } else {
                *w++ = *s;
            }
        } else if (*s == '\'' || *s == '"') {
            quote = *s;
            in_word = 1;
        } else if (*s == '\\' && s[1]) {
            if (*++s != '\n') {         /* a continued line is just a blank */
                *w++ = *s;
                in_word = 1;
            }
        } else if (strchr(" \t\r\n", *s)) {
            if (in_word) {
                *w = 0;
                sl_push(out, word);
                w = word;
                in_word = 0;
            }
        } else {
            *w++ = *s;
            in_word = 1;
        }
    }
    if (quote) {
        snprintf(err, err_size, "a %c is never closed", quote);
        free(word);
        return -1;
    }
    if (in_word) {
        *w = 0;
        sl_push(out, word);
    }
    free(word);
    return 0;
}

int parse_request(const char *text, char *method, size_t method_size, char *url_path, size_t path_size,
                  struct strlist *options, char *err, size_t err_size) {
    struct strlist words = { 0, 0, 0 };
    int i, r = -1;

    if (shell_words(text, &words, err, err_size) < 0) {
        goto out;
    }
    if (words.n < 2) {
        snprintf(err, err_size, "expected a method and a path, as in: GET /Shop/main");
        goto out;
    }
    if (strspn(words.v[0], "ABCDEFGHIJKLMNOPQRSTUVWXYZ") != strlen(words.v[0]) || strlen(words.v[0]) >= method_size) {
        snprintf(err, err_size, "'%s' is not a method; a request starts with one, as in: GET /Shop/main", words.v[0]);
        goto out;
    }
    if (words.v[1][0] != '/' || strlen(words.v[1]) >= path_size) {
        snprintf(err, err_size, "'%s' is not a path; the method is followed by one, starting with /", words.v[1]);
        goto out;
    }
    snprintf(method, method_size, "%s", words.v[0]);
    snprintf(url_path, path_size, "%s", words.v[1]);

    for (i = 2; i < words.n; i += 2) {
        const char *o = words.v[i], *v = i + 1 < words.n ? words.v[i + 1] : NULL;
        int is_data = !strcmp(o, "-d") || !strcmp(o, "--data");
        int is_cookie = !strcmp(o, "-b") || !strcmp(o, "--cookie");
        if (!is_data && !is_cookie) {
            snprintf(err, err_size, "'%s' is not taken here; the options are -d (--data) and -b (--cookie)", o);
            goto out;
        }
        if (!v) {
            snprintf(err, err_size, "%s lacks its value", o);
            goto out;
        }
        if (is_data && v[0] == '@') {
            snprintf(err, err_size, "-d '%s' would send a file; write out what is to be sent", v);
            goto out;
        }
        if (is_cookie && !strchr(v, '=')) {
            snprintf(err, err_size, "-b '%s' would read a file; a cookie is NAME=VALUE", v);
            goto out;
        }
        sl_push(options, is_data ? "-d" : "-b");
        sl_push(options, v);
    }
    r = 0;
out:
    sl_free(&words);
    return r;
}

/* --------------------------------------------------------------- servers */

/* The value of NAME= in the handshake a server writes to descriptor 3
 * (pid=..., port=..., status='OK', a line each), or NULL. */
static const char *handshake_value(const char *text, const char *name) {
    static char value[64];
    size_t n = strlen(name);
    const char *line = text, *end;

    while (line && *line) {
        if (!strncmp(line, name, n) && line[n] == '=') {
            line += n + 1;
            end = strchr(line, '\n');
            snprintf(value, sizeof value, "%.*s", end ? (int) (end - line) : (int) strlen(line), line);
            return value;
        }
        line = strchr(line, '\n');
        if (line) {
            line++;
        }
    }
    return NULL;
}

int server_start(const char *exe, const struct strlist *args, const char *out_path, const char *err_path,
                 struct server *s) {
    /* -P: the first free port from -p on, so that runs never collide;
     * -d3: go to the background once listening, and say so on fd 3. */
    static const char *const ours[] = { "-a", "127.0.0.1", "-p", "8000", "-P", "9999", "-d3" };
    struct strlist argv = { 0, 0, 0 };
    char **command, *said;
    const char *v;
    int i, status, ok;

    s->pid = 0;
    s->port = 0;
    sl_push(&argv, exe);
    for (i = 0; i < args->n; i++) {
        sl_push(&argv, args->v[i]);
    }
    for (i = 0; i < (int) (sizeof ours / sizeof ours[0]); i++) {
        sl_push(&argv, ours[i]);
    }
    command = sl_argv(&argv);
    said = run_fd3(command, out_path, err_path, &status);
    free(command);
    sl_free(&argv);
    if (!said) {
        return -1;
    }
    if ((v = handshake_value(said, "pid"))) {
        s->pid = atol(v);
    }
    if ((v = handshake_value(said, "port"))) {
        s->port = atoi(v);
    }
    v = handshake_value(said, "status");
    ok = status == 0 && v && !strcmp(v, "'OK'") && s->pid > 0 && s->port > 0;
    free(said);
    if (!ok) {
        server_stop(s);
        return -1;
    }
    return 0;
}

/* Whether the process is dead but not yet reaped: it is not ours to
 * reap, and under a PID 1 that reaps no orphans (some containers') it
 * stays so, answering kill(pid, 0) as if alive.  Linux says in /proc;
 * elsewhere nothing does, and the wait below runs its course. */
static int is_zombie(long pid) {
    char path[64], buf[512], *paren;
    FILE *f;
    size_t n;

    snprintf(path, sizeof path, "/proc/%ld/stat", pid);
    f = fopen(path, "r");
    if (!f) {
        return 0;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    paren = strrchr(buf, ')');      /* the name before it may hold anything */
    return paren && paren[1] == ' ' && paren[2] == 'Z';
}

void server_stop(struct server *s) {
    struct timespec nap = { 0, 10 * 1000 * 1000 };
    int i;

    if (s->pid <= 0) {
        return;
    }
    /* Not our child -- it left its parent when it went to the background
     * -- so there is nothing to wait for but its disappearance. */
    kill((pid_t) s->pid, SIGTERM);
    for (i = 0; i < 500 && kill((pid_t) s->pid, 0) == 0 && !is_zombie(s->pid); i++) {
        nanosleep(&nap, NULL);
    }
    if (i == 500) {
        kill((pid_t) s->pid, SIGKILL);
    }
    s->pid = 0;
}
