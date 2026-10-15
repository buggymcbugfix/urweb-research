/* runs.c -- reading a request file; starting and stopping a server.
 * See runs.h.
 */

#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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
    const char *fixed[] = { "Host", "User-Agent" };
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
    /* -P: the first free port from -p on, so that runs never collide;
     * -d3: go to the background once listening, and say so on fd 3. */
    static const char *const ours[] = { "-a", "127.0.0.1", "-p", "8000", "-P", "9999", "-d3" };
    struct strlist argv = { 0, 0, 0 };
    char **command, *said;
    const char *v;
    int i, status;

    s->pid = 0;
    s->port = 0;
    s->ours = 0;
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

int fastcgi_start(const char *exe, const struct strlist *args, const char *sock_path, const char *out_path,
                  const char *err_path, struct server *s, char *why, size_t why_size) {
    struct strlist argv = { 0, 0, 0 };
    char spawner[PATHLEN], **command;
    int i, null;

    s->pid = 0;
    s->port = 0;
    s->ours = 1;
    *why = 0;
    if (strlen(sock_path) >= sizeof ((struct sockaddr_un *) 0)->sun_path) {
        snprintf(why, why_size, "the path of its socket, %s, is %lu bytes, and a Unix socket's can hold %lu: "
                 "set TMPDIR to a shorter directory", sock_path, (unsigned long) strlen(sock_path),
                 (unsigned long) sizeof ((struct sockaddr_un *) 0)->sun_path - 1);
        return -1;
    }
    if (!find_on_path("spawn-fcgi", spawner)) {
        snprintf(why, why_size, "there is no spawn-fcgi on the PATH to start it with");
        return -1;
    }
    /* -n: no fork, and no /dev/null for its stdout and stderr, as spawn-fcgi
     * gives the program it forks: it becomes the program, our child. */
    sl_push(&argv, spawner);
    sl_push(&argv, "-n");
    sl_push(&argv, "-s");
    sl_push(&argv, sock_path);
    sl_push(&argv, "--");
    sl_push(&argv, exe);
    for (i = 0; i < args->n; i++) {
        sl_push(&argv, args->v[i]);
    }
    command = sl_argv(&argv);
    null = open("/dev/null", O_RDONLY);
    s->pid = null < 0 ? -1 : spawn_on(command, null, out_path, err_path);
    if (null >= 0) {
        close(null);
    }
    free(command);
    sl_free(&argv);
    if (s->pid < 0) {
        s->pid = 0;
        snprintf(why, why_size, "it could not be started: %s", strerror(errno));
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------- FastCGI */

enum { FCGI_BEGIN_REQUEST = 1, FCGI_END_REQUEST = 3, FCGI_PARAMS = 4, FCGI_STDIN = 5, FCGI_STDOUT = 6,
       FCGI_STDERR = 7, FCGI_RESPONDER = 1, FCGI_REQUEST_COMPLETE = 0 };
enum { FCGI_VERSION_1 = 1, FCGI_HEADER = 8, FCGI_CONTENT_MAX = 65535, FCGI_ALIGN = 8 };
/* The body of FCGI_END_REQUEST: appStatus, big-endian, then protocolStatus. */
enum { FCGI_END_BODY = 8, FCGI_APP_STATUS = 0, FCGI_PROTOCOL_STATUS = 4 };

/* Bytes, growing at the end. */
struct bytes { unsigned char *p; size_t n, size; };

static void bytes_add(struct bytes *b, const void *data, size_t n) {
    if (b->n + n > b->size) {
        unsigned char *p;
        b->size = 2 * (b->n + n) + 1024;
        p = xmalloc(b->size);
        if (b->n) {
            memcpy(p, b->p, b->n);
        }
        free(b->p);
        b->p = p;
    }
    if (n) {
        memcpy(b->p + b->n, data, n);
    }
    b->n += n;
}

/* A record of request 1, the only one a connection carries here, padded
 * to a multiple of FCGI_ALIGN bytes as nginx pads the records it sends. */
static void add_record(struct bytes *b, int type, const void *data, size_t n) {
    static const unsigned char zeros[FCGI_ALIGN];
    size_t padding = (FCGI_ALIGN - n % FCGI_ALIGN) % FCGI_ALIGN;
    unsigned char h[FCGI_HEADER] = { FCGI_VERSION_1, (unsigned char) type, 0, FCGI_REQUEST_ID,
                                     (unsigned char) (n >> 8), (unsigned char) n, (unsigned char) padding, 0 };
    bytes_add(b, h, sizeof h);
    bytes_add(b, data, n);
    bytes_add(b, zeros, padding);
}

/* A stream: its bytes in records of at most FCGI_CONTENT_MAX, then the
 * empty record that ends it. */
static void add_stream(struct bytes *b, int type, const unsigned char *data, size_t n) {
    for (; n > FCGI_CONTENT_MAX; data += FCGI_CONTENT_MAX, n -= FCGI_CONTENT_MAX) {
        add_record(b, type, data, FCGI_CONTENT_MAX);
    }
    if (n) {
        add_record(b, type, data, n);
    }
    add_record(b, type, NULL, 0);
}

/* A length in a name-value pair: one byte below 128, else four, the top
 * bit set. */
static void add_length(struct bytes *b, size_t n) {
    unsigned char c[4] = { (unsigned char) (0x80 | (n >> 24)), (unsigned char) (n >> 16), (unsigned char) (n >> 8),
                           (unsigned char) n };
    bytes_add(b, n < 128 ? c + 3 : c, n < 128 ? 1 : 4);
}

static int write_bytes(const char *path, const struct bytes *b) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        return -1;
    }
    if (b->n) {
        fwrite(b->p, 1, b->n, f);
    }
    return fclose(f) == 0 ? 0 : -1;
}

