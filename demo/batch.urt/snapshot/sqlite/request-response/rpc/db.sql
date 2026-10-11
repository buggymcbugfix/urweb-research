PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Batch_t (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    CONSTRAINT uw_Batch_t_pkey PRIMARY KEY (uw_Id)
);

