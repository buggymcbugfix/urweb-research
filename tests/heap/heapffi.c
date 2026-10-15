#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <urweb.h>
#include "heapffi.h"

static int aligned(void *p) {
  return (uintptr_t)p % sizeof(void *) == 0;
}

static const char *yesno(int b) {
  return b ? "yes" : "no";
}

// Every byte written, then read back; a fault here would be a crash, not
// a wrong answer.
static int writable(unsigned char *p, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i)
    p[i] = (unsigned char)(i * 7 + 1);
  for (i = 0; i < n; ++i)
    if (p[i] != (unsigned char)(i * 7 + 1))
      return 0;
  return 1;
}

uw_Basis_string uw_Heapffi_block(uw_context ctx, uw_Basis_int n) {
  unsigned char *p = uw_malloc(ctx, n);
  char buf[128];
  snprintf(buf, sizeof buf, "block of %lld bytes: aligned %s, writable %s",
           (long long)n, yesno(aligned(p)), yesno(writable(p, n)));
  return uw_strdup(ctx, buf);
}

uw_Basis_string uw_Heapffi_odd(uw_context ctx, uw_Basis_int n) {
  unsigned char *prev = NULL;
  size_t prev_size = 0, size = 1;
  int all_aligned = 1, none_overlap = 1, all_writable = 1;
  long long i;
  char buf[128];

  for (i = 0; i < n; ++i) {
    unsigned char *p = uw_malloc(ctx, size);
    if (!aligned(p))
      all_aligned = 0;
    if (prev && p < prev + prev_size)
      none_overlap = 0;
    if (!writable(p, size))
      all_writable = 0;
    prev = p;
    prev_size = size;
    size = size * 2 + 1;  // 1, 3, 7, 15, ...: never a multiple of the word
    if (size > 5000)
      size = 1;
  }
  snprintf(buf, sizeof buf, "%lld odd-sized blocks: all aligned %s, none overlapping %s, all writable %s",
           (long long)n, yesno(all_aligned), yesno(none_overlap), yesno(all_writable));
  return uw_strdup(ctx, buf);
}

uw_Basis_string uw_Heapffi_pageBoundary(uw_context ctx) {
  size_t ps = sysconf(_SC_PAGESIZE);
  unsigned char *p0 = uw_malloc(ctx, 0), *p1, *p2;
  size_t gap = ps - (uintptr_t)p0 % ps;
  char buf[160];

  p1 = uw_malloc(ctx, gap);
  p2 = uw_malloc(ctx, 1);
  snprintf(buf, sizeof buf, "up to the boundary: starts where measured %s, ends on it %s, writable %s; then one byte: page-aligned %s, writable %s",
           yesno(p1 == p0), yesno((uintptr_t)(p1 + gap) % ps == 0), yesno(writable(p1, gap)),
           yesno((uintptr_t)p2 % ps == 0), yesno(writable(p2, 1)));
  return uw_strdup(ctx, buf);
}

uw_Basis_string uw_Heapffi_zero(uw_context ctx) {
  unsigned char *p0 = uw_malloc(ctx, 0), *p1 = uw_malloc(ctx, 0), *p2 = uw_malloc(ctx, 1);
  char buf[128];
  snprintf(buf, sizeof buf, "size 0: non-NULL %s, aligned %s, twice the same address %s, the next byte allocated there %s",
           yesno(p0 != NULL), yesno(aligned(p0)), yesno(p0 == p1), yesno(p2 == p0 && writable(p2, 1)));
  return uw_strdup(ctx, buf);
}

uw_unit uw_Heapffi_stats(uw_context ctx) {
  uw_memstats(ctx);
  return uw_unit_v;
}
