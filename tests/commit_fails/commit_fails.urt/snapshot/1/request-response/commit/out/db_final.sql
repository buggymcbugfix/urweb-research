PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Commit_fails_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Commit_fails_t VALUES(2);
COMMIT;
