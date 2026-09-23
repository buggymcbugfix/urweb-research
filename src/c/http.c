#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <stdarg.h>
#include <sys/un.h>

#include <pthread.h>

#include "urweb.h"
#include "request.h"
#include "queue.h"

extern uw_app uw_application;

int uw_backlog = SOMAXCONN;
static int keepalive = 0, quiet = 0;

#define qfprintf(f, fmt, args...) do { if(!quiet) fprintf(f, fmt, ##args); } while(0)
#define qprintf(fmt, args...) do { if(!quiet) printf(fmt, ##args); } while(0)

static char *get_header(void *data, const char *h) {
  char *s = data;
  int len = strlen(h);
  char *p;

  while ((p = strchr(s, ':'))) {
    if (p - s == len && !strncasecmp(s, h, len)) {
      return p + 2;
    } else {
      if ((s = strchr(p, 0)) && s[1] != 0)
        s += 2;
      else
        return NULL;
    }
  }
  
  return NULL;
}

static char *get_env(void *data, const char *name) {
  (void)data;
  return getenv(name);
}

static void on_success(uw_context ctx) {
  uw_write_header(ctx, "HTTP/1.1 200 OK\r\n");
}

static void on_failure(uw_context ctx) {
  uw_write_header(ctx, "HTTP/1.1 500 Internal Server Error\r\n");
}

static void log_error(void *data, const char *fmt, ...) {
  (void)data;

  va_list ap;
  va_start(ap, fmt);

  vfprintf(stderr, fmt, ap);
  fflush(stderr);
}

static void log_debug(void *data, const char *fmt, ...) {
  (void)data;

  if (!quiet) {
    va_list ap;
    va_start(ap, fmt);

    vprintf(fmt, ap);
    fflush(stdout);
  }
}

static uw_loggers ls = {NULL, log_error, log_debug};

static unsigned max_buf_size = 300 * 1024 * 1024; // That's 300MB.

