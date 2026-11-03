#!/bin/sh
#
# The build and all checks on FreeBSD, which has no Nix: its packages and
# its system clang. Writes the logs and exit statuses to ci-out, as the
# Nix jobs do. Run as root, from the root of the checkout, in the VM.

mkdir -p ci-out
here=$PWD

pkg install -y mlton autoconf automake libtool pkgconf gmake bash \
  icu sqlite3 curl python3 git postgresql17-client > ci-out/pkg.log 2>&1
echo $? > ci-out/pkg.status

# The fix derivation.nix applies to MLton's copy of the SML/NJ Library, if
# this MLton's copy has the bug.
f=$(find /usr/local/lib/mlton -name int-binary-set.sml | head -1)
if grep -q 'if (x < v) then split_gt(r,x)' "$f"; then
  sed -i '' \
    -e 's/if (x < v) then split_gt(r,x)/if (x > v) then split_gt(r,x)/' \
    -e 's/else if (x > v) then concat3(split_gt(l,x),v,r)/else if (x < v) then concat3(split_gt(l,x),v,r)/' \
    "$f"
  ibs="patched $f"
else
  ibs="no fix needed in $f"
fi

# spawn-fcgi with the patch the Nix build applies: the package binds its
# Unix socket under a name one byte short.
{
  fetch -q -o /tmp/sf.tar.xz \
    https://download.lighttpd.net/spawn-fcgi/releases-1.6.x/spawn-fcgi-1.6.6.tar.xz &&
  tar -xf /tmp/sf.tar.xz -C /tmp &&
  cd /tmp/spawn-fcgi-1.6.6 &&
  patch -s -p1 < "$here/nix/patches/spawn-fcgi/sun-len.patch" &&
  printf '#define PACKAGE_NAME "spawn-fcgi"\n#define PACKAGE_VERSION "1.6.6"\n' > src/config.h &&
  cc -DHAVE_SOCKLEN_T -DHAVE_ISSETUGID -o /usr/local/bin/spawn-fcgi src/spawn-fcgi.c
} > ci-out/spawn-fcgi.log 2>&1
cd "$here"

{
  uname -a; freebsd-version; echo "IntBinarySet: $ibs"; cc --version; mlton
  python3 --version; sqlite3 --version; command -v spawn-fcgi
} > ci-out/env.txt 2>&1

export CC=cc
# As for clang on Linux: the flags of the build assume gcc.
export CFLAGS="-g -O2 -Wno-error"
export SQHEADER=/usr/local/include/sqlite3.h
export PGHEADER=/usr/local/include/libpq-fe.h
export ICU_INCLUDES=-I/usr/local/include
export ICU_LIBS=-L/usr/local/lib
export CCARGS="-I$here/out/include -I/usr/local/include -L/usr/local/lib -Wno-error=int-conversion -Qunused-arguments"
{
  ./autogen.sh && ./configure --prefix="$here/out" &&
  gmake -C src/c -j4 && gmake -C src/c install && gmake mlton
} > ci-out/build.log 2>&1
echo $? > ci-out/build.status

# urt asks for -std=c99, under which FreeBSD's headers hide MSG_DONTWAIT;
# built here with the BSD namespace, so that make urt has nothing left to do.
gmake -C urt CC="cc -D__BSD_VISIBLE=1" > ci-out/urt.log 2>&1
echo $? > ci-out/urt.status

if [ "$(cat ci-out/build.status)" = 0 ]; then
  timeout 5400 gmake -k check-local check-urt check-snapshot > ci-out/check.log 2>&1
  echo $? > ci-out/check.status
fi
exit 0
