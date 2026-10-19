The benchmark's application (`../bench.urp`) as the benchmark builds it,
one case per protocol, and its five pages as each build answers them:
`tiny`, `page` (64 KiB built and escaped), `file` (the 64 KiB of
`../page.txt` served from the binary), `read` of a row that is there and
of one that is not, and `write`, whose insert is `05-write.sql`.  The
`server.c` holds the served file as the compiler embeds it, four bytes
of C for each byte of the file.  The `run` runs are the program as the
benchmark's nokeep rows start it; the `keep` runs are the HTTP server
with `-k` and the FastCGI program on one kept connection (`keep-conn`),
as behind nginx with keep-alive.

The http case tracks `server.c`, the fastcgi case does not: the
generated C is the same for both, since the protocol is the runtime
library the program is linked with and nothing in the generated file,
and what pins the FastCGI side is its runs.
