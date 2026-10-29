Where a program's log lines go. `run.sh` builds one application for
FastCGI, HTTP, CGI and static; `client.py` starts each build with the
flags that choose what it logs, sends it requests that make it log (a
page that calls `debug`, one that calls `debug` and then fails, and for
FastCGI a request in a role the program refuses), and records what the
program wrote on stdout and on stderr.

- FastCGI without flags, with `-q`, with `-l` and each level's name, with
  a name that is not one, and with `-q` and `-l debug` in both orders.
- FastCGI with its standard error connected to the journal, as systemd
  connects it: `JOURNAL_STREAM` holds the device and inode of the file
  that standard error goes to, as systemd's variable holds those of its
  stream. Without flags and with `-l debug`.
- FastCGI, plain and with the journal, sent other things: a request
  whose web server closed the connection before the program started, as
  nginx closes one after its `fastcgi_read_timeout`; a page that logs a
  line longer than the buffer a log line is first formatted into; a page
  that logs two lines in one call and then one without a newline; forty
  requests at once for the long line, to four threads, so that the
  threads' lines are tried against each other; and a page that sleeps,
  whose web server leaves before it is done.
- FastCGI, plain and with the journal, sent what cannot be served as
  asked, with what the program answers: a page that is none, a
  constructor that is none in a URL argument, a bad escape in a string
  argument, a form handler without its field, a page that reads the POST
  body by GET, a PUT, a POST short of its CONTENT_LENGTH, and a poll for
  a client that is none.
- FastCGI, sent in one run what a request is refused for before any page
  runs, each on a connection of its own, with what the program answers,
  the reason among it: a multipart form that is malformed in each of the
  ways the program tells apart (and one that is not, for contrast), a poll
  without its client or its password, and a negative CONTENT_LENGTH, which
  the program does not answer at all.
- FastCGI, sent each method other than GET and POST, each on a connection
  of its own, with what the program answers: HEAD, OPTIONS, DELETE,
  TRACE, PATCH, CONNECT, GET in lower case, no method at all, and one
  with a newline in it.
- FastCGI, sent a GET with a negative CONTENT_LENGTH, with what the
  program answers.
- FastCGI with a `JOURNAL_STREAM` that does not name its standard error:
  another inode, another device, text after the inode, no numbers. The
  program is not taken to be under the journal then.
- FastCGI with `-q` and with `-l err`, sent the request whose web server
  has gone: the line about dropping it is a warning, shown by one and not
  by the other. And with `-l` twice, in both orders: the later counts.
- HTTP without flags, with `-q`, with `-l debug` and with a name that is
  not a level, and with its standard error connected to the journal.
- HTTP, in one run, sent what a request is refused for: a page that does
  not exist, a HEAD, a PUT, a bad escape, a multipart type without a
  boundary, a poll without its fields, a negative Content-Length, one
  that is no number and a request line without a space.
- HTTP with `-k`, sent a HEAD and then a GET in one write, on one
  connection: the status and length of the answer to the HEAD, and what
  follows the head of that answer, which a client that reads no body
  after a HEAD takes for the answer to the GET.
- CGI and static, one page per run, with no option, as a web server
  starts them: stdout is their response. The pages include one that does
  not exist and one with a bad escape. CGI is also sent a PUT and a POST
  with a negative CONTENT_LENGTH.

Each output's lines are printed sorted, each once with how often it came,
since the lines the threads print at start-up come in no fixed order; a
line of x's is shown with its count of them.
