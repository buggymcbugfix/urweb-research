PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_EmptyUpdate_a (
    uw_B integer NOT NULL
);

INSERT INTO uw_EmptyUpdate_a VALUES (1), (2);
