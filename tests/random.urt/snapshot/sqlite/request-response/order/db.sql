PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Random_t (
    uw_A integer NOT NULL
);

INSERT INTO uw_Random_t VALUES (1);
INSERT INTO uw_Random_t VALUES (2), (3), (4), (5);
