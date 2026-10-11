/* runs.h -- what `urt snapshot` needs to run an application and talk to
 * it: reading a request file, and starting and stopping the server.
 */

#ifndef URT_RUNS_H
#define URT_RUNS_H

#include <stddef.h>

#include "fs.h"

/* ------------------------------------------------------------- requests */

/* A request file is a method, a path, and then options in curl's
 * spelling, of which only these are taken:
 *
 *   POST /Shop/add
 *       -d 'name=kettle'        --data: one field of the body
 *       -d 'price=12'
 *       -b 'visitor=somebody'   --cookie: a cookie, NAME=VALUE; several
 *                               go in one Cookie header, joined by "; "
 *       -H 'Accept: text/html'  --header: a header, NAME: VALUE
 *
 * Words are split as a shell would: blanks and newlines separate, '...'
 * is literal, "..." is literal but for \" and \\, a backslash elsewhere
 * takes the next character as it is.  Nothing is expanded, and curl's
 * own escapes from the command line (-d @file, -b file, -H @file) are
 * refused: a request says what is sent, all of it.  So are the headers
 * urt sets for every request, Host and User-Agent, under any spelling
 * of the name; a header or cookie with a control character in it, which
 * could not go in one header line; and a path with a blank, a control
 * character or a # in it, since curl would send something else.
 *
 * 0, with the method, the path and the options as curl takes them
 * ("-d", "name=kettle", ...); or -1 with the complaint in err. */
int parse_request(const char *text, char *method, size_t method_size, char *url_path, size_t path_size,
                  struct strlist *options, char *err, size_t err_size);

/* --------------------------------------------------------------- servers */

struct server { long pid; int port; };

/* Start exe, an Ur/Web application with the built-in HTTP server, with
 * args and then the flags that make it listen on a free port of the
 * loopback interface and go to the background once it does.  Its
 * environment is this process's.  stdout and stderr go to the files
 * named.  0 with the server filled in, or -1: the files say why. */
int server_start(const char *exe, const struct strlist *args, const char *out_path, const char *err_path,
                 struct server *s);

/* Stop it and wait until it is gone.  Safe to call twice. */
void server_stop(struct server *s);

#endif
