PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Versioned1_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Versioned1_t (
    uw_When text NOT NULL,
    uw_Version integer NOT NULL,
    uw_Id integer NOT NULL,
    uw_Nam text,
    uw_ShoeSize integer
);

