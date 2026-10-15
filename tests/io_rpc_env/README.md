An RPC that reads an environment variable and writes the database.  The
compiler counts `getenv` as a read of request data, since under cgi and
fastcgi the environment carries the request, so a transaction RPC doing
that and writing is a ReadCookieWrite export whose request must carry the
cookie signature, like one that reads a cookie.  An io RPC that reads the
variable with `io_getenv` outside its transactions and writes in one of
them is to be classified the same.  The snapshot has the two handlers and
the client's two calls; the run loads the page, which carries the
signature, and calls each function with it and without it, then reads the
rows.
