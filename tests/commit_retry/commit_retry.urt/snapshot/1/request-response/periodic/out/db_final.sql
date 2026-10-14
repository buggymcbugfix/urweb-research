PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Commit_retry_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Commit_retry_t VALUES(3);
COMMIT;
