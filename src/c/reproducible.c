// reproducible.c - Make the request-response behavior and database deltas
// reproducible for testing.
//
// Pass the following env vars to the application:
//
//   URWEB_REPRODUCIBLE_EPOCH         seconds since the epoch (an unsigned int)
//   URWEB_REPRODUCIBLE_PRNG_SEED     pseudo-random number generator seed (an unsigned int)
//
// Each is independent of the other, and without them nothing changes.
// Reproducibility is somewhat at odds with threaded execution, but we leave it
// up to you to not run this with -t / URWEB_NUM_THREADS set to a value > 1.
// Recall that other parts of the runtime run in their own threads, such as
// periodic tasks and the reaper, giving another source of nondeterminism to be
// mindful of.

#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "urweb.h"

static int epoch_pinned = 0, rand_pinned = 0;
static int64_t start_time;
static uint64_t seed, requests = 0;

static _Thread_local uint64_t my_request = 0;
static _Thread_local uint64_t my_rand_state;
static _Thread_local int my_rand_started = 0;

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

// Called once for every request upon its arrival.
void uw_reproducible_request(void) {
  my_request = __atomic_add_fetch(&requests, 1, __ATOMIC_SEQ_CST);
}

// Called right at the beginning of every attempt at handling a request, to make
// sure every potential retry of the request gets the same random numbers.
void uw_reproducible_attempt(void) {
  uint64_t state = seed + (my_request - 1) * 0x9e3779b97f4a7c15;
  my_rand_state = splitmix64(&state);
  my_rand_started = 1;
}

// The pinned time into *out, or 0 if the time is not pinned.
int uw_reproducible_epoch(int64_t *out) {
  uint64_t n;

  if (!epoch_pinned) { return 0; }

  n = my_request ? my_request : __atomic_load_n(&requests, __ATOMIC_SEQ_CST);
  *out = start_time + (int64_t)(n ? n - 1 : 0);
  return 1;
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
