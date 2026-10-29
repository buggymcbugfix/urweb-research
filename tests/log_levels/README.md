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
- HTTP without flags, with `-q`, with `-l debug` and with a name that is
  not a level, and with its standard error connected to the journal.
- CGI and static, one page per run, with no option, as a web server
  starts them: stdout is their response.

Each output's lines are printed sorted, each once with how often it came,
since the lines the threads print at start-up come in no fixed order; a
line of x's is shown with its count of them.
