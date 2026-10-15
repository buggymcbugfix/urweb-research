PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Crud2_t (
    uw_Id integer NOT NULL,
    uw_Name text NOT NULL,
    uw_Ready integer NOT NULL,
    CONSTRAINT uw_Crud2_t_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Crud2_seq (id INTEGER PRIMARY KEY AUTOINCREMENT);

