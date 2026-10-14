The first COMMIT of a request (the request run) or of a periodic task (the
periodic run) fails the way a serializable transaction does on postgres: the
database has rolled the transaction back and the driver returns -1.  The
injection is a swapped db_commit (conflict.c), so that SQLite can show what
the runtime does.
