`sqlcache` with io code: the cache locks the compiler takes at the start of
an io task's body are released when the body ends, so a page reading the
cached table answers between ticks; and the queries inside an io RPC's
transactions go through the cache, so a read repeated after one that stored
its value runs no SQL.  The run waits for a tick and reads the page, twice,
then calls the RPCs (an update with a read, then a read three times) and
reads the page again; the RPCs' connection logs every statement it runs.
