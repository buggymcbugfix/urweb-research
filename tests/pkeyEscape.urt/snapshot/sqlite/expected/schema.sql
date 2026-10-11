PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_PkeyEscape_t (
    uw_A integer NOT NULL,
    uw_B integer NOT NULL,
    CONSTRAINT uw_PkeyEscape_t_pkey PRIMARY KEY (uw_A, uw_B)
);