static void *worker(void *data) {
  int me = *(int *)data;
  uw_context ctx = uw_request_new_context(me, &uw_application, &ls);
  size_t buf_size = 1024;
  char *buf = malloc(buf_size), *back = buf;
  uw_request_context rc = uw_new_request_context();
  int sock = 0;

  while (1) {
    if (sock == 0) {
      back = buf;
      sock = uw_dequeue();
    }

    uw_set_remoteSock(ctx, sock);

    qprintf("Handling connection with thread #%d.\n", me);

    while (1) {
      int r;
      char *method, *path, *query_string, *headers, *body, *after, *s, *s2;

      if (back - buf == buf_size - 1) {
        char *new_buf;
        size_t new_buf_size = buf_size*2;
        if (new_buf_size > max_buf_size) {
          qfprintf(stderr, "HTTP input exceeds buffer-size limit of %u bytes.\n", max_buf_size);
          close(sock);
          sock = 0;
          break;
        }
        new_buf = realloc(buf, new_buf_size);
        if(!new_buf) {
          qfprintf(stderr, "Realloc failed while receiving header\n");
          close(sock);
          sock = 0;
          break;
        }
        buf_size = new_buf_size;
        back = new_buf + (back - buf);
        buf = new_buf;
      }

      *back = 0;
      body = strstr(buf, "\r\n\r\n");
      if (body == NULL) {
        r = recv(sock, back, buf_size - 1 - (back - buf), 0);

        if (r < 0) {
          qfprintf(stderr, "Recv failed while receiving header, retcode %d errno %m\n", r);
          close(sock);
          sock = 0;
          break;
        }

        if (r == 0) {
          qprintf("Connection closed.\n");
          close(sock);
          sock = 0;
          break;
        }

        back += r;
        *back = 0;
      }

      if (body != NULL || (body = strstr(buf, "\r\n\r\n"))) {
        request_result rr;
        int should_keepalive = 0;

        body[0] = body[1] = 0;
        body += 4;

        if ((s = strcasestr(buf, "\r\nContent-Length: ")) && s < body) {
          int clen;

          if (sscanf(s + 18, "%d\r\n", &clen) != 1) {
            fprintf(stderr, "Malformed Content-Length header\n");
            close(sock);
            sock = 0;
            break;
          }

          while (back - body < clen) {
            if (back - buf == buf_size - 1) {
              char *new_buf;
              size_t new_buf_size = buf_size * 2;
              if (new_buf_size > max_buf_size) {
                qfprintf(stderr, "HTTP input exceeds buffer-size limit of %u bytes.\n", max_buf_size);
                close(sock);
                sock = 0;
                break;
              }
              new_buf = realloc(buf, new_buf_size);
              if(!new_buf) {
                qfprintf(stderr, "Realloc failed while receiving content\n");
                close(sock);
                sock = 0;
                goto done;
              }

              buf_size = new_buf_size;
              back = new_buf + (back - buf);
              body = new_buf + (body - buf);
              s = new_buf + (s - buf);

              buf = new_buf;
            }

            r = recv(sock, back, buf_size - 1 - (back - buf), 0);

            if (r < 0) {
              qfprintf(stderr, "Recv failed while receiving content, retcode %d errno %m\n", r);
              close(sock);
              sock = 0;
              goto done;
            }

            if (r == 0) {
              qfprintf(stderr, "Connection closed.\n");
              close(sock);
              sock = 0;
              goto done;
            }

            back += r;
            *back = 0;      
          }

          after = body + clen;
        } else
          after = body;

        body[-4] = '\r';
        body[-3] = '\n';

        if (!(s = strstr(buf, "\r\n"))) {
          fprintf(stderr, "No newline in request\n");
          close(sock);
          sock = 0;
          goto done;
        }

        body[-4] = body[-3] = 0;

        *s = 0;
        headers = s + 2;
        method = s = buf;

        strsep(&s, " ");
        if (!s) {
          fprintf(stderr, "No first space in HTTP command\n");
          close(sock);
          sock = 0;
          goto done;
        }
        path = s;

        if ((s = strchr(path, ' ')))
          *s = 0;

        if ((s = strchr(path, '?'))) {
          *s = 0;
          query_string = s+1;
        }
        else
          query_string = NULL;

        s = headers;
        while ((s2 = strchr(s, '\r'))) {
          if (s2 == s) {
            *s = 0;
            break;
          }

          s = s2;

          if (s[1] == 0)
            break;

          *s = 0;
          s += 2;
        }

        uw_set_headers(ctx, get_header, headers);
        uw_set_env(ctx, get_env, NULL);

        qprintf("Serving URI %s....\n", path);
        rr = uw_request(rc, ctx, method, path, query_string, body, back - body,
                        on_success, on_failure,
                        NULL, log_error, log_debug,
                        sock, uw_really_send, close);

        if (rr != KEEP_OPEN) {
          if (keepalive) {
            char *connection = uw_Basis_requestHeader(ctx, "Connection");

            should_keepalive = !(connection && !strcmp(connection, "close"));
          }

          if (!should_keepalive)
            uw_write_header(ctx, "Connection: close\r\n");

          if (!uw_has_contentLength(ctx)) {
            char clen[100];

            sprintf(clen, "Content-length: %d\r\n", uw_pagelen(ctx));
            uw_write_header(ctx, clen);
          }

          uw_send(ctx, sock);
        }

        if (rr == SERVED || rr == FAILED) {
          if (should_keepalive) {
            // In case any other requests are queued up, shift
            // unprocessed part of buffer to front.
            int kept = back - after;

            if (kept == 0) {
              // No pipelining going on here.
              // We'd might as well try to switch to a different connection,
              // while we wait for more input on this one.
              uw_enqueue(sock);
              sock = 0;
            } else {
              // More input!  Move it to the front and continue in this loop.
              memmove(buf, after, kept);
              back = buf + kept;
            }
          } else {
            close(sock);
            sock = 0;
          }
        } else if (rr == KEEP_OPEN)
          sock = 0;
        else
          fprintf(stderr, "Illegal uw_request return code: %d\n", rr);

        break;
      }
    }

  done:
    uw_reset(ctx);
  }

  return NULL;
}

