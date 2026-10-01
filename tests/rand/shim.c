/* LD_PRELOAD shim for tests/rand/run.sh: makes RAND_bytes misbehave as
 * $RAND_SHIM says, and otherwise calls the real one.
 *
 *   fail         return 0 with a reason in the error queue, as OpenSSL
 *                does when it cannot get entropy
 *   unsupported  return -1, the documented answer of a RAND_METHOD that
 *                does not support RAND_bytes.  The buffer is filled with
 *                0xab first, so that a caller which ignores the failure
 *                shows it: it does not read stack garbage, it reads that.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/err.h>
#include <openssl/randerr.h>

int RAND_bytes(unsigned char *buf, int num) {
  const char *mode = getenv("RAND_SHIM");

  if (mode && !strcmp(mode, "fail")) {
    ERR_raise_data(ERR_LIB_RAND, RAND_R_ERROR_RETRIEVING_ENTROPY, "injected by tests/rand/shim.c");
    return 0;
  }

  if (mode && !strcmp(mode, "unsupported")) {
    memset(buf, 0xab, num);
    return -1;
  }

  {
    int (*real)(unsigned char *, int) = (int (*)(unsigned char *, int))dlsym(RTLD_NEXT, "RAND_bytes");
    return real(buf, num);
  }
}
