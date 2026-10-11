PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Crud1_t1 (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C real NOT NULL,
    uw_D integer NOT NULL,
    CONSTRAINT uw_Crud1_t1_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Crud1_seq (id INTEGER PRIMARY KEY AUTOINCREMENT);

