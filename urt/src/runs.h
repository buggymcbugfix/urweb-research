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

/* How long a server gets to say it is listening, and a FastCGI program's
 * socket to be there.  The real one has its database to open and its
 * initialize task to run first; a hung one would otherwise hang the run
 * for good. */
#define START_SECONDS 10

/* A server: its process, the port it listens on (0 for a socket), and
 * whether it is this process's child, to be reaped when it is stopped. */
struct server { long pid; int port; int ours; };

/* Start exe, an Ur/Web application with the built-in HTTP server, with
 * args and then the flags that make it listen on a free port of the
 * loopback interface and go to the background once it does.  Its
 * environment is this process's.  stdout and stderr go to the files
 * named.  0 with the server filled in, or -1 with what happened in
 * why: how it exited, what its handshake lacks, or that it said nothing
 * within ten seconds. */
int server_start(const char *exe, const struct strlist *args, const char *out_path, const char *err_path,
                 struct server *s, char *why, size_t why_size);

/* Start exe, a FastCGI program, with args, as spawn-fcgi starts one in
 * production: a Unix socket at sock_path, listening, as its descriptor 0,
 * where it takes its connections from.  spawn-fcgi has to be on the PATH.
 * stdout and stderr go to the files named.  It is started and not waited
 * for: nothing says when the socket is there, nor when the program is
 * ready, and a connection made before the one fails while one made
 * before the other waits in the socket's queue.  0 with the server filled
 * in, or -1 with why. */
int fastcgi_start(const char *exe, const struct strlist *args, const char *sock_path, const char *out_path,
                  const char *err_path, struct server *s, char *why, size_t why_size);

/* What came of a request besides its status: the id of the first record
 * that was for another request, -1 when every record was for this one;
 * the protocolStatus of its FCGI_END_REQUEST, 0 (FCGI_REQUEST_COMPLETE)
 * unless the program refused the request; and whether it went on a new
 * connection because the program had closed the one kept for it. */
struct fastcgi_oddity { int request_id; int protocol_status; int reconnected; };

/* The id urt gives the one request a connection carries. */
#define FCGI_REQUEST_ID 1

/* What fastcgi_request returns besides a program's status for a request. */
#define FASTCGI_NO_END (-1)       /* the connection ended before FCGI_END_REQUEST */
#define FASTCGI_REFUSED (-2)      /* there was no connection to be had */
#define FASTCGI_TIMEOUT (-3)      /* no FCGI_END_REQUEST within the time given */
#define FASTCGI_UNWRITABLE (-4)   /* the response or stream file could not be written */

/* Send one request to the FastCGI program listening at sock_path, as a web
 * server would: FCGI_BEGIN_REQUEST for a responder, then params, a list of
 * NAME=value, as its parameters, and what the file body holds as its stdin,
 * all in one write.  It goes on *conn, or on a new connection when that is
 * -1 or when the program has closed the one kept, as a web server checks a
 * kept connection before reusing it.  With keep, the request asks the
 * program to keep the connection open (FCGI_KEEP_CONN), and after
 * FCGI_END_REQUEST it stays in *conn for the next request; otherwise, or
 * when it has ended, it is closed and *conn is -1.  What the program
 * answers on its stdout stream goes
 * into the file response, and what it logs on its stderr stream, which a
 * web server puts in its error log, into the file stream.  Both files are
 * written, empty if nothing came.  Returns the program's status for the
 * request from its FCGI_END_REQUEST, as a process's exit status would have
 * it: the low byte, so 255 for Ur/Web's failure; or one of the FASTCGI_
 * values above.  What was odd about the answer goes into *odd.  The
 * answer has seconds to come. */
int fastcgi_request(const char *sock_path, int *conn, int keep, const struct strlist *params, const char *body,
                    const char *response, const char *stream, int seconds, struct fastcgi_oddity *odd);

/* Whether a server started as our child (fastcgi_start) has exited: then
 * it is reaped, the server forgotten, and its exit status (128 + the
 * signal for one killed) is in *status. */
int server_exited(struct server *s, int *status);

/* Stop it and wait until it is gone.  Safe to call twice. */
void server_stop(struct server *s);

/* Whether it is still there: running, and not dead and unreaped. */
int server_alive(const struct server *s);

#endif
