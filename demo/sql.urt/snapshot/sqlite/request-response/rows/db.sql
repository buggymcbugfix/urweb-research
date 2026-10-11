PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Sql_t (
    uw_A integer NOT NULL,
    uw_B real NOT NULL,
    uw_C text NOT NULL,
    uw_D integer NOT NULL,
    CONSTRAINT uw_Sql_t_pkey PRIMARY KEY (uw_A)
);

