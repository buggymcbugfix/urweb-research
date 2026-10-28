ACLOCAL_AMFLAGS = -I m4

BIN       = @BIN@
SRCLIB    = @SRCLIB@
INCLUDE   = @INCLUDE@
SITELISP  = @SITELISP@
VERSION   = @VERSION@
MLTONARGS = @MLTONARGS@

LIB_UR   = $(SRCLIB)/ur
LIB_C    = $(SRCLIB)/c
LIB_JS   = $(SRCLIB)/js

all-local: smlnj mlton

SUBDIRS = src/c

.PHONY: smlnj mlton mlton-tc test check-local installcheck-local urt check-snapshot update-snapshot check-urt

smlnj: src/urweb.cm src/version.sml xml/entities.sml
mlton: bin/urweb
mlton-tc: bin/urwebtc

clean-local:
	rm -f bin/urweb bin/urwebtc compiler.du src/urweb.mlton.* \
		src/urweb.cm src/urweb.mlb src/version.sml xml/parse xml/entities.sml
	rm -f demo/demo.exe
	rm -rf .cm src/.cm
	if test -d urt; then $(MAKE) -C urt clean; fi

src/urweb.cm: src/prefix.cm src/sources
	cat $^ | sed -e 's/$$(SRC)\///g' > $@

src/urweb.mlb: src/prefix.mlb src/sources src/suffix.mlb
	sed -e 's/^\(.*\).grm$$/$$(BUILD)\/\1.mlton.grm.sig:\1.mlton.grm.sml/' -e 'y/:/\n/' \
		-e 's/^\(.*\).lex$$/$$(BUILD)\/\1.mlton.lex.sml/' \
	$^ > $@

src/urweb.mlton.lex: src/urweb.lex
	cp $< $@
src/urweb.mlton.grm: src/urweb.grm
	cp $< $@

src/urweb.mlton.lex.sml: src/urweb.mlton.lex
	mllex $<

src/urweb.mlton.grm.sig src/urweb.mlton.grm.sml: src/urweb.mlton.grm
	mlyacc $<

# The commit `urweb -version` names after the release, from URWEB_COMMIT
# in the environment, which a release build sets, or "?" without it.  Not
# read from git: a new commit would then rebuild the compiler although
# its sources are the same.  Rewritten only when it changes, so nothing is
# relinked for nothing.
src/version.sml: FORCE
	@commit=$${URWEB_COMMIT:-?}; \
	echo "structure Version = struct val commit = \"$$commit\" end" > $@.tmp; \
	if cmp -s $@.tmp $@; then rm -f $@.tmp; else mv -f $@.tmp $@; fi
FORCE:

MLTON = mlton

#ifdef DEBUG
#	MLTON += -const 'Exn.keepHistory true'
#endif

#ifdef PROFILE
#	MLTON += -profile $(PROFILE)
#endif