int fastcgi_request(const char *sock_path, const struct strlist *params, const char *body, const char *response,
                    const char *stream, int seconds, struct fastcgi_oddity *odd) {
    static const unsigned char begin[8] = { 0, FCGI_RESPONDER, 0, 0, 0, 0, 0, 0 };
    struct bytes req = { 0, 0, 0 }, pairs = { 0, 0, 0 }, in = { 0, 0, 0 }, out = { 0, 0, 0 }, err = { 0, 0, 0 };
    struct sockaddr_un a;
    struct sigaction ignore, saved;
    long deadline = tenths_now() + seconds * 10L;
    char *text;
    size_t at;
    int fd, i, status = FASTCGI_NO_END;

    odd->request_id = -1;
    odd->protocol_status = FCGI_REQUEST_COMPLETE;

    if (strlen(sock_path) >= sizeof a.sun_path) {
        return FASTCGI_REFUSED;
    }
    memset(&a, 0, sizeof a);
    a.sun_family = AF_UNIX;
    strcpy(a.sun_path, sock_path);
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return FASTCGI_REFUSED;
    }
    if (connect(fd, (struct sockaddr *) &a, sizeof a) < 0) {
        close(fd);
        return FASTCGI_REFUSED;
    }

    add_record(&req, FCGI_BEGIN_REQUEST, begin, sizeof begin);
    for (i = 0; i < params->n; i++) {
        const char *eq = strchr(params->v[i], '=');
        size_t n = eq ? (size_t) (eq - params->v[i]) : strlen(params->v[i]), v = eq ? strlen(eq + 1) : 0;
        add_length(&pairs, n);
        add_length(&pairs, v);
        bytes_add(&pairs, params->v[i], n);
        bytes_add(&pairs, eq ? eq + 1 : "", v);
    }
    add_stream(&req, FCGI_PARAMS, pairs.p, pairs.n);
    text = read_text(body);
    add_stream(&req, FCGI_STDIN, (const unsigned char *) (text ? text : ""), text ? strlen(text) : 0);
    free(text);

    /* All of it at once, as a web server sends it, so that a program that
     * answers and closes before it has read the end of its stdin cannot
     * make a later write fail.  SIGPIPE would end urt: a connection the
     * program has closed is an outcome to record, like any other. */
    memset(&ignore, 0, sizeof ignore);
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGPIPE, &ignore, &saved);
    for (at = 0; at < req.n;) {
        ssize_t w = write(fd, req.p + at, req.n - at);
        if (w < 0 && errno == EINTR && !fs_interrupted) {
            continue;
        }
        if (w <= 0) {
            break;      /* closed: what the program sent before is still read */
        }
        at += (size_t) w;
    }
    sigaction(SIGPIPE, &saved, NULL);

    /* Records until FCGI_END_REQUEST, or until the connection ends. */
    for (;;) {
        unsigned char buf[FCGI_HEADER + FCGI_CONTENT_MAX + 255];    /* a whole record, padding and all */
        struct pollfd p;
        long left;
        ssize_t r;
        int ended = 0;
        while (!ended && in.n >= FCGI_HEADER) {
            size_t len = ((size_t) in.p[4] << 8) | in.p[5], whole = FCGI_HEADER + len + in.p[6];
            const unsigned char *content = in.p + FCGI_HEADER;
            int id = (in.p[2] << 8) | in.p[3];
            if (in.n < whole) {
                break;
            }
            /* A record for another request: taken all the same, since the
             * program meant it for this one (nothing else was asked), and
             * noted, since a web server would not take it. */
            if (id != FCGI_REQUEST_ID && odd->request_id < 0) {
                odd->request_id = id;
            }
            if (in.p[1] == FCGI_STDOUT) {
                bytes_add(&out, content, len);
            } else if (in.p[1] == FCGI_STDERR) {
                bytes_add(&err, content, len);
            } else if (in.p[1] == FCGI_END_REQUEST && len >= FCGI_END_BODY) {
                status = content[FCGI_APP_STATUS + 3];      /* the low byte of appStatus */
                odd->protocol_status = content[FCGI_PROTOCOL_STATUS];
                ended = 1;
            }
            memmove(in.p, in.p + whole, in.n - whole);
            in.n -= whole;
        }
        if (ended) {
            break;
        }
        left = deadline - tenths_now();
        if (left <= 0) {
            status = FASTCGI_TIMEOUT;
            break;
        }
        p.fd = fd;
        p.events = POLLIN;
        p.revents = 0;
        r = poll(&p, 1, (int) (left * 100));
        if (r < 0 && errno == EINTR && !fs_interrupted) {
            continue;
        }
        if (r == 0) {
            status = FASTCGI_TIMEOUT;
            break;
        }
        if (r < 0 || (r = read(fd, buf, sizeof buf)) <= 0) {
            if (r < 0 && errno == EINTR && !fs_interrupted) {
                continue;
            }
            break;      /* closed, or reset, before FCGI_END_REQUEST */
        }
        bytes_add(&in, buf, (size_t) r);
    }
    close(fd);

    if (write_bytes(response, &out) < 0 || write_bytes(stream, &err) < 0) {
        status = FASTCGI_UNWRITABLE;
    }
    free(req.p);
    free(pairs.p);
    free(in.p);
    free(out.p);
    free(err.p);
    return status;
}