static void help(char *cmd) {
  printf(
    "Usage: %s [-p <port>] [-P <upper-bound port>] [-a <IPv4 address>] [-A <IPv6 address>] [-u <UNIX socket>] [-t <thread count>] [-m <bytes>] [-k] [-q] [-T SEC] [-d[fd]]\n"
      "The '-P' option picks the first free port in the range [p..P], where p is the value of '-p' (default 8080) and P is the value of '-P'.\n"
      "The '-k' option turns on HTTP keepalive.\n"
      "The '-q' option turns off some chatter on stdout.\n"
      "The '-T' option sets socket recv timeout (0 disables timeout, default is 5 sec).\n"
      "The '-m' sets the maximum size (in bytes) for any buffer used to hold HTTP data sent by clients.  (The default is 1 MB.)\n" // XXX: I don't think this is correct
      "The '-d' option daemonizes the server by forking once it is listening; when the optional <fd>\n"
      "         arg is provided, the parent first writes shell assignments for 'pid' and either     \n"
      "         'port' or 'socket' to file descriptor <fd>. Typical shell usage:                    \n"
      "                                                                                             \n"
      "             eval \"$(... -d3 3>&1 1>/dev/null)\"                                            \n"
      "             [ \"$status\" = 'OK' ] || { echo failed; exit 1 }                               \n"
    , cmd);
}

static void sigint(int signum) {
  (void)signum;
  printf("Exiting....\n");
  exit(0);
}

union uw_sockaddr {
  struct sockaddr sa;
  struct sockaddr_in ipv4;
  struct sockaddr_in6 ipv6;
  struct sockaddr_un un;
};

