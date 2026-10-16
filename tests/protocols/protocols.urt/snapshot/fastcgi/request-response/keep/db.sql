PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Note (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL,
    CONSTRAINT uw_Note_pkey PRIMARY KEY (uw_Id)
);
