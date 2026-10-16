#include "config.h"

#include <stdio.h>
#include <stdarg.h>

#include "urweb.h"

extern uw_app uw_application;

static void log_(void *data, const char *fmt, ...) {
  (void)data;

  va_list ap;
  va_start(ap, fmt);

  vprintf(fmt, ap);
}

static uw_loggers loggers = {NULL, log_, log_};

static char *get_header(void *data, const char *h) {
  (void)data;
  (void)h;
  return NULL;
}

int main(int argc, char *argv[]) {
  uw_context ctx;
  failure_kind fk;

  if (argc != 2) {
    fprintf(stderr, "Pass exactly one argument: the URI to run\n");
    return 1;
  }
 
  // The global set-up, as in the other protocols: the project's limits, the
  // signing key, the reproducible clock, the DML log.
  uw_global_init();
  uw_app_init(&uw_application);

  ctx = uw_init(0, &loggers);
  uw_set_app(ctx, &uw_application);
  uw_set_headers(ctx, get_header, NULL);
  uw_initialize(ctx);

  // The page is the program's one request, numbered and attempted as a
  // server's would be (uw_request), so that its pinned clock and random
  // numbers are those the server gives that request.
  uw_reproducible_request();

  while (1) {
    uw_reproducible_attempt();
    fk = uw_begin(ctx, argv[1]);

    if (fk == SUCCESS || fk == RETURN_INDIRECTLY) {
      uw_commit(ctx);
      uw_print(ctx, 1);
      puts("");
      return 0;
    } else if (fk != UNLIMITED_RETRY) {
      fprintf(stderr, "Error: %s\n", uw_error_message(ctx));
      return 1;
    }

    uw_reset(ctx);
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
  (void)ctx;
  (void)cli;
  (void)data;
}

void uw_post_expunge(uw_context ctx, void *data) {
  (void)ctx;
  (void)data;
}

int uw_supports_direct_status = 0;
