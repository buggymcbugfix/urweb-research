PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Grid0_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Grid0_t (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    CONSTRAINT uw_Grid0_t_pkey PRIMARY KEY (uw_Id)
);

