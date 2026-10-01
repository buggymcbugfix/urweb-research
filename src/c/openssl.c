#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include <openssl/opensslconf.h>
#include <openssl/opensslv.h>
#include <openssl/sha.h>
#include <openssl/rand.h>
#include <openssl/err.h>

#if !defined(OPENSSL_THREADS)
#error "Ur/Web requires OpenSSL configured with thread support"
#endif

#define PASSSIZE 4

int uw_hash_blocksize = 32;

static int password[PASSSIZE];

char *uw_sig_file = NULL;

// Fills buf with num cryptographically strong random bytes and returns 1.  On
// failure returns 0 and writes the reason to why, a string of up to len bytes.
// RAND_bytes' -1, "not supported by the current RAND method", is a failure
// too: the buffer is not to be used.  The reason is what OpenSSL queued for
// this thread, which is emptied here; it is the only place that says why.
int uw_rand_bytes(unsigned char *buf, int num, char *why, size_t len) {
  int rc = RAND_bytes(buf, num);
  size_t n = 0;
  unsigned long e;

  if (rc == 1)
    return 1;

  why[0] = 0;

  if (rc < 0) {
    ERR_clear_error();
    snprintf(why, len, "not supported by the current RAND method");
    return 0;
  }

  while ((e = ERR_get_error())) {
    char one[256];

    ERR_error_string_n(e, one, sizeof one);
    n += snprintf(why + n, len - n, "%s%s", n ? "; " : "", one);
    if (n >= len)
      n = len - 1;
  }

  if (!n)
    snprintf(why, len, "OpenSSL gave no reason");

  return 0;
}

static void random_password() {
  char why[256];

  if (!uw_rand_bytes((unsigned char *)password, sizeof password, why, sizeof why)) {
    fprintf(stderr, "Error generating random password: %s\n", why);
    exit(1);
  }
}

void uw_init_crypto() {
  // Prepare signatures.
  if (uw_sig_file) {
    int fd;

    if (access(uw_sig_file, F_OK)) {
      random_password();

      if ((fd = open(uw_sig_file, O_WRONLY | O_CREAT, 0700)) < 0) {
        fprintf(stderr, "Can't open signature file %s\n", uw_sig_file);
        perror("open");
        exit(1);
      }

      if (write(fd, &password, sizeof password) != sizeof password) {
        fprintf(stderr, "Error writing signature file\n");
        exit(1);
      }

      close(fd);
    } else {
      if ((fd = open(uw_sig_file, O_RDONLY)) < 0) {
        fprintf(stderr, "Can't open signature file %s\n", uw_sig_file);
        perror("open");
        exit(1);
      }

      if (read(fd, &password, sizeof password) != sizeof password) {
        fprintf(stderr, "Error reading signature file\n");
        exit(1);
      }

      close(fd);
    }
  } else
    random_password();
}

void uw_sign(const char *in, unsigned char *out) {
  SHA256_CTX c;

  SHA256_Init(&c);
  SHA256_Update(&c, password, sizeof password);
  SHA256_Update(&c, in, strlen(in));
  SHA256_Final(out, &c);
}
