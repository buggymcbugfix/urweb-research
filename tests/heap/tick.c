#include <urweb.h>
#include "tick.h"

uw_Basis_int uw_Tick_periodic(uw_context ctx) {
  static uw_Basis_int n = 0;
  (void)ctx;
  return ++n;
}

uw_Basis_bool uw_Tick_once(uw_context ctx, uw_Basis_string message) {
  static int done = 0;
  (void)ctx;
  (void)message;
  if (done)
    return uw_Basis_False;
  done = 1;
  return uw_Basis_True;
}
