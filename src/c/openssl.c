#include "config.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <openssl/opensslconf.h>
#include <openssl/opensslv.h>
#include <openssl/sha.h>
#include <openssl/rand.h>

#include "urweb.h"

#if !defined(OPENSSL_THREADS)
#error "Ur/Web requires OpenSSL configured with thread support"
#endif

#define PASSSIZE 4

int uw_hash_blocksize = 32;

static int password[PASSSIZE];

char *uw_sig_file = NULL;

static void random_password() {
  if (!RAND_bytes((unsigned char *)password, sizeof password)) {
    uw_log(UW_LOG_ERR, "Error generating random password\n");
    exit(1);
  }
}

void uw_init_crypto() {
  // The key file named when the application was compiled can be replaced
  // when it is started, as the database can.
  char *env_sig_file = getenv("URWEB_SIGFILE_PATH");
  if (env_sig_file)
    uw_sig_file = env_sig_file;

  // Prepare signatures.
  if (uw_sig_file) {
    int fd;

    if (access(uw_sig_file, F_OK)) {
      random_password();

      if ((fd = open(uw_sig_file, O_WRONLY | O_CREAT, 0700)) < 0) {
        uw_log(UW_LOG_ERR, "Can't open signature file %s: %s\n", uw_sig_file, strerror(errno));
        exit(1);
      }

      if (write(fd, &password, sizeof password) != sizeof password) {
        uw_log(UW_LOG_ERR, "Error writing signature file\n");
        exit(1);
      }

      close(fd);
    } else {
      if ((fd = open(uw_sig_file, O_RDONLY)) < 0) {
        uw_log(UW_LOG_ERR, "Can't open signature file %s: %s\n", uw_sig_file, strerror(errno));
        exit(1);
      }

      if (read(fd, &password, sizeof password) != sizeof password) {
        uw_log(UW_LOG_ERR, "Error reading signature file\n");
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