bin/urweb: src/compiler.mlb xml/entities.sml \
		src/urweb.mlb $(srcdir)/src/*.sig $(srcdir)/src/*.sml src/config.sml src/version.sml \
		src/urweb.mlton.lex.sml \
		src/urweb.mlton.grm.sig src/urweb.mlton.grm.sml
	mkdir -p bin
	$(MLTON) $(MLTONARGS) -mlb-path-var 'SRC $(abs_srcdir)/src' -mlb-path-var 'BUILD $(abs_builddir)/src' -output $@ $<
bin/urwebtc: src/compiler.mlb xml/entities.sml \
		src/urweb.mlb $(srcdir)/src/*.sig $(srcdir)/src/*.sml src/config.sml src/version.sml \
		src/urweb.mlton.lex.sml \
		src/urweb.mlton.grm.sig src/urweb.mlton.grm.sml
	mkdir -p bin
	$(MLTON) $(MLTONARGS) -prefer-abs-paths true -show-def-use compiler.du -stop tc -mlb-path-var 'SRC $(abs_srcdir)/src' -mlb-path-var 'BUILD $(abs_builddir)/src' -output $@ $<

xml/entities.sml: xml/parse xml/xhtml-lat1.ent xml/xhtml-special.ent xml/xhtml-symbol.ent
	$^ > $@

xml/parse: xml/parse.sml
	mkdir -p xml
	$(MLTON) $(MLTONARGS) -output $@ $<

install-exec-emacs:
if USE_EMACS
	mkdir -p $(DESTDIR)$(SITELISP)
	cp $(srcdir)/src/elisp/*.el $(DESTDIR)$(SITELISP)/
endif

install-exec-local-main:
	mkdir -p $(DESTDIR)$(BIN)
	install bin/urweb $(DESTDIR)$(BIN)/
	mkdir -p $(DESTDIR)$(LIB_UR)
	cp $(srcdir)/lib/ur/*.urs $(DESTDIR)$(LIB_UR)/
	cp $(srcdir)/lib/ur/*.ur $(DESTDIR)$(LIB_UR)/
	mkdir -p $(DESTDIR)$(LIB_JS)
	cp $(srcdir)/lib/js/*.js $(DESTDIR)$(LIB_JS)/
	mkdir -p $(DESTDIR)$(INCLUDE)
	cp $(srcdir)/include/urweb/*.h $(DESTDIR)$(INCLUDE)/

install-exec-local: install-exec-local-main install-exec-emacs
	-ldconfig

uninstall-emacs:
if USE_EMACS
	rm -f $(DESTDIR)$(SITELISP)/urweb-compat.el $(DESTDIR)$(SITELISP)/urweb-mode.el $(DESTDIR)$(SITELISP)/urweb-move.el \
		$(DESTDIR)$(SITELISP)/urweb-defs.el $(DESTDIR)$(SITELISP)/urweb-mode-startup.el $(DESTDIR)$(SITELISP)/urweb-util.el
endif

uninstall-local-main:
	rm -f $(DESTDIR)$(BIN)/urweb $(DESTDIR)$(LIB_UR)/basis.urs $(DESTDIR)$(LIB_UR)/char.urs $(DESTDIR)$(LIB_UR)/datetime.urs		\
		$(DESTDIR)$(LIB_UR)/listPair.urs $(DESTDIR)$(LIB_UR)/list.urs $(DESTDIR)$(LIB_UR)/monad.urs					\
		$(DESTDIR)$(LIB_UR)/option.urs $(DESTDIR)$(LIB_UR)/string.urs $(DESTDIR)$(LIB_UR)/top.urs $(DESTDIR)$(LIB_UR)/char.ur		\
		$(DESTDIR)$(LIB_UR)/datetime.ur $(DESTDIR)$(LIB_UR)/listPair.ur $(DESTDIR)$(LIB_UR)/list.ur $(DESTDIR)$(LIB_UR)/monad.ur	\
		$(DESTDIR)$(LIB_UR)/option.ur $(DESTDIR)$(LIB_UR)/string.ur $(DESTDIR)$(LIB_UR)/top.ur $(DESTDIR)$(LIB_JS)/urweb.js		\
		$(DESTDIR)$(INCLUDE)/config.h $(DESTDIR)$(INCLUDE)/queue.h $(DESTDIR)$(INCLUDE)/request.h $(DESTDIR)$(INCLUDE)/types.h		\
		$(DESTDIR)$(INCLUDE)/urweb.h $(DESTDIR)$(INCLUDE)/types_cpp.h $(DESTDIR)$(INCLUDE)/urweb_cpp.h \
		$(DESTDIR)$(LIB_UR)/result.ur $(DESTDIR)$(LIB_UR)/result.urs $(DESTDIR)$(LIB_UR)/hex.ur $(DESTDIR)$(LIB_UR)/hex.urs \
		$(DESTDIR)$(LIB_UR)/nonEmpty.ur $(DESTDIR)$(LIB_UR)/nonEmpty.urs

uninstall-local: uninstall-local-main uninstall-emacs

EXTRA_DIST = demo/prose demo/*.urs demo/*.ur demo/*.urp demo/more/prose demo/more/*.urs demo/more/*.ur demo/more/*.urp \
	doc/Makefile doc/*.tex doc/*.ur doc/LICENSE lib/js \
	lib/ur/*.urs lib/ur/*.ur xml/parse.sml xml/*.ent \
	src/coq src/*.sig src/*.sml src/*.mlb src/config.sml.in src/elisp src/*.cm src/sources src/*.grm src/*.lex \
	CHANGELOG LICENSE urweb.ebuild include/urweb/*.h tests/*.html

dist-hook:
	cd src; rm -f urweb.mlton.lex urweb.mlton.grm *.lex.* *.grm.*
	cd demo; rm -f demo.ur*
	cd demo/more; rm -f demo.ur*

# The compiler the checks run, and the flags it gets: by default the in-tree
# build, which needs -boot to find the library in the source tree.
URWEB = bin/urweb
URWEB_FLAGS = -boot -noEmacs

# make installcheck, after make install, runs the checks of make check on
# the installed compiler, $(bindir)/urweb, and builds nothing.  Given on the
# command line of the inner make, URWEB and URWEB_FLAGS also reach the
# runners under tests/, which read them from their environment.  A tree
# with urt adds its checks:
#
#   make installcheck INSTALLCHECKS='check-local check-urt check-snapshot'
INSTALLCHECKS = check-local
installcheck-local:
	$(MAKE) $(INSTALLCHECKS) URWEB='$(bindir)/urweb' URWEB_FLAGS=-noEmacs

TEST_PRELUDE = set -e; \
	pid=""; \
	dbfile=$$(mktemp); \
	cleanup() { \
		kill $$pid 2>/dev/null || true; \
		pid=""; \
		rm -f $$dbfile $$dbfile-shm $$dbfile-wal; \
	}; \
	setup() { \
		sqlite3 $$dbfile < demo/demo.sql; \
	}; \
	trap cleanup EXIT INT TERM; \
	\
	echo "Compiling demo.exe."; \
	$(URWEB) $(URWEB_FLAGS) -dbms sqlite -db $$dbfile -demo /Demo demo

# The Ur/Web toolbox
urt: $(URWEB)
	$(MAKE) -C urt CC="$(CC)"

# Snapshot tests (see urt/urt snapshot --help).  Always from here, since
# a snapshot records the file names the compiler printed and those are
# relative to the directory it ran in.
check-snapshot: urt
	@urt/urt snapshot --whine .

update-snapshot: urt
	@urt/urt snapshot .


check-urt: urt
	$(MAKE) -C urt check CC="$(CC)"

# HTTP requests from io code, against a stub server: see
# tests/io_curl/README.md.  Needs python3, curl and sqlite3, and libcurl to
# link the application.  The script runs through bash, not its first line:
# the nix build sandbox has no /usr/bin/env.
.PHONY: check-io-curl
check-io-curl: $(URWEB)
	bash tests/io_curl/run.sh

# io code under contention, several clients at once: see
# tests/io_concurrent/README.md.  Needs curl and sqlite3, and on Linux cc.
# Through bash, as check-io-curl.
.PHONY: check-io-concurrent
check-io-concurrent: $(URWEB)
	bash tests/io_concurrent/run.sh

# What the id of the toolchain in shell.nix reacts to: see
# tests/shell_id/run.sh.  Needs nix, so it is not part of check, which runs
# in the nix build.  Through bash, as check-io-curl.
.PHONY: check-shell-id
check-shell-id:
	bash tests/shell_id/run.sh

# A kept FastCGI connection and a second one while it idles: see
# tests/fastcgi_idle/README.md.  Needs python3 and spawn-fcgi.  Through
# bash, as check-io-curl.
.PHONY: check-fastcgi-idle
check-fastcgi-idle: $(URWEB)
	bash tests/fastcgi_idle/run.sh

# Kept FastCGI connections beyond the first few, closed ones, requests that
# follow each other on one, and FCGI_WEB_SERVER_ADDRS: see
# tests/fastcgi_kept/README.md.  Needs python3.  Through bash, as above.
.PHONY: check-fastcgi-kept
check-fastcgi-kept: $(URWEB)
	bash tests/fastcgi_kept/run.sh

# Where a program's log lines go with -q and -l, and with stderr connected
# to the journal: see tests/log_levels/README.md. Needs python3. Through
# bash, as above.
.PHONY: check-log-levels
check-log-levels: $(URWEB)
	bash tests/log_levels/run.sh

# How the runtime is linked into a program, and what -dynamic needs: see
# tests/linking/README.md.  Through bash, as above.
.PHONY: check-linking
check-linking: $(URWEB)
	bash tests/linking/run.sh

check-local: $(URWEB) check-io-curl check-io-concurrent check-fastcgi-idle check-fastcgi-kept check-log-levels check-linking
	@$(TEST_PRELUDE); \
	\
	echo "Running IPv4 test."; \
	setup; \
	eval "$$(demo/demo.exe -a 127.0.0.1 -p 8000 -P 9000 -d3 3>&1 1>/dev/null)"; \
	[ "$$status" = 'OK' ] || { echo "demo.exe failed to start" >&2; exit 1; }; \
	curl -s "http://localhost:$$port/Demo/Hello/main" | diff tests/hello.html - || { echo "Test 'Hello' failed" >&2; exit 1; }; \
	curl -s "http://localhost:$$port/Demo/Crud1/create?A=1&B=2&C=3&D=4" | diff tests/crud1.html - || { echo "Test 'Crud1' failed" >&2; exit 1; }; \
	echo "Tests succeeded."

.PHONY: check-ipv6
check-ipv6: $(URWEB)
	@$(TEST_PRELUDE); \
	\
	echo "Running IPv6 test."; \
	setup; \
	eval "$$(demo/demo.exe -A ::1 -p 8000 -P 9000 -d3 3>&1 1>/dev/null)"; \
	[ "$$status" = 'OK' ] || { echo "demo.exe failed to start" >&2; exit 1; }; \
	curl -g -6 -s "http://[::1]:$$port/Demo/Hello/main" | diff tests/hello.html - || { echo "Test 'Hello' failed" >&2; exit 1; }; \
	curl -g -6 -s "http://[::1]:$$port/Demo/Crud1/create?A=1&B=2&C=3&D=4" | diff tests/crud1.html - || { echo "Test 'Crud1' failed" >&2; exit 1; }; \
	echo "Tests succeeded."

.PHONY: resnapshot
resnapshot:
	@test -n "$(REV)" || { echo "usage: make resnapshot REV=<rev>" >&2; exit 1; }
	git rebase $(REV) --exec '$(MAKE) all && urt snapshot && git add -A && git commit --amend --no-edit'