CC = cc
CFLAGS = -std=c99 -Wall -Wextra -O2

SRC = src/urt.c src/snapshot.c src/runs.c src/format.c src/report.c src/fs.c src/tui.c
HDR = src/commands.h src/report.h src/fs.h src/runs.h src/tui.h

all: urt urt-format

# The engines' wall clock on the run's, loaded into them by urt-browse
# (src/urtclock.c); Linux only.
ifeq ($(shell uname -s),Linux)
all: liburtclock.so
endif

urt: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -o urt $(SRC)

liburtclock.so: src/urtclock.c
	$(CC) $(CFLAGS) -shared -fPIC -o liburtclock.so src/urtclock.c -ldl

# On macOS, untried as yet, so built only when asked for, and then used.
liburtclock.dylib: src/urtclock-mac.c
	$(CC) $(CFLAGS) -dynamiclib -o liburtclock.dylib src/urtclock-mac.c

# The formatter is built in its own directory (src/format/Makefile)
# and copied beside Urt, where `urt format` looks for it.
urt-format: src/format/urt-format
	cp src/format/urt-format urt-format

src/format/urt-format:
	$(MAKE) -C src/format urt-format

check: urt urt-format
	./check

check-snapshot: urt
	./check snapshot

check-pty: urt
	./check pty

check-format: urt urt-format
	./check fmt

check-demo: urt urt-format
	./check fmt-demo

check-oracle: urt urt-format
	./check fmt-oracle

update: urt urt-format
	./check -u

update-snapshot: urt
	./check -u snapshot

update-pty: urt
	./check -u pty

update-format: urt urt-format
	./check -u fmt

update-demo: urt urt-format
	./check -u fmt-demo

clean:
	$(MAKE) -C src/format clean
	rm -f urt urt-format liburtclock.so liburtclock.dylib

.PHONY: all check check-snapshot check-pty check-format check-demo check-oracle \
        update update-snapshot update-pty update-format update-demo clean src/format/urt-format
