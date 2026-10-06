#include <string.h>
#include <unistd.h>
#include <urweb.h>
#include "race.h"

/* Sleep ms the first time a name is seen, never again: a retried request does not wait. */
uw_unit uw_Race_nap(uw_context ctx, uw_Basis_string name, uw_Basis_int ms) {
  static char seen[8][32];
  static int n = 0;
  int i;
  (void)ctx;
  for (i = 0; i < n; ++i)
    if (!strcmp(seen[i], name))
      return uw_unit_v;
  if (n < 8) {
    strncpy(seen[n], name, 31);
    ++n;
  }
  usleep(ms * 1000);
  return uw_unit_v;
}
