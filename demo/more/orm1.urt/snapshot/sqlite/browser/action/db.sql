PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Orm1_T_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Orm1_T_t (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    uw_B text NOT NULL
);

CREATE TABLE uw_Orm1_S_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Orm1_S_t (
    uw_Id integer NOT NULL,
    uw_C integer NOT NULL,
    uw_D real NOT NULL
);

