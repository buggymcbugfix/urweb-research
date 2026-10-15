/* runs.c -- reading a request file; starting and stopping a server.
 * See runs.h.
 */

#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "runs.h"

/* ------------------------------------------------------------- requests */

int parse_duration(const char *s, long long *ms) {
    static const struct { const char *unit; double ms; } units[] = {
        { "ms", 1 }, { "s", 1000 }, { "min", 60000 }, { "h", 3600000 }, { "d", 86400000 },
    };
    const char *p = s;
    double n;
    size_t i;

    if (!(*p >= '0' && *p <= '9')) {
        return -1;
    }
    while (*p >= '0' && *p <= '9') {
        p++;
    }
    if (*p == '.') {
        p++;
        if (!(*p >= '0' && *p <= '9')) {
            return -1;
        }
        while (*p >= '0' && *p <= '9') {
            p++;
        }
    }
    n = strtod(s, NULL);
    for (i = 0; i < sizeof units / sizeof units[0]; i++) {
        if (!strcmp(p, units[i].unit)) {
            if (n * units[i].ms > 1e15) {
                return -1;
            }
            *ms = (long long) (n * units[i].ms + 0.5);
            return 0;
        }
    }
    return -1;
}

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
            /* A continued line is just a blank, with CRLF line ends too. */
            if (s[1] == '\r' && s[2] == '\n') {
                s += 2;
            } else if (*++s != '\n') {
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

/* 0 if the header named by the first n characters of v is one urt sets
 * itself for every request, else 1.  Header names are case-insensitive. */
static int fixed_header(const char *v, size_t n) {
    const char *fixed[] = { "Host", "User-Agent", "UrWeb-Time" };
    size_t i;

    for (i = 0; i < sizeof fixed / sizeof *fixed; i++) {
        if (strlen(fixed[i]) == n && !strncasecmp(v, fixed[i], n)) {
            return 0;
        }
    }
    return 1;
}

/* The name of the header v, NAME: VALUE or NAME; with the blanks around
 * it dropped: its start into *name and its length returned, 0 for none. */
static size_t header_name(const char *v, const char **name) {
    size_t n = strcspn(v, ":;");
    const char *s = v, *e = v + n;

    while (s < e && (*s == ' ' || *s == '\t')) {
        s++;
    }
    while (e > s && (e[-1] == ' ' || e[-1] == '\t')) {
        e--;
    }
    *name = s;
    return v[n] ? (size_t) (e - s) : 0;
}

/* Whether s holds a byte that cannot go in a header line, or a path:
 * a control character, CR and LF among them. */
static int has_control(const char *s) {
    for (; *s; s++) {
        if ((unsigned char) *s < 0x20 || *s == 0x7f) {
            return 1;
        }
    }
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
    if (!*words.v[0] || strspn(words.v[0], "ABCDEFGHIJKLMNOPQRSTUVWXYZ") != strlen(words.v[0])
        || strlen(words.v[0]) >= method_size) {
        snprintf(err, err_size, "'%s' is not a method; a request starts with one, as in: GET /Shop/main", words.v[0]);
        goto out;
    }
    if (words.v[1][0] != '/' || strlen(words.v[1]) >= path_size) {
        snprintf(err, err_size, "'%s' is not a path; the method is followed by one, starting with /", words.v[1]);
        goto out;
    }
    if (strchr(words.v[1], '#') || strchr(words.v[1], ' ') || has_control(words.v[1])) {
        snprintf(err, err_size, "'%s' cannot be sent as a path: a # starts a fragment, which never leaves the client,"
                 " and a blank or a control character has to be percent-encoded", words.v[1]);
        goto out;
    }
    snprintf(method, method_size, "%s", words.v[0]);
    snprintf(url_path, path_size, "%s", words.v[1]);

    for (i = 2; i < words.n; i += 2) {
        const char *o = words.v[i], *v = i + 1 < words.n ? words.v[i + 1] : NULL;
        int is_data = !strcmp(o, "-d") || !strcmp(o, "--data");
        int is_cookie = !strcmp(o, "-b") || !strcmp(o, "--cookie");
        int is_header = !strcmp(o, "-H") || !strcmp(o, "--header");
        if (!is_data && !is_cookie && !is_header) {
            snprintf(err, err_size, "'%s' is not taken here; the options are -d (--data), -b (--cookie) and -H (--header)", o);
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
        if ((is_cookie || is_header) && has_control(v)) {
            snprintf(err, err_size, "%s '%s' holds a control character; it goes in a header line, which can hold none",
                     o, v);
            goto out;
        }
        if (is_header) {
            /* curl takes NAME: VALUE, NAME; for an empty value, NAME: for
             * none of curl's own, and @FILE for a file of them. */
            const char *name;
            size_t n = header_name(v, &name);
            if (v[0] == '@') {
                snprintf(err, err_size, "-H '%s' would read a file; write out the headers to be sent", v);
                goto out;
            }
            if (n == 0 || memchr(name, ' ', n) || memchr(name, '\t', n)) {
                snprintf(err, err_size, "-H '%s' is not a header; a header is NAME: VALUE", v);
                goto out;
            }
            if (!fixed_header(name, n)) {
                snprintf(err, err_size, "-H '%s' would change what urt sends with every request: the host is localhost, the user agent is urt", v);
                goto out;
            }
        }
        sl_push(options, is_data ? "-d" : is_cookie ? "-b" : "-H");
        sl_push(options, v);
    }
    r = 0;
out:
    sl_free(&words);
    return r;
}

/* --------------------------------------------------------------- servers */

/* How long a server gets to say it is listening.  The real one has its
 * database to open and its initialize task to run first; a hung one
 * would otherwise hang the run for good. */
#define START_SECONDS 10

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
                 struct server *s, char *why, size_t why_size) {
    /* -p 0: a port of the kernel's choosing, which it gives no other
     * server while this one has it, so that runs never collide, however
     * many start at once; -d3: go to the background once listening, and
     * say so, and on which port, on fd 3. */
    static const char *const ours[] = { "-a", "127.0.0.1", "-p", "0", "-d3" };
    struct strlist argv = { 0, 0, 0 };
    char **command, *said;
    const char *v;
    int i, status;

    s->pid = 0;
    s->port = 0;
    *why = 0;
    sl_push(&argv, exe);
    for (i = 0; i < args->n; i++) {
        sl_push(&argv, args->v[i]);
    }
    for (i = 0; i < (int) (sizeof ours / sizeof ours[0]); i++) {
        sl_push(&argv, ours[i]);
    }
    command = sl_argv(&argv);
    said = run_fd3(command, out_path, err_path, START_SECONDS, &status);
    free(command);
    sl_free(&argv);
    if (!said) {
        snprintf(why, why_size, "it could not be started: %s", strerror(errno));
        return -1;
    }
    if ((v = handshake_value(said, "pid"))) {
        s->pid = atol(v);
    }
    if ((v = handshake_value(said, "port"))) {
        s->port = atoi(v);
    }
    v = handshake_value(said, "status");
    /* The one that stays in the foreground exits 0 once the other is
     * listening, after saying which process and port that is. */
    if (status < 0) {
        snprintf(why, why_size, "it did not say it was listening within %d seconds", START_SECONDS);
    } else if (status >= 128) {
        snprintf(why, why_size, "it was killed by signal %d before it was listening", status - 128);
    } else if (status != 0) {
        snprintf(why, why_size, "it exited with status %d before it was listening", status);
    } else if (!*said) {
        snprintf(why, why_size, "it exited as if listening, but said nothing on descriptor 3");
    } else if (s->pid <= 0) {
        snprintf(why, why_size, "it exited as if listening, but its handshake names no pid");
    } else if (s->port <= 0) {
        snprintf(why, why_size, "it exited as if listening, but its handshake names no port");
    } else if (!v) {
        snprintf(why, why_size, "it exited as if listening, but its handshake has no status line");
    } else if (strcmp(v, "'OK'")) {
        snprintf(why, why_size, "it exited as if listening, but its handshake says status=%s, not 'OK'", v);
    }
    free(said);
    if (*why) {
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

int server_alive(const struct server *s) {
    return s->pid > 0 && kill((pid_t) s->pid, 0) == 0 && !is_zombie(s->pid);
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

int server_clock(const struct server *s, long long advance, long long *next, char *why, size_t why_size) {
    struct sockaddr_in addr;
    struct timeval limit = { CLOCK_SECONDS, 0 };
    char request[128], answer[1024], *body;
    size_t got = 0;
    ssize_t n = 0;
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd < 0) {
        snprintf(why, why_size, "cannot make a socket: %s", strerror(errno));
        return -1;
    }
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short) s->port);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &limit, sizeof limit);
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &limit, sizeof limit);
    if (connect(fd, (struct sockaddr *) &addr, sizeof addr) < 0) {
        snprintf(why, why_size, "cannot connect to the server: %s", strerror(errno));
        close(fd);
        return -1;
    }
    if (advance >= 0) {
        snprintf(request, sizeof request, "GET /.clock?advance=%lld HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n",
                 advance);
    } else {
        snprintf(request, sizeof request, "GET /.clock HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n");
    }
    {
        /* A server gone meanwhile is an error here, not a signal; and only
         * here, since what urt runs inherits what it ignores. */
        void (*was)(int) = signal(SIGPIPE, SIG_IGN);
        int sent = write(fd, request, strlen(request)) == (ssize_t) strlen(request), e = errno;
        signal(SIGPIPE, was);
        if (!sent) {
            snprintf(why, why_size, "cannot send to the server: %s", strerror(e));
            close(fd);
            return -1;
        }
    }
    /* The server closes the connection after its answer. */
    while (got < sizeof answer - 1 && (n = read(fd, answer + got, sizeof answer - 1 - got)) > 0) {
        got += (size_t) n;
    }
    if (n < 0 && got < sizeof answer - 1) {
        int e = errno;
        if (e == EAGAIN || e == EWOULDBLOCK) {
            snprintf(why, why_size, "the server did not answer the clock within %d seconds", CLOCK_SECONDS);
        } else {
            snprintf(why, why_size, "cannot read the server's answer to the clock: %s", strerror(e));
        }
        close(fd);
        return -1;
    }
    close(fd);
    answer[got] = 0;
    body = strstr(answer, "\r\n\r\n");
    if (strncmp(answer, "HTTP/1.1 200 ", 13) != 0 || !body) {
        snprintf(why, why_size, "the server's answer to the clock is not 200 OK%s", got ? "" : ": it said nothing");
        return -1;
    }
    body += 4;
    /* The first line is ours; what follows (released N) is a driver's. */
    if (!strncmp(body, "next none\n", 10)) {
        return 0;
    }
    if (sscanf(body, "next %lld", next) == 1) {
        return 1;
    }
    snprintf(why, why_size, "the server's answer to the clock is not 'next T' nor 'next none'");
    return -1;
}
