// reproducible.c - Make the request-response behavior and database deltas
// reproducible for testing.
//
// Pass the following env vars to the application:
//
//   URWEB_REPRODUCIBLE_EPOCH         seconds since the epoch (an unsigned int)
//   URWEB_REPRODUCIBLE_PRNG_SEED     pseudo-random number generator seed (an unsigned int)
//   URWEB_REPRODUCIBLE_TASKS         first or last (default first); see below
//
// Each is independent of the others, and without them nothing changes.
// Reproducibility is somewhat at odds with threaded execution, but we leave it
// up to you to not run this with -t / URWEB_NUM_THREADS set to a value > 1.
//
// With the epoch pinned, the time is logical, and comes from outside.  A
// request can say when it is made, in milliseconds since the Unix epoch, in a
// header, UrWeb-Time; its handler's clock then reads that time.  A request
// without one is served at the epoch plus a second for every request before
// it.  Nothing happens on the wall clock: the periodic tasks and the expiry of
// clients are events on the logical clock, due at the epoch plus a multiple of
// a task's period, and a second after a client's timeout ran out.  They happen
// when the clock is moved past them, by a request -- those due before it are
// handled first, and those due at the same time first or last as
// URWEB_REPRODUCIBLE_TASKS says -- or by a request for /.clock (whatever
// the application's prefix), ?advance=T, which handles everything due before
// T, in order.  That request answers with when the next such event is due,
// `next T` or `next none`, so that whoever drives the clock knows where to
// stop next: a task's message on a channel is then sent at the time it was
// due, and advance=T+1 handles what is due at T and nothing later.

#include "config.h"

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "urweb.h"

static int epoch_pinned = 0, rand_pinned = 0, tasks_last = 0;
static int64_t start_time;
static uint64_t seed, requests = 0;

static _Thread_local uint64_t my_request = 0;
static _Thread_local int64_t my_time = -1;    // ms, when this thread was told it
static _Thread_local uint64_t my_rand_state;
static _Thread_local int my_rand_started = 0;

static void (*attempt_hook)(void) = NULL;

static int env_integer(const char *name, uint64_t *out) {
  char *s = getenv(name), *end;
  if (!s) { return 0; }

  errno = 0;
  *out = strtoull(s, &end, 10);

  if (errno || end == s || *end) {
    fprintf(stderr, "%s: '%s' is not an integer\n", name, s);
    exit(1);
  }

  return 1;
}

void uw_reproducible_init(void) {
  uint64_t n;

  if (env_integer("URWEB_REPRODUCIBLE_EPOCH", &n)) {
    epoch_pinned = 1;
    start_time = (int64_t)n;
  }

  if (env_integer("URWEB_REPRODUCIBLE_PRNG_SEED", &n)) {
    rand_pinned = 1;
    seed = n;
  }

  {
    char *s = getenv("URWEB_REPRODUCIBLE_TASKS");
    if (s && !strcmp(s, "last")) {
      tasks_last = 1;
    } else if (s && strcmp(s, "first")) {
      fprintf(stderr, "URWEB_REPRODUCIBLE_TASKS: '%s' is neither first nor last\n", s);
      exit(1);
    }
  }
}

int uw_reproducible_clock_pinned(void) {
  return epoch_pinned;
}

// The next pseudo-random number into *out, or 0 if there is no seed.
//
// SplitMix64: Steele, Lea and Flood, "Fast Splittable Pseudorandom Number Generators", OOPSLA 2014.
// The constants are due to Vigna <https://prng.di.unimi.it/splitmix64.c>.
static uint64_t splitmix64(uint64_t *state) {
  uint64_t z = (*state += 0x9e3779b97f4a7c15);
  z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
  z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
  return z ^ (z >> 31);
}

// The periodic tasks, while the clock is pinned: when each is due next, and
// whether it has been told to run.  A task's thread waits until it is; the
// thread that moves the clock tells it, and waits until it has run.
typedef struct {
  int64_t period, due;    // ms; due is INT64_MAX for a task that is due never again
  int granted;
} scheduled;

