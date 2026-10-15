/* The probe of this test: see late.urs. */

#include <stdlib.h>
#include <string.h>
#include <urweb.h>
#include "late.h"

typedef struct {
  uw_context ctx;
  char *tag;
} late;

static void say(late *l, const char *what) {
  uw_loggers *ls = uw_get_loggers(l->ctx);
  ls->log_debug(ls->logger_data, "late transactional %s: %s\n", l->tag, what);
}

static void commit(void *d) {
  late *l = d;
  say(l, "commit, failing");
  uw_set_error_message(l->ctx, "%s", l->tag);
}

static void free_late(void *d, int will_retry) {
  late *l = d;
  (void)will_retry;
  say(l, "free");
  free(l->tag);
  free(l);
}

uw_unit uw_Late_failInCommit(uw_context ctx, uw_Basis_string tag) {
  late *l = malloc(sizeof *l);
  l->ctx = ctx;
  l->tag = strdup(tag);
  if (uw_register_transactional(ctx, l, commit, NULL, free_late))
    uw_error(ctx, FATAL, "too many transactionals");
  return uw_unit_v;
}
