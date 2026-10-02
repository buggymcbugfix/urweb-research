The first COMMIT of a request (request/) or of a periodic task (periodic/)
fails the way a serializable transaction does on postgres: the database has
rolled the transaction back and the driver returns -1.  The injection is a
swapped db_commit (conflict.c), so that SQLite can show what the runtime does.
