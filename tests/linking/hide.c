/* Preloaded into the compiler by run.sh: access() answers ENOENT for every
 * path that ends in $HIDE, as for a file that is not there, and asks the C
 * library for the others. The preload is for the compiler alone: the
 * shell and the C compiler it starts may use another C library, which this
 * object cannot be loaded with. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

__attribute__((constructor)) static void for_this_process_only(void) {
    unsetenv("LD_PRELOAD");
}

int access(const char *path, int mode) {
    static int (*real)(const char *, int);
    const char *hide = getenv("HIDE");
    size_t n = strlen(path), m = hide ? strlen(hide) : 0;

    if (m > 0 && n >= m && strcmp(path + n - m, hide) == 0) {
        errno = ENOENT;
        return -1;
    }
    if (!real)
        real = (int (*)(const char *, int))dlsym(RTLD_NEXT, "access");
    return real(path, mode);
}
