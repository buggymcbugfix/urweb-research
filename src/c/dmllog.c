// dmllog.c - A log of what an application changes in its database (currently
// SQLite only!)
//
// Controlled via the env var
//
//   URWEB_DML_LOG   file to append DML statements to
//
// The file is opened in the commit phase of every transaction and closed again, so it can
// be renamed or removed at any time, by logrotate or by a test that wants to
// know what one request did.
// NB: With the current design, there is no (en|dis)abling this at runtime.

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "urweb.h"

static char global_name[] = "uw_dml_log";
static char *log_path = NULL;

void uw_dml_log_init(void) {
  char *s = getenv("URWEB_DML_LOG");
  log_path = s && *s ? s : NULL; // check that s is non-empty
}

int uw_dml_log_enabled(void) {
  return log_path != NULL;
}

static void flush(void *data) {
  uw_buffer *buf = data;
  size_t len = uw_buffer_used(buf);
  int fd;

  if (len == 0)
    return;

  fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0666);
  if (fd < 0 || uw_really_write(fd, buf->start, len) < 0) {
    fprintf(stderr, "URWEB_DML_LOG: cannot write %s: %s\n", log_path, strerror(errno));
  }
  if (fd >= 0) { close(fd); }
}

// After a commit or a rollback alike: the next transaction starts afresh.
static void forget(void *data, int will_retry) {
  (void)will_retry;
  uw_buffer_reset(data);
}

static void free_buffer(void *data) {
  uw_buffer_free(data);
  free(data);
}

// @param sql: a statement that has run in full
// @param loc: its source location and/or function name
// @pparam rows: how many rows it changed, or -1 if the backend cannot tell
void uw_log_dml(uw_context ctx, const char *loc, const char *sql, long rows) {
  uw_buffer *buf;
  char line[64];
  int failed = 0;

  if (!log_path) { return; }

  buf = uw_get_global(ctx, global_name);
  if (!buf) {
    buf = malloc(sizeof *buf);
    uw_buffer_init(SIZE_MAX, buf, 0);
    uw_set_global(ctx, global_name, buf, free_buffer);
  }

  if (uw_buffer_used(buf) == 0) {
    // The first statement of this transaction.
    time_t now = uw_Basis_now(ctx).seconds;
    struct tm tm;
    char *url = uw_Basis_currentUrl(ctx);

    if (uw_register_transactional(ctx, buf, flush, NULL, forget) < 0) {
      fprintf(stderr, "URWEB_DML_LOG: too many transactionals; not logging %s\n", sql);
      return;
    }

    gmtime_r(&now, &tm);
    strftime(line, sizeof line, "-- %Y-%m-%d %H:%M:%S UTC", &tm);
    failed |= uw_buffer_append(buf, line, strlen(line));
    if (url && *url) {
      const char *method = uw_Basis_currentUrlHasPost(ctx) ? " POST " : " GET ";
      failed |= uw_buffer_append(buf, method, strlen(method));
      failed |= uw_buffer_append(buf, url, strlen(url));
    }
    failed |= uw_buffer_append(buf, "\n", 1);
  }

  if (*loc || rows >= 0) {
    failed |= uw_buffer_append(buf, "--", 2);
    if (*loc) {
      failed |= uw_buffer_append(buf, " ", 1);
      failed |= uw_buffer_append(buf, loc, strlen(loc));
    }
    if (rows >= 0) {
      snprintf(line, sizeof line, " (%ld row%s)", rows, rows == 1 ? "" : "s");
      failed |= uw_buffer_append(buf, line, strlen(line));
    }
    failed |= uw_buffer_append(buf, "\n", 1);
  }
  failed |= uw_buffer_append(buf, sql, strlen(sql));
  failed |= uw_buffer_append(buf, ";\n", 2);

  if (failed) {
    fprintf(stderr, "URWEB_DML_LOG: out of memory; the log of this transaction is incomplete\n");
  }
}
