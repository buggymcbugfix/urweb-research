`sqlcache` with an io task: the cache locks the compiler takes at the start
of the body are released when the body ends, so a page reading the cached
table answers between ticks.  The run waits for a tick, reads the page,
waits for the next tick and reads it again.
