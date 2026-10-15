/* liburtclock: an engine's wall clock on the run's.
 *
 * urt-browse loads it into every process of the engines it starts
 * (LD_PRELOAD), so that what goes by the wall clock inside them -- the
 * expiry of cookies, first of all -- goes by the run's clock: the realtime
 * clock reads what the file URT_CLOCK_FILE holds, 8 bytes, a native int64,
 * nanoseconds since the epoch.  The file is mapped once, when the library
 * is loaded, and read at every call, so the driver moves the clock of every
 * process by writing those 8 bytes, and none lags behind.  Between the
 * driver's writes the clock stands still, as the run's does.  0 there, or
 * no file, and the real clock reads through.
 *
 * Only the wall clock is the run's.  The monotonic clocks, which timers,
 * timeouts and hang detectors measure with, are left alone: the engines'
 * own work goes on in real time, and the pages' timers are on the run's
 * clock already, through Playwright.  A wait with a deadline on the
 * realtime clock (pthread_cond_timedwait on a condition without a clock of
 * its own, sem_timedwait, ...) has the deadline moved from the run's clock
 * to the real one, keeping what is left of it: else a wait until now + 5s,
 * with now in 2100, would never end.  Not covered: realtime timers armed
 * with an absolute time (timer_settime, timerfd_settime), and the clock read
 * by a system call made directly.
 *
 * Linux and glibc only: the clock of a condition is read from glibc's
 * pthread_cond_t, where it has been since glibc 2.25. */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static volatile int64_t *clock_word;        /* the mapped file, or NULL */

static int (*real_clock_gettime)(clockid_t, struct timespec *);
static int (*real_gettimeofday)(struct timeval *, void *);
static time_t (*real_time)(time_t *);
static int (*real_timespec_get)(struct timespec *, int);
static int (*real_pthread_cond_timedwait)(pthread_cond_t *, pthread_mutex_t *, const struct timespec *);
static int (*real_pthread_cond_clockwait)(pthread_cond_t *, pthread_mutex_t *, clockid_t, const struct timespec *);
static int (*real_sem_timedwait)(sem_t *, const struct timespec *);
static int (*real_sem_clockwait)(sem_t *, clockid_t, const struct timespec *);
static int (*real_pthread_mutex_timedlock)(pthread_mutex_t *, const struct timespec *);
static int (*real_pthread_mutex_clocklock)(pthread_mutex_t *, clockid_t, const struct timespec *);
static int (*real_pthread_rwlock_timedrdlock)(pthread_rwlock_t *, const struct timespec *);
static int (*real_pthread_rwlock_timedwrlock)(pthread_rwlock_t *, const struct timespec *);
static int (*real_pthread_rwlock_clockrdlock)(pthread_rwlock_t *, clockid_t, const struct timespec *);
static int (*real_pthread_rwlock_clockwrlock)(pthread_rwlock_t *, clockid_t, const struct timespec *);
static int (*real_clock_nanosleep)(clockid_t, int, const struct timespec *, struct timespec *);

/* Looked up at the first call as well as at load: another library's
 * constructor may read the clock before this one has run. */
#define RESOLVE(f) do { if (!real_##f) { real_##f = dlsym(RTLD_NEXT, #f); } } while (0)

static void resolve_all(void) {
    RESOLVE(clock_gettime);
    RESOLVE(gettimeofday);
    RESOLVE(time);
    RESOLVE(timespec_get);
    RESOLVE(pthread_cond_timedwait);
    RESOLVE(pthread_cond_clockwait);
    RESOLVE(sem_timedwait);
    RESOLVE(sem_clockwait);
    RESOLVE(pthread_mutex_timedlock);
    RESOLVE(pthread_mutex_clocklock);
    RESOLVE(pthread_rwlock_timedrdlock);
    RESOLVE(pthread_rwlock_timedwrlock);
    RESOLVE(pthread_rwlock_clockrdlock);
    RESOLVE(pthread_rwlock_clockwrlock);
    RESOLVE(clock_nanosleep);
}

__attribute__((constructor)) static void urtclock_init(void) {
    const char *f = getenv("URT_CLOCK_FILE");
    struct stat st;
    int fd;
    void *p;
    resolve_all();
    if (!f || !*f || (fd = open(f, O_RDONLY | O_CLOEXEC)) < 0) {
        return;
    }
    /* A shorter file would fault at the first read. */
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

static int is_realtime(clockid_t id) {
    return id == CLOCK_REALTIME || id == CLOCK_REALTIME_COARSE || id == CLOCK_REALTIME_ALARM;
}

static int64_t real_now_ns(void) {
    struct timespec ts;
    RESOLVE(clock_gettime);
    real_clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t) ts.tv_sec * 1000000000 + ts.tv_nsec;
}

static void to_timespec(int64_t ns, struct timespec *ts) {
    ts->tv_sec = ns / 1000000000;
    ts->tv_nsec = ns % 1000000000;
    if (ts->tv_nsec < 0) {
        ts->tv_sec--;
        ts->tv_nsec += 1000000000;
    }
}

/* A realtime deadline given on the run's clock, on the real one: what is
 * left of it, from now. */
