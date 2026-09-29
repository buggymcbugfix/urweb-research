A copy of urweb-curl (https://github.com/buggymcbugfix/urweb-curl, branch
rewrite, commit e906e59 of 2026-09-29): the library's five source files,
unchanged, so that the io tests can make HTTP requests from io code without
fetching anything.  Its own build and tests are in that repository; here
the test application links curl.c as a source and libcurl from the system.
