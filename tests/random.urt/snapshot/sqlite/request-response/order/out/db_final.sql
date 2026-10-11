PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Random_t (
    uw_A integer NOT NULL
);
INSERT INTO uw_Random_t VALUES(1);
COMMIT;
