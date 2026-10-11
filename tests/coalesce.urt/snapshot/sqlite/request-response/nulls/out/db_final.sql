PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Coalesce_t (
    uw_A integer
);
INSERT INTO uw_Coalesce_t VALUES(NULL);
INSERT INTO uw_Coalesce_t VALUES(1);
INSERT INTO uw_Coalesce_t VALUES(NULL);
INSERT INTO uw_Coalesce_t VALUES(2);
COMMIT;
