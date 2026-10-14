PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Notes (
    uw_Id integer NOT NULL,
    uw_What text NOT NULL,
    CONSTRAINT uw_Notes_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_noteIds (id INTEGER PRIMARY KEY AUTOINCREMENT);

