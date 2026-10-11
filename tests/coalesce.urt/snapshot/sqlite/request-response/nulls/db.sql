PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Coalesce_t (
    uw_A integer
);

INSERT INTO uw_Coalesce_t VALUES (NULL), (1), (NULL), (2);
