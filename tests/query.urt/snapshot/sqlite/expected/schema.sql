PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Query_t1 (
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C real NOT NULL,
    uw_D integer NOT NULL
);

CREATE TABLE uw_Query_t2 (
    uw_A real NOT NULL,
    uw_D integer NOT NULL
);