static scheduled *tasks = NULL;
static int n_tasks = 0;
static pthread_mutex_t sched_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sched_cond = PTHREAD_COND_INITIALIZER;

// A periodic task, every period seconds, to be run on the logical clock: its
// number, for the calls below, or -1 when the clock is not pinned and the task
// keeps to the wall clock.  Called before the tasks' threads start.
int uw_reproducible_periodic(int period) {
  if (!epoch_pinned) { return -1; }

  tasks = realloc(tasks, (n_tasks + 1) * sizeof *tasks);
  if (!tasks) {
    fprintf(stderr, "Out of memory scheduling a periodic task\n");
    exit(1);
  }
  // A period of 0 runs the task back to back on the wall clock; on a logical
  // clock that would never let the time move, so such a task runs once.
  tasks[n_tasks].period = period > 0 ? (int64_t)period * 1000 : INT64_MAX;
  tasks[n_tasks].due = start_time * 1000;
  tasks[n_tasks].granted = 0;
  return n_tasks++;
}

// In task i's thread: until it is its turn, whose time its clock then reads.
void uw_reproducible_periodic_wait(int i) {
  pthread_mutex_lock(&sched_mutex);
  while (!tasks[i].granted) {
    pthread_cond_wait(&sched_cond, &sched_mutex);
  }
  my_time = tasks[i].due;
  pthread_mutex_unlock(&sched_mutex);
}

// In task i's thread: it has run, and is due again a period later.
void uw_reproducible_periodic_done(int i) {
  pthread_mutex_lock(&sched_mutex);
  if (tasks[i].period == INT64_MAX || tasks[i].due > INT64_MAX - tasks[i].period) {
    tasks[i].due = INT64_MAX;
  } else {
    tasks[i].due += tasks[i].period;
  }
  tasks[i].granted = 0;
  pthread_cond_broadcast(&sched_cond);
  pthread_mutex_unlock(&sched_mutex);
}

static void run_task(int i) {
  pthread_mutex_lock(&sched_mutex);
  tasks[i].granted = 1;
  pthread_cond_broadcast(&sched_cond);
  while (tasks[i].granted) {
    pthread_cond_wait(&sched_cond, &sched_mutex);
  }
  pthread_mutex_unlock(&sched_mutex);
}

// The next event the server is due to handle by itself into *at, and which:
// task *task, or a client's expiry (-1).  At the same time a task comes first,
// and the tasks in the order they were declared.  0 if there is none.
static int next_event(uw_context ctx, int64_t *at, int *task) {
  int i, found = 0;
  int64_t e;

  pthread_mutex_lock(&sched_mutex);
  for (i = 0; i < n_tasks; i++) {
    if (tasks[i].due != INT64_MAX && (!found || tasks[i].due < *at)) {
      *at = tasks[i].due;
      *task = i;
      found = 1;
    }
  }
  pthread_mutex_unlock(&sched_mutex);

  if (uw_next_client_expiry(ctx, &e) && (!found || e < *at)) {
    *at = e;
    *task = -1;
    found = 1;
  }
  return found;
}

// Everything due before until, or at it too when inclusive, handled in order;
// ctx serves for the expunges and is left reset.  Returns whether it did
// anything.
static int catch_up(uw_context ctx, int64_t until, int inclusive) {
  int64_t at, saved = my_time, last = -1;
  int task, did = 0;

  while (next_event(ctx, &at, &task) && (at < until || (inclusive && at == until))) {
    did = 1;
    if (task >= 0) {
      run_task(task);
    } else {
      if (at == last) {
        break;      // nothing could be expunged; it will not change by trying again
      }
      last = at;
      my_time = at;
      uw_prune_clients(ctx);
      uw_reset(ctx);
    }
  }
  my_time = saved;
  return did;
}