static const struct timespec *to_real(const struct timespec *abstime, struct timespec *buf) {
    int64_t f = faked();
    if (!f || !abstime) {
        return abstime;
    }
    to_timespec(real_now_ns() + ((int64_t) abstime->tv_sec * 1000000000 + abstime->tv_nsec - f), buf);
    return buf;
}

int clock_gettime(clockid_t id, struct timespec *ts) {
    int64_t f;
    RESOLVE(clock_gettime);
    if (is_realtime(id) && (f = faked())) {
        to_timespec(f, ts);
        return 0;
    }
    if (id == CLOCK_TAI && (f = faked())) {
        /* TAI keeps its offset to the realtime clock. */
        struct timespec tai, rt;
        int r = real_clock_gettime(CLOCK_TAI, &tai);
        if (r == 0 && real_clock_gettime(CLOCK_REALTIME, &rt) == 0) {
            to_timespec(f + ((int64_t) (tai.tv_sec - rt.tv_sec) * 1000000000 + (tai.tv_nsec - rt.tv_nsec)), ts);
        }
        return r;
    }
    return real_clock_gettime(id, ts);
}

int gettimeofday(struct timeval *tv, void *tz) {
    /* glibc declares tv nonnull; Linux takes NULL there, and so does this,
     * read through a volatile so that the test is not optimised away. */
    struct timeval *volatile out = tv;
    int64_t f;
    RESOLVE(gettimeofday);
    if ((f = faked())) {
        if (tz) {
            real_gettimeofday(NULL, tz);
        }
        if (out) {
            out->tv_sec = f / 1000000000;
            out->tv_usec = (f % 1000000000) / 1000;
        }
        return 0;
    }
    return real_gettimeofday(tv, tz);
}

time_t time(time_t *t) {
    int64_t f;
    RESOLVE(time);
    if ((f = faked())) {
        time_t s = f / 1000000000;
        if (t) {
            *t = s;
        }
        return s;
    }
    return real_time(t);
}

int timespec_get(struct timespec *ts, int base) {
    int64_t f;
    RESOLVE(timespec_get);
    if (base == TIME_UTC && (f = faked())) {
        to_timespec(f, ts);
        return base;
    }
    return real_timespec_get(ts, base);
}

/* A condition's clock: bit 1 of glibc's __wrefs, 0 realtime, 1 monotonic. */
static int cond_is_realtime(const pthread_cond_t *c) {
    return !((c->__data.__wrefs >> 1) & 1);
}

int pthread_cond_timedwait(pthread_cond_t *c, pthread_mutex_t *m, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_cond_timedwait);
    return real_pthread_cond_timedwait(c, m, cond_is_realtime(c) ? to_real(abstime, &buf) : abstime);
}

int pthread_cond_clockwait(pthread_cond_t *c, pthread_mutex_t *m, clockid_t id, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_cond_clockwait);
    return real_pthread_cond_clockwait(c, m, id, is_realtime(id) ? to_real(abstime, &buf) : abstime);
}

int sem_timedwait(sem_t *s, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(sem_timedwait);
    return real_sem_timedwait(s, to_real(abstime, &buf));
}

int sem_clockwait(sem_t *s, clockid_t id, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(sem_clockwait);
    return real_sem_clockwait(s, id, is_realtime(id) ? to_real(abstime, &buf) : abstime);
}

int pthread_mutex_timedlock(pthread_mutex_t *m, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_mutex_timedlock);
    return real_pthread_mutex_timedlock(m, to_real(abstime, &buf));
}

int pthread_mutex_clocklock(pthread_mutex_t *m, clockid_t id, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_mutex_clocklock);
    return real_pthread_mutex_clocklock(m, id, is_realtime(id) ? to_real(abstime, &buf) : abstime);
}

int pthread_rwlock_timedrdlock(pthread_rwlock_t *l, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_rwlock_timedrdlock);
    return real_pthread_rwlock_timedrdlock(l, to_real(abstime, &buf));
}

int pthread_rwlock_timedwrlock(pthread_rwlock_t *l, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_rwlock_timedwrlock);
    return real_pthread_rwlock_timedwrlock(l, to_real(abstime, &buf));
}

int pthread_rwlock_clockrdlock(pthread_rwlock_t *l, clockid_t id, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_rwlock_clockrdlock);
    return real_pthread_rwlock_clockrdlock(l, id, is_realtime(id) ? to_real(abstime, &buf) : abstime);
}

int pthread_rwlock_clockwrlock(pthread_rwlock_t *l, clockid_t id, const struct timespec *abstime) {
    struct timespec buf;
    RESOLVE(pthread_rwlock_clockwrlock);
    return real_pthread_rwlock_clockwrlock(l, id, is_realtime(id) ? to_real(abstime, &buf) : abstime);
}

int clock_nanosleep(clockid_t id, int flags, const struct timespec *req, struct timespec *rem) {
    struct timespec buf;
    RESOLVE(clock_nanosleep);
    if (is_realtime(id) && (flags & TIMER_ABSTIME)) {
        req = to_real(req, &buf);
    }
    return real_clock_nanosleep(id, flags, req, rem);
}
