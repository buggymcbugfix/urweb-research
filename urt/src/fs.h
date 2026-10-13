/* fs.h -- files, directories and child processes, as urt needs them.
 *
 * Paths are plain char buffers of PATHLEN; join() builds them and dies
 * when one would not fit.  Lists of names are struct strlist, sorted in
 * byte order where order matters, which is the order `LC_ALL=C sort`
 * gives and the one the snapshots were recorded in.
 */

#ifndef URT_FS_H
#define URT_FS_H

#include <stddef.h>

#include "report.h"

#define PATHLEN 4096

/* ---------------------------------------------------------------- lists */

struct strlist { char **v; int n, cap; };

void sl_push(struct strlist *l, const char *s);   /* a copy of s */
void sl_sort(struct strlist *l);                  /* byte order */
void sl_free(struct strlist *l);
int  sl_has(const struct strlist *l, const char *s);

/* The list's strings as a NULL-terminated array, for exec: the array is
 * the caller's to free, the strings stay the list's. */
char **sl_argv(const struct strlist *l);

/* ---------------------------------------------------------------- paths */

/* dst = a/b, or whichever alone when the other is empty.  Dies if it
 * would not fit. */
void join(char *dst, const char *a, const char *b);

/* dst = the formatted path.  Dies if it would not fit. */
void pathf(char *dst, const char *format, ...) URT_PRINTF(2, 3);

/* The part after the last slash, or the whole. */
const char *basename_of(const char *p);

/* The part before the last slash into dst, or "." when there is none. */
void dirname_of(char *dst, const char *p);

int has_suffix(const char *s, const char *suffix);
int has_prefix(const char *s, const char *prefix);

/* --------------------------------------------------------------- files */

int is_file(const char *p);    /* a regular file, symlinks followed, like test -f */
int is_dir(const char *p);     /* a directory, symlinks followed, like test -d */
int exists(const char *p);     /* like test -e */
int is_link(const char *p);    /* a symbolic link, like test -L */
long file_size(const char *p); /* in bytes, or -1 */
int can_exec(const char *p);   /* like test -x */

/* The whole file, NUL-terminated, malloc'd; NULL when it cannot be read. */
char *read_text(const char *p);

/* Create or truncate p, writing s (which may be empty).  0 when done. */
int write_text(const char *p, const char *s);

int mkdir_p(const char *p);
int rm_rf(const char *p);      /* p may not exist; 0 when gone */
int copy_file(const char *from, const char *to);
int move_file(const char *from, const char *to);   /* replacing to; permissions kept */
int same_contents(const char *a, const char *b);   /* byte for byte */

/* ---------------------------------------------------------- directories */

/* The entries of dir, sorted, without . and ..: all of them, or only the
 * directories, or only the regular files, judged without following
 * symlinks, as find -type does. */
enum { LS_ALL, LS_DIRS, LS_FILES };
void list_dir(const char *dir, int what, struct strlist *out);

/* Every path under dir, relative to it, sorted; symlinks not followed.
 * What `find dir -mindepth 1` prints, less the prefix. */
void walk(const char *dir, struct strlist *out);

/* The directories under dir (dir itself included) whose name ends in
 * suffix, as dir/..., sorted.  What `find dir -type d -name "*suffix"`
 * prints. */
void find_dirs(const char *dir, const char *suffix, struct strlist *out);

/* ------------------------------------------------------------ processes */

/* Run argv, its stdout and stderr to the files named (NULL: inherited),
 * and return its exit status as a shell would: 128 + the signal when it
 * was killed, 127 when it could not be run. */
int run(char *const argv[], const char *out_path, const char *err_path);

/* The same, with its stdin from the file named (NULL: inherited). */
int run_in(char *const argv[], const char *in_path, const char *out_path, const char *err_path);

/* The same as run(), for a program that reports on descriptor 3, as a
 * server started with -d3 does: what it and its offspring wrote there,
 * malloc'd, once all of them have closed it and the program itself has
 * exited, with its status in *status; NULL if it could not be started.
 * If the descriptor is not closed within the seconds given, what was
 * written so far comes back with *status = -1, and the program has
 * been killed (its offspring, if it has any by then, has not).  A
 * program that cannot be run exits 127 with the reason in err_path. */
char *run_fd3(char *const argv[], const char *out_path, const char *err_path, int seconds, int *status);

/* The same as run(), for a program that reports a line at a time on its
 * stdout as it goes: each line, without its newline, goes to fn as it
 * arrives, with data.  When a signal arrives the program is told to stop
 * (SIGTERM), what it still says is read, and the caller finds
 * fs_interrupted set.  Returns the exit status as run() does. */
int run_lines(char *const argv[], const char *err_path, void (*fn)(const char *line, void *data), void *data);

/* Whether the PATH has a program of that name. */
int on_path(const char *name);

/* Remembers argv[0], for program_dir. */
void fs_set_argv0(const char *argv0);

/* Set when SIGINT, SIGTERM, SIGHUP or SIGQUIT arrived; run() returns and the caller
 * decides. */
extern volatile int fs_interrupted;
void fs_catch_signals(void);

/* The directory this program was run from, resolved, into dst: where
 * argv[0] points, or where the PATH has a program of that bare name; 0
 * if neither tells. */
int program_dir(char *dst);

/* A fresh directory under $TMPDIR (or /tmp), its canonical path into dst. */
int make_tmpdir(char *dst);

/* Wall-clock time in tenths of a second, from some fixed point. */
long tenths_now(void);

/* s split at spaces, tabs, CRs and newlines, empties dropped: the shell's word
 * splitting of an unquoted variable. */
void split_words(const char *s, struct strlist *out);

void *xmalloc(size_t n);
char *xstrdup(const char *s);

#endif
