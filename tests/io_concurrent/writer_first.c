/* An LD_PRELOAD shim for run.sh: pthread read-write locks that let a
   waiting writer go first, as macOS's do.  Whether a reader may take the
   lock while a writer waits for it is left to the implementation by POSIX,
   and glibc lets it, so on Linux a lock's behaviour on a Mac does not show.
   With the shim it does: pthread_rwlock_rdlock waits while any writer waits
   for the same lock, and then takes it with glibc's tryrdlock.  Only the two
   calls the runtime makes are changed; the waiting writers are counted per
   lock, in a table that only grows, which is enough for a test.

     cc -shared -fPIC -o writer_first.so writer_first.c -ldl */

#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define SLOTS 1024

static struct { pthread_rwlock_t *lock; int writers_waiting; } slots[SLOTS];
static pthread_mutex_t slots_mutex = PTHREAD_MUTEX_INITIALIZER;
static int (*real_wrlock)(pthread_rwlock_t *);
static int (*real_tryrdlock)(pthread_rwlock_t *);

__attribute__((constructor)) static void find_real(void) {
  real_wrlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_wrlock");
  real_tryrdlock = (int (*)(pthread_rwlock_t *))dlsym(RTLD_NEXT, "pthread_rwlock_tryrdlock");
}

/* The count for a lock, under slots_mutex. */
static int *writers_waiting(pthread_rwlock_t *l) {
  size_t i = ((uintptr_t)l >> 4) % SLOTS;
  while (slots[i].lock && slots[i].lock != l)
    i = (i + 1) % SLOTS;
  slots[i].lock = l;
  return &slots[i].writers_waiting;
}

int pthread_rwlock_wrlock(pthread_rwlock_t *l) {
  int r;
  pthread_mutex_lock(&slots_mutex);
  ++*writers_waiting(l);
  pthread_mutex_unlock(&slots_mutex);
  r = real_wrlock(l);
  pthread_mutex_lock(&slots_mutex);
  --*writers_waiting(l);
  pthread_mutex_unlock(&slots_mutex);
  return r;
}

int pthread_rwlock_rdlock(pthread_rwlock_t *l) {
  const struct timespec ms = {0, 1000000};
  for (;;) {
    int waiting, r;
    pthread_mutex_lock(&slots_mutex);
    waiting = *writers_waiting(l);
    pthread_mutex_unlock(&slots_mutex);
    if (!waiting && (r = real_tryrdlock(l)) != EBUSY)
      return r;
    nanosleep(&ms, NULL);
  }
}
