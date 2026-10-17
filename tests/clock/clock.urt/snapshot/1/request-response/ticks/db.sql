PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Ticks (
    uw_N integer NOT NULL,
    uw_At text NOT NULL
);

CREATE TABLE uw_tickIds (id INTEGER PRIMARY KEY AUTOINCREMENT);

