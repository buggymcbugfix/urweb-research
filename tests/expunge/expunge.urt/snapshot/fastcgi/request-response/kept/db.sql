PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Chan (
    uw_Cli integer NOT NULL,
    uw_Ch integer NOT NULL,
    CONSTRAINT uw_Chan_pkey PRIMARY KEY (uw_Cli)
);
