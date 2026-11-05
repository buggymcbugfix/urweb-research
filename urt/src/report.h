/* report.h -- how urt talks: verdict lines, warnings, errors, and the
 * styled fragments inside them.
 *
 * Colour when stdout is a terminal with 256 colours, NO_COLOR is unset
 * and TERM is not dumb; plain text otherwise, with nothing in between.
 * The look is sober and the same everywhere: a verdict, and the WARN or
 * ERROR in front of a message, is a bold word in a muted tone; a test or
 * a file named inside running text is bold, so that it is not drowned
 * out, while one that a line is about -- a verdict's test, a listing's
 * entry -- is in the text's plain weight; an explanation is grey, with a
 * command to type standing out in the text's own colour; nothing has a
 * background.
 */

#ifndef URT_REPORT_H
#define URT_REPORT_H

/* Lets the compiler check printf-style arguments, where it can. */
#ifdef __GNUC__
#define URT_PRINTF(fmt_at, args_at) __attribute__((format(printf, fmt_at, args_at)))
#else
#define URT_PRINTF(fmt_at, args_at)
#endif

enum style {
    ST_OK, ST_BAD, ST_WARN, ST_NEW,           /* ok; FAIL and ERROR; UPDATED and WARN; CREATED and EDITED */
    ST_INFO,                                  /* an explanation, a hint, a note */
    ST_CASE, ST_PATH,                         /* a test, a file, named inside running text */
    ST_SUBJECT,                               /* the test or file a line is about, on its own */
    ST_CMD, ST_KEY,                           /* a command line to type; an option or a key */
    ST_STRONG, ST_GREEN, ST_RED, ST_AMBER,    /* the summary's count and its tallies */
    ST_RESET
};

/* Decide on colour.  Also makes stdout line-buffered, so that what goes
 * to stdout and to stderr comes out in the order it was said. */
void report_init(void);

/* The escape sequence for a style, or "" without colour. */
const char *style(enum style s);

/* Text for a message: formatted, or a fragment in a style.  Each returns
 * one of a ring of buffers, good until 32 more have been asked for, which
 * is enough to build one message. */
const char *fmt(const char *format, ...) URT_PRINTF(1, 2);
const char *test(const char *s);     /* a snapshot or a case named in running text */
const char *path(const char *s);     /* any other file or directory named in running text */
const char *subject(const char *s);  /* the test or file a line is about */
const char *cmd(const char *s);
const char *key(const char *s);

/* A whole line grey, resuming grey after any styled fragment inside it. */
const char *info(const char *s);

/* A verdict line on stdout: the verdict, the test it is about, and
 * anything worth adding. */
void say(enum style s, const char *verdict, const char *test_dir, const char *note);
/* On GitHub Actions (GITHUB_ACTIONS=true), a failure as a workflow command,
 * which GitHub shows as an annotation of the run: on FILE (none: the run
 * as a whole) at the first line the unified DIFF changes, with the start of
 * DIFF, or else TITLE, as its text.  Elsewhere it does nothing. */
void annotate(const char *file, const char *title, const char *diff);

/* A warning on stderr: WARN, then the message. */
void warn(const char *msg);

/* Something is wrong with the tests themselves rather than with the
 * compiler: say so on stderr -- ERROR, the message, then explanation
 * lines until NULL -- and exit 2. */
void die(const char *msg, ...);

/* The same complaint, but about one snapshot: report it as an ERROR line,
 * count it, and carry on with the others.  The run then fails at the end. */
void die_later(const char *msg, ...);

extern int report_failed;     /* die_later was called */
extern int report_problems;   /* how many times */

#endif