int server_exited(struct server *s, int *status) {
    int st;
    if (s->pid <= 0 || !s->ours) {
        return 0;
    }
    if (waitpid((pid_t) s->pid, &st, WNOHANG) != (pid_t) s->pid) {
        return 0;
    }
    s->pid = 0;
    *status = WIFEXITED(st) ? WEXITSTATUS(st) : WIFSIGNALED(st) ? 128 + WTERMSIG(st) : st;
    return 1;
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
    kill((pid_t) s->pid, SIGTERM);
    if (s->ours) {
        /* Our child, started on a socket: reaped, or it would stay a
         * zombie. */
        for (i = 0; i < 500 && waitpid((pid_t) s->pid, NULL, WNOHANG) == 0; i++) {
            nanosleep(&nap, NULL);
        }
        if (i == 500) {
            kill((pid_t) s->pid, SIGKILL);
            while (waitpid((pid_t) s->pid, NULL, 0) < 0 && errno == EINTR) {
                ;
            }
        }
        s->pid = 0;
        return;
    }
    /* Not our child -- it left its parent when it went to the background
     * -- so there is nothing to wait for but its disappearance. */
    for (i = 0; i < 500 && kill((pid_t) s->pid, 0) == 0 && !is_zombie(s->pid); i++) {
        nanosleep(&nap, NULL);
    }
    if (i == 500) {
        kill((pid_t) s->pid, SIGKILL);
    }
    s->pid = 0;
}
