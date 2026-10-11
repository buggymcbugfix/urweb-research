PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Pkey_t (
    uw_A integer NOT NULL,
    uw_B integer NOT NULL,
    CONSTRAINT uw_Pkey_t_pkey PRIMARY KEY (uw_B, uw_A)
);

