#include <string.h>
#include <urweb.h>
#include "outc.h"

uw_Outc_outcome uw_Outc_send(uw_context ctx, uw_Basis_string s) {
  uw_Outc_outcome r = uw_malloc(ctx, sizeof(struct uw_Outc_outcome));
  if (!strcmp(s, "ok"))
    r->tag = uw_Outc_Sent;
  else if (!strcmp(s, "later")) {
    r->tag = uw_Outc_NotSent;
    r->data.uw_NotSent = uw_strdup(ctx, "try again");
  } else {
    r->tag = uw_Outc_Unknown;
    r->data.uw_Unknown = uw_strdup(ctx, "who knows");
  }
  return r;
}

uw_Basis_string uw_Outc_describe(uw_context ctx, uw_Outc_outcome o) {
  switch (o->tag) {
  case uw_Outc_Sent: return "C:sent";
  case uw_Outc_NotSent: return uw_Basis_strcat(ctx, "C:notsent:", o->data.uw_NotSent);
  default: return uw_Basis_strcat(ctx, "C:unknown:", o->data.uw_Unknown);
  }
}
