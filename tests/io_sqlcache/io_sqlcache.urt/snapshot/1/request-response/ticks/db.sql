PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Io_sqlcache_counter (
    uw_Id integer NOT NULL,
    uw_V integer NOT NULL
);


INSERT INTO uw_Io_sqlcache_counter (uw_Id, uw_V) VALUES (1, 0);
