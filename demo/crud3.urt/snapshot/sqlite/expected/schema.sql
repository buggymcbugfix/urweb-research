PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Crud3_t (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL,
    CONSTRAINT uw_Crud3_t_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Crud3_seq (id INTEGER PRIMARY KEY AUTOINCREMENT);

