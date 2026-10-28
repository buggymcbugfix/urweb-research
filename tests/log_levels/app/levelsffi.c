#include <string.h>
#include <unistd.h>
#include <urweb.h>
#include "levelsffi.h"

// Sleeps for that many milliseconds, in the request's thread.
uw_unit uw_Levelsffi_wait(uw_context ctx, uw_Basis_int ms) {
  (void)ctx;
  usleep((useconds_t)ms * 1000);
  return 0;
}

// A string of that many x's, in one piece: concatenating them one by one
// in Ur/Web would copy the string over and over.
uw_Basis_string uw_Levelsffi_xs(uw_context ctx, uw_Basis_int n) {
  char *s = uw_malloc(ctx, n + 1);
  memset(s, 'x', n);
  s[n] = 0;
  return s;
}
