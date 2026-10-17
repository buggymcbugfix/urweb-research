PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_BatchG_t (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    uw_B real NOT NULL,
    CONSTRAINT uw_BatchG_t_pkey PRIMARY KEY (uw_Id)
);

