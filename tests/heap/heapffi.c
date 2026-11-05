#include <errno.h>
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

// uw_memstats, with the heap's available size rounded up to 64 KiB.  The
// runtime keeps the heap in whole pages, so the exact size depends on the
// machine's page size (4 KiB on most, 16 KiB on Apple Silicon); 64 KiB is a
// multiple of every page size in use, and the sizes of the decay runs
// still go down from one request to the next.  The context's buffers are
// not public, so uw_memstats prints to a temporary file put in place of
// stdout, and its lines are copied from there.  The runs that print
// statistics send one request at a time, so no other thread prints
// meanwhile.
uw_unit uw_Heapffi_stats(uw_context ctx) {
  FILE *f = tmpfile();
  int saved;
  char line[256];
  unsigned long used, avail;

  if (!f)
    uw_error(ctx, FATAL, "tmpfile: %s", strerror(errno));
  fflush(stdout);
  saved = dup(1);
  dup2(fileno(f), 1);
  uw_memstats(ctx);
  fflush(stdout);
  dup2(saved, 1);
  close(saved);

  rewind(f);
  while (fgets(line, sizeof line, f))
    if (sscanf(line, "Heap: %lu/%lu", &used, &avail) == 2)
      printf("Heap: %lu/%lu\n", used, (avail + 65535) / 65536 * 65536);
    else
      fputs(line, stdout);
  fclose(f);
  fflush(stdout);
  return uw_unit_v;
}
