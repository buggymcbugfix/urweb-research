/* report.c -- verdicts, warnings, errors and styled fragments.  See report.h. */

/* POSIX 2008 with the XSI extensions, and on macOS the BSD ones too,
 * which its headers hide once a strict level is asked for (mkdtemp is
 * one).  _DARWIN_C_SOURCE means nothing anywhere else. */
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "report.h"

int report_failed = 0;
int report_problems = 0;

static int colour = 0;

/* The palette: four muted tones from the 256-colour set, bold for a
 * verdict and for a name inside running text, grey for what is an aside.
 * This is the place to change the look. */
static const char *const sgr[] = {
    "\033[1;38;5;71m",     /* ST_OK      bold green */
    "\033[1;38;5;167m",    /* ST_BAD     bold red */
    "\033[1;38;5;172m",    /* ST_WARN    bold amber */
    "\033[1;38;5;67m",     /* ST_NEW     bold steel blue */
    "\033[38;5;245m",      /* ST_INFO    grey */
    "\033[1m",             /* ST_CASE    bold */
    "\033[1m",             /* ST_PATH    bold */
    "",                     /* ST_SUBJECT the plain weight */
    "\033[39m",            /* ST_CMD     the text's own colour, which stands out in an explanation */
    "\033[1;39m",          /* ST_KEY     bold, in the text's own colour */
    "\033[1m",             /* ST_STRONG  bold */
    "\033[38;5;71m",       /* ST_GREEN */
    "\033[38;5;167m",      /* ST_RED */
    "\033[38;5;172m",      /* ST_AMBER */
    "\033[0m"              /* ST_RESET */
};

/* Whether TERM names a terminal with 256 colours: what `tput colors`
 * would say, near enough, without reading terminfo. */
static int has_256_colours(void) {
    static const char *const known[] = { "alacritty", "kitty", "xterm-kitty", "foot", "wezterm",
                                         "rio", "ghostty", "xterm-ghostty", NULL };
    const char *term = getenv("TERM"), *ct = getenv("COLORTERM");
    int i;
    if (!term) {
        return 0;
    }
    if (strstr(term, "256color") || strstr(term, "truecolor") || strstr(term, "direct")) {
        return 1;
    }
    if (ct && (!strcmp(ct, "truecolor") || !strcmp(ct, "24bit"))) {
        return 1;
    }
    for (i = 0; known[i]; i++) {
        if (!strcmp(term, known[i])) {
            return 1;
        }
    }
    return 0;
}

void report_init(void) {
    const char *nc = getenv("NO_COLOR"), *term = getenv("TERM");
    colour = isatty(STDOUT_FILENO) && !(nc && *nc) && term && strcmp(term, "dumb") && has_256_colours();
    setvbuf(stdout, NULL, _IOLBF, 0);
}

const char *style(enum style s) { return colour ? sgr[s] : ""; }

/* ------------------------------------------------------------ fragments */

#define RING 32
#define RINGLEN 8192
static char ring[RING][RINGLEN];
static int ring_next = 0;

static char *ring_buf(void) {
    char *b = ring[ring_next];
    ring_next = (ring_next + 1) % RING;
    return b;
}

const char *fmt(const char *format, ...) {
    char *b = ring_buf();
    va_list ap;
    va_start(ap, format);
    vsnprintf(b, RINGLEN, format, ap);
    va_end(ap);
    return b;
}

static const char *styled(enum style s, const char *text) {
    char *b = ring_buf();
    snprintf(b, RINGLEN, "%s%s%s", style(s), text, style(ST_RESET));
    return b;
}

const char *test(const char *s) { return styled(ST_CASE, s); }
const char *path(const char *s) { return styled(ST_PATH, s); }
const char *subject(const char *s) { return styled(ST_SUBJECT, s); }
const char *cmd(const char *s) { return styled(ST_CMD, s); }
const char *key(const char *s) { return styled(ST_KEY, s); }