int main(int argc, char *argv[]) {
  // The skeleton for this function comes from Beej's sockets tutorial.
  int sockfd;  // listen on sock_fd
  union uw_sockaddr my_addr;
  union uw_sockaddr their_addr; // connector's address information
  socklen_t my_size = 0, sin_size;
  int yes = 1, uw_port = 8080, nthreads = 1, i, *names, opt;
  int recv_timeout_sec = 5;
  int port_upper = 0; // 0 => -P not given; otherwise inclusive upper bound for port allocation.
  int daemonize = 0; // -d: fork once we are listening, parent exits.
  int meta_fd = -1; // -1 => no -d argument given; otherwise the file descriptor the parent should write structured metadata to before exiting.

  signal(SIGINT, sigint);
  signal(SIGPIPE, SIG_IGN); 

  // default if not specified: IPv4 with my IP
  memset(&my_addr, 0, sizeof my_addr);
  my_addr.sa.sa_family = AF_INET;
  my_addr.ipv4.sin_addr.s_addr = INADDR_ANY; // auto-fill with my IP

  while ((opt = getopt(argc, argv, "hp:P:a:A:u:t:kqT:m:d::")) != -1) {
    switch (opt) {
    case '?':
      fprintf(stderr, "Unknown command-line option\n");
      help(argv[0]);
      return 1;

    case 'h':
      help(argv[0]);
      return 0;

    case 'p':
      uw_port = atoi(optarg);
      if (uw_port <= 0) {
        fprintf(stderr, "Invalid port number\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 'P':
      port_upper = atoi(optarg);
      if (port_upper <= 0) {
        fprintf(stderr, "Invalid upper-bound port number\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 'a':
      my_addr.sa.sa_family = AF_INET;
      if (!inet_pton(AF_INET, optarg, &my_addr.ipv4.sin_addr)) {
        fprintf(stderr, "Invalid IPv4 address\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 'A':
      my_addr.sa.sa_family = AF_INET6;
      if (!inet_pton(AF_INET6, optarg, &my_addr.ipv6.sin6_addr)) {
        fprintf(stderr, "Invalid IPv6 address\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 'u':
      my_addr.sa.sa_family = AF_UNIX;
      if (!strncpy(my_addr.un.sun_path, optarg, sizeof(my_addr.un.sun_path)-1)) {
        fprintf(stderr, "Invalid UNIX socket filename\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 't':
      nthreads = atoi(optarg);
      if (nthreads <= 0) {
        fprintf(stderr, "Invalid thread count\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 'k':
      keepalive = 1;
      break;

    case 'T':
      recv_timeout_sec = atoi(optarg);
      if (recv_timeout_sec < 0) {
        fprintf(stderr, "Invalid recv timeout\n");
        help(argv[0]);
        return 1;
      }
      break;

    case 'q':
      quiet = 1;
      break;

    case 'm':
      opt = atoi(optarg);
      if (opt <= 0) {
        fprintf(stderr, "Invalid maximum buffer size\n");
        help(argv[0]);
        return 1;
      }
      max_buf_size = opt;
      break;

    case 'd':
      daemonize = 1;
      if (optarg) {
        // strtol so we catch trailing junk that atoi would silently accept.
        char *end;
        long v = strtol(optarg, &end, 10);
        if (*optarg == '\0' || *end != '\0' || v < 0 || v > INT_MAX) {
          fprintf(stderr, "Invalid file descriptor for -d: '%s'\n", optarg);
          return 1;
        }
        meta_fd = (int)v;
      }
      break;

    default:
      fprintf(stderr, "Unexpected getopt() behavior\n");
      return 1;
    }
  }

  if (port_upper != 0) {
    if (port_upper <= uw_port) {
      fprintf(stderr, "-P upper bound must be strictly greater than -p lower bound\n");
      return 1;
    }
    if (my_addr.sa.sa_family == AF_UNIX) {
      fprintf(stderr, "-P is not applicable to UNIX-socket listeners\n");
      return 1;
    }
  }

  uw_request_init(&uw_application, &ls);

  names = calloc(nthreads, sizeof(int));

  sockfd = socket(my_addr.sa.sa_family, SOCK_STREAM, 0); // do some error checking!

  if (sockfd < 0) {
    fprintf(stderr, "Listener socket creation failed\n");
    return 1;
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0) {
    fprintf(stderr, "Listener socket option setting failed\n");
    return 1;
  }

  switch (my_addr.sa.sa_family)
  {
  case AF_INET:
    my_size = sizeof(my_addr.ipv4);
    my_addr.ipv4.sin_port = htons(uw_port);
    break;

  case AF_INET6:
    my_size = sizeof(my_addr.ipv6);
    my_addr.ipv6.sin6_port = htons(uw_port);
    break;

  case AF_UNIX:
    unlink(my_addr.un.sun_path);
    my_size = sizeof(my_addr.un);
    break;
  }

  if (port_upper == 0) {
    if (bind(sockfd, &my_addr.sa, my_size) < 0) {
      fprintf(stderr, "Listener socket bind failed\n");
      return 1;
    }
  } else {
    // -P was given; try each port in [uw_port..port_upper] until one binds.
    // AF_UNIX was already ruled out above, so only the two IP families apply here.
    int port;
    for (port = uw_port; port <= port_upper; ++port) {
      if (my_addr.sa.sa_family == AF_INET)
        my_addr.ipv4.sin_port = htons(port);
      else
        my_addr.ipv6.sin6_port = htons(port);
      if (bind(sockfd, &my_addr.sa, my_size) == 0)
        break;
    }
    if (port > port_upper) {
      fprintf(stderr, "No free port found in range [%d..%d]\n", uw_port, port_upper);
      return 1;
    }
    uw_port = port; // so the existing "Listening on port %d" message reports the chosen one.
  }

  if (listen(sockfd, uw_backlog) < 0) {
    fprintf(stderr, "Socket listen failed\n");
    return 1;
  }

  sin_size = sizeof their_addr;

  qprintf("Starting the Ur/Web native HTTP server, which is intended for use\n"
          "ONLY DURING DEVELOPMENT.  You probably want to use one of the other backends,\n"
          "behind a production-quality HTTP server, for a real deployment.\n\n");

  // -d: daemonize. The fork MUST happen before any pthread_create call: fork()
  // only carries the calling thread, so any mutexes held by the others would
  // remain locked forever in the child. Doing it here (after listen() succeeds)
  // also means the parent's successful exit is a synchronous "ready to accept"
  // signal for the caller.
  //
  // -d <fd>: in addition, the parent writes `eval`-friendly shell assignments
  // for `pid` and either `port` or `socket` to file descriptor <fd> before
  // exiting. The caller is expected to set up <fd> with shell redirection,
  // e.g. `eval "$(... -d3 3>&1 1>/dev/null)"`.
  if (daemonize) {
    pid_t child = fork();
    if (child < 0) {
      fprintf(stderr, "fork failed: %m\n");
      return 1;
    }
    if (child > 0) {
      qprintf("Forking to PID %d....\n", child);
      if (meta_fd >= 0) {
        // Use fdopen+fprintf rather than write() so we can format integers
        // without manual itoa. "w" is fine: fdopen does not truncate or
        // re-open; it just wraps the existing fd. fclose flushes the buffer
        // and closes the underlying fd, which is desirable: it releases our
        // hold on the caller's pipe so their `eval $(...)` consumer unblocks.
        FILE *mf = fdopen(meta_fd, "w");
        int ok = mf != NULL;
        if (ok) {
          ok = fprintf(mf, "pid=%ld\n", (long)child) >= 0;
          if (ok) {
            if (my_addr.sa.sa_family == AF_UNIX) {
              ok = fprintf(mf, "socket='%s'\n", my_addr.un.sun_path) >= 0; 
            }
            else {
              ok = fprintf(mf, "port=%d\n", uw_port) >= 0;
            }
          }
          if (ok) {
            ok = fprintf(mf, "status='OK'\n") >= 0;
          }
          if (fclose(mf) != 0) {
            ok = 0;
          }
        }
        if (!ok) {
          fprintf(stderr, "Failed to write metadata to fd %d: %m\n", meta_fd);
          kill(child, SIGTERM);
          return 1;
        }
      }
      return 0;
    }
    if (meta_fd >= 0) close(meta_fd);
    setvbuf(stdout, NULL, _IOLBF, 0);
  }

  qprintf("Listening on port %d....\n", uw_port);

  {
    pthread_t thread;

    pruner_data *pd = (pruner_data *)malloc(sizeof(pruner_data));
    pd->app = &uw_application;
    pd->loggers = &ls;

    if (pthread_create_big(&thread, NULL, client_pruner, pd)) {
      fprintf(stderr, "Error creating pruner thread\n");
      return 1;
    }
  }

  for (i = 0; i < nthreads; ++i) {
    pthread_t thread;    
    names[i] = i;
    if (pthread_create_big(&thread, NULL, worker, &names[i])) {
      fprintf(stderr, "Error creating worker thread #%d\n", i);
      return 1;
    }
  }

  while (1) {
    int new_fd = accept(sockfd, &their_addr.sa, &sin_size);

    if (new_fd < 0) {
      qfprintf(stderr, "Socket accept failed\n");
    } else {
      qprintf("Accepted connection.\n");

      if (keepalive) {
        int flag = 1; 
        setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
      }

      if(recv_timeout_sec>0) {
        int ret;
        struct timeval tv;
        memset(&tv, 0, sizeof(struct timeval));
        tv.tv_sec = recv_timeout_sec;
        ret = setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval));
        if(ret != 0) {
          qfprintf(stderr, "Timeout setting failed, errcode %d errno '%m'\n", ret);
        }
      }

      uw_enqueue(new_fd);
    }
  }
}

void *uw_init_client_data() {
  return NULL;
}

void uw_free_client_data(void *data) {
  (void)data;
}

void uw_copy_client_data(void *dst, void *src) {
  (void)dst;
  (void)src;
}

void uw_do_expunge(uw_context ctx, uw_Basis_client cli, void *data) {
  (void)data;

  uw_ensure_transaction(ctx);
  uw_get_app(ctx)->expunger(ctx, cli);

  if (uw_commit(ctx))
    uw_error(ctx, UNLIMITED_RETRY, "Rerunning expunge transaction");
}

void uw_post_expunge(uw_context ctx, void *data) {
  (void)ctx;
  (void)data;
}

int uw_supports_direct_status = 1;
