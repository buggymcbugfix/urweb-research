PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Random_t (
    uw_A integer NOT NULL
);
INSERT INTO uw_Random_t VALUES(1);
INSERT INTO uw_Random_t VALUES(2);
INSERT INTO uw_Random_t VALUES(3);
INSERT INTO uw_Random_t VALUES(4);
INSERT INTO uw_Random_t VALUES(5);
COMMIT;
