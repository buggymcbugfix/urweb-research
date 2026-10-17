PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Log (
    uw_N integer NOT NULL,
    uw_At text NOT NULL,
    uw_What text NOT NULL
);

CREATE TABLE uw_logIds (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Clients (
    uw_Id integer NOT NULL,
    uw_Ch integer NOT NULL
);

CREATE TABLE uw_clientIds (id INTEGER PRIMARY KEY AUTOINCREMENT);

