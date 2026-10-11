PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Sql_ops_t (
    uw_A integer NOT NULL,
    uw_B real NOT NULL
);

INSERT INTO uw_Sql_ops_t VALUES (1, 1.5), (-2, 0.1), (3000000000, 1e200);
