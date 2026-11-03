PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Onerror_commitE_failures (
    uw_N integer NOT NULL
);
INSERT INTO uw_Onerror_commitE_failures VALUES(1);
CREATE TABLE uw_Onerror_commit_t (
    uw_N integer NOT NULL
);
COMMIT;