// Called once for every request upon its arrival, with what its UrWeb-Time
// header says, if anything.  While the clock is pinned, the request is served
// at that time, or else at the epoch plus a second for every request before
// it, and everything due before then is handled first, with ctx.  Returns
// whether ctx was used.
int uw_reproducible_request(uw_context ctx, const char *time_header) {
  int64_t t;
  int used;

  my_request = __atomic_add_fetch(&requests, 1, __ATOMIC_SEQ_CST);
  my_time = -1;

  if (!epoch_pinned) { return 0; }

  if (time_header) {
    char *end;
    errno = 0;
    t = strtoll(time_header, &end, 10);
    if (errno || end == time_header || *end) {
      fprintf(stderr, "UrWeb-Time: '%s' is not a number of milliseconds; the request is served at the time of the requests counted\n",
              time_header);
      time_header = NULL;
    }
  }
  if (!time_header) {
    uw_reproducible_now(&t);
  }
  used = catch_up(ctx, t, !tasks_last);
  my_time = t;
  return used;
}

// A request for .clock: with ?advance=T in its query string, everything due
// before T is handled first, with ctx; then when the next event is due goes into
// buf (size), the answer.  Not a request of the application's: it takes no
// number, so the numbers of the others stay as they were.
void uw_reproducible_clock(uw_context ctx, const char *query, char *buf, size_t size) {
  int64_t at;
  int task;
  const char *a = query ? strstr(query, "advance=") : NULL;

  my_time = -1;
  if (a && (a == query || a[-1] == '&')) {
    char *end;
    long long t;
    errno = 0;
    t = strtoll(a + 8, &end, 10);
    if (errno || end == a + 8 || (*end && *end != '&')) {
      snprintf(buf, size, "error: advance takes a number of milliseconds\n");
      return;
    }
    catch_up(ctx, t, 0);
  }
  if (next_event(ctx, &at, &task)) {
    snprintf(buf, size, "next %lld\n", (long long)at);
  } else {
    snprintf(buf, size, "next none\n");
  }
}

// What a database backend has to do at the start of every attempt while the
// numbers are pinned, so that what it draws from its own generator depends on
// the attempt alone: SQLite seeds its generator once, from the VFS, and runs on
// from there, so it is made to forget its seed and draw a fresh one from the
// attempt's numbers.  The generator is one per process, which is one more
// reason to pin the numbers only with a single thread.
void uw_reproducible_on_attempt(void (*hook)(void)) {
  attempt_hook = hook;
}

// Called right at the beginning of every attempt at handling a request, to make
// sure every potential retry of the request gets the same random numbers.
void uw_reproducible_attempt(void) {
  uint64_t state = seed + (my_request - 1) * 0x9e3779b97f4a7c15;
  my_rand_state = splitmix64(&state);
  my_rand_started = 1;

  if (rand_pinned && attempt_hook) { attempt_hook(); }
}

// The pinned time in milliseconds into *out, or 0 if the time is not pinned:
// what this thread was told, or else the time of the requests counted.
int uw_reproducible_now(int64_t *out) {
  uint64_t n;

  if (!epoch_pinned) { return 0; }

  if (my_time >= 0) {
    *out = my_time;
    return 1;
  }
  n = my_request ? my_request : __atomic_load_n(&requests, __ATOMIC_SEQ_CST);
  *out = (start_time + (int64_t)(n ? n - 1 : 0)) * 1000;
  return 1;
}

// The same in whole seconds.
int uw_reproducible_epoch(int64_t *out) {
  int64_t ms;

  if (!uw_reproducible_now(&ms)) { return 0; }

  *out = ms >= 0 ? ms / 1000 : -((-ms + 999) / 1000);
  return 1;
}

int uw_reproducible_rand_pinned(void) {
  return rand_pinned;
}

int uw_reproducible_rand(uint64_t *out) {
  if (!rand_pinned) { return 0; }

  if (!my_rand_started) {
    my_rand_state = seed;
    my_rand_started = 1;
  }

  *out = splitmix64(&my_rand_state);
  return 1;
}
