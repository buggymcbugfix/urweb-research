/* liburtclock for macOS: what urtclock.c does on Linux, by dyld's
 * interposing.  urt-browse starts the engines with DYLD_INSERT_LIBRARIES
 * naming it, and __XPC_DYLD_INSERT_LIBRARIES too, for WebKit's helpers:
 * they are XPC services, which get from the environment only what
 * launchd hands them, the __XPC_ names, without the prefix.  The realtime
 * clock reads what the file URT_CLOCK_FILE holds, 8 bytes, a native
 * int64, nanoseconds since the epoch, mapped once and read at every call;
 * 0 there, or no file, and the real clock reads through.
 *
 * Untried as yet: make builds it only when asked for (make -C urt
 * liburtclock.dylib), and urt-browse uses it when it is there.  An engine
 * built with the hardened runtime and without leave to take DYLD_
 * variables does not load it.
 *
 * Every condition is on the realtime clock on macOS, there being no
 * pthread_condattr_setclock, so every timed wait on one has its deadline
 * moved from the run's clock to the real one, keeping what is left of it.
 * Not covered: dispatch_walltime and run loop timers given a date on the
 * wall clock, which would come late or never. */

#define _DARWIN_C_SOURCE
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

/* As dyld-interposing.h has it: calls to replacee from every other image
 * go to replacement; this image's own calls go where they did. */
#define INTERPOSE(replacement, replacee) \
    __attribute__((used)) static struct { const void *r; const void *e; } interpose_##replacee \
    __attribute__((section("__DATA,__interpose"))) = \
        { (const void *) (unsigned long) &replacement, (const void *) (unsigned long) &replacee }

static volatile int64_t *clock_word;        /* the mapped file, or NULL */

__attribute__((constructor)) static void urtclock_init(void) {
    const char *f = getenv("URT_CLOCK_FILE");
    struct stat st;
    int fd;
    void *p;
    if (!f || !*f || (fd = open(f, O_RDONLY | O_CLOEXEC)) < 0) {
        return;
    }
    if (fstat(fd, &st) == 0 && st.st_size >= (off_t) sizeof(int64_t)) {
        p = mmap(NULL, sizeof(int64_t), PROT_READ, MAP_SHARED, fd, 0);
        if (p != MAP_FAILED) {
            clock_word = p;
        }
    }
    close(fd);
}

/* The run's time in nanoseconds, or 0 for the real clock. */
static int64_t faked(void) {
    return clock_word ? __atomic_load_n(clock_word, __ATOMIC_ACQUIRE) : 0;
}

static void to_timespec(int64_t ns, struct timespec *ts) {
    ts->tv_sec = ns / 1000000000;
    ts->tv_nsec = ns % 1000000000;
    if (ts->tv_nsec < 0) {
        ts->tv_sec--;
        ts->tv_nsec += 1000000000;
    }
}

/* A realtime deadline given on the run's clock, on the real one. */
static const struct timespec *to_real(const struct timespec *abstime, struct timespec *buf) {
    int64_t f = faked();
    struct timespec now;
    if (!f || !abstime) {
        return abstime;
    }
    clock_gettime(CLOCK_REALTIME, &now);
    to_timespec((int64_t) now.tv_sec * 1000000000 + now.tv_nsec
                + ((int64_t) abstime->tv_sec * 1000000000 + abstime->tv_nsec - f), buf);
    return buf;
}

static int urt_clock_gettime(clockid_t id, struct timespec *ts) {
    int64_t f;
    if (id == CLOCK_REALTIME && (f = faked())) {
        to_timespec(f, ts);
        return 0;
    }
    return clock_gettime(id, ts);
}
INTERPOSE(urt_clock_gettime, clock_gettime);

static uint64_t urt_clock_gettime_nsec_np(clockid_t id) {
    int64_t f;
    if (id == CLOCK_REALTIME && (f = faked())) {
        return (uint64_t) f;
    }
    return clock_gettime_nsec_np(id);
}
INTERPOSE(urt_clock_gettime_nsec_np, clock_gettime_nsec_np);

static int urt_gettimeofday(struct timeval *tv, void *tz) {
    struct timeval *volatile out = tv;
    struct timeval ignored;
    int64_t f;
    if ((f = faked())) {
        if (tz) {
            gettimeofday(&ignored, tz);
        }
        if (out) {
            out->tv_sec = f / 1000000000;
            out->tv_usec = (f % 1000000000) / 1000;
        }
        return 0;
    }
    return gettimeofday(tv, tz);
}
INTERPOSE(urt_gettimeofday, gettimeofday);

static time_t urt_time(time_t *t) {
    int64_t f;
    if ((f = faked())) {
        time_t s = f / 1000000000;
        if (t) {
            *t = s;
        }
        return s;
    }
    return time(t);
}
INTERPOSE(urt_time, time);

static int urt_timespec_get(struct timespec *ts, int base) {
    int64_t f;
    if (base == TIME_UTC && (f = faked())) {
        to_timespec(f, ts);
        return base;
    }
    return timespec_get(ts, base);
}
INTERPOSE(urt_timespec_get, timespec_get);

static int urt_pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *abstime) {
    struct timespec buf;
    return pthread_cond_timedwait(c, m, to_real(abstime, &buf));
}
INTERPOSE(urt_pthread_cond_timedwait, pthread_cond_timedwait);