const char *info(const char *s) {
    char *b = ring_buf();
    const char *reset = style(ST_RESET), *grey = style(ST_INFO), *hit;
    size_t used = 0, rl = strlen(reset);
    if (!colour) {
        snprintf(b, RINGLEN, "%s", s);
        return b;
    }
    used += (size_t) snprintf(b + used, RINGLEN - used, "%s", grey);
    /* A fragment inside ends with a reset; grey resumes after each. */
    while ((hit = strstr(s, reset)) && used < RINGLEN) {
        used += (size_t) snprintf(b + used, RINGLEN - used, "%.*s%s%s", (int) (hit - s), s, reset, grey);
        s = hit + rl;
    }
    if (used < RINGLEN) {
        snprintf(b + used, RINGLEN - used, "%s%s", s, reset);
    }
    return b;
}

/* ------------------------------------------------------------- messages */

/* A word that carries a verdict, in its tone. */
static const char *word(enum style s, const char *w) {
    return styled(s, w);
}

void say(enum style s, const char *verdict, const char *test_dir, const char *note) {
    printf("%s %s%s\n", word(s, verdict), subject(test_dir), note);
}

/* Text for a workflow command: % and the line ends are escaped, and in a
 * property (prop) also , and :, as GitHub reads them. */
static void put_escaped(FILE *out, const char *s, size_t n, int prop) {
    size_t i;
    for (i = 0; i < n && s[i]; i++) {
        switch (s[i]) {
        case '%': fputs("%25", out); break;
        case '\r': fputs("%0D", out); break;
        case '\n': fputs("%0A", out); break;
        case ',': fputs(prop ? "%2C" : ",", out); break;
        case ':': fputs(prop ? "%3A" : ":", out); break;
        default: fputc(s[i], out);
        }
    }
}

void annotate(const char *file, const char *title, const char *diff) {
    const char *ga = getenv("GITHUB_ACTIONS");
    char text[4001];
    size_t n = 0;
    long line = 1;
    FILE *f;
    if (!ga || strcmp(ga, "true")) {
        return;
    }
    if (diff && (f = fopen(diff, "rb"))) {
        n = fread(text, 1, sizeof text - 1, f);
        fclose(f);
    }
    text[n] = 0;
    if (n) {
        /* The line of the expected file where the first change starts. */
        const char *hunk = strstr(text, "\n@@ -");
        if (hunk) {
            line = strtol(hunk + 5, NULL, 10);
        }
        if (line < 1) {
            line = 1;
        }
    }
    fflush(stdout);
    fputs("::error", stdout);
    if (file) {
        fputs(" file=", stdout);
        put_escaped(stdout, file, strlen(file), 1);
        printf(",line=%ld,", line);
    } else {
        fputc(' ', stdout);
    }
    fputs("title=", stdout);
    put_escaped(stdout, title, strlen(title), 1);
    fputs("::", stdout);
    if (n) {
        put_escaped(stdout, text, n, 0);
    } else {
        put_escaped(stdout, title, strlen(title), 0);
    }
    fputc('\n', stdout);
    fflush(stdout);
}

void warn(const char *msg) {
    fprintf(stderr, "%s %s\n", word(ST_WARN, "WARN"), msg);
}

/* The explanation lines of die and die_later. */
static void explain(va_list ap) {
    const char *line;
    while ((line = va_arg(ap, const char *))) {
        fprintf(stderr, "%s\n", info(line));
    }
}

void die(const char *msg, ...) {
    va_list ap;
    fprintf(stderr, "%s %s\n", word(ST_BAD, "ERROR"), msg);
    annotate(NULL, msg, NULL);
    va_start(ap, msg);
    explain(ap);
    va_end(ap);
    exit(2);
}

void die_later(const char *msg, ...) {
    va_list ap;
    report_failed = 1;
    report_problems++;
    fprintf(stderr, "%s %s\n", word(ST_BAD, "ERROR"), msg);
    annotate(NULL, msg, NULL);
    va_start(ap, msg);
    explain(ap);
    va_end(ap);
}
