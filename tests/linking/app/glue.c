#include "foo.h"

/* Defined in libfoo, which the link line names but this test never builds:
 * the link command is recorded, not run. */
extern int foo_add(int, int);

uw_Basis_int uw_Foo_add(uw_context ctx, uw_Basis_int a, uw_Basis_int b) {
    return foo_add(a, b);
}
