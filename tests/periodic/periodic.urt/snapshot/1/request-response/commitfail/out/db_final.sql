PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Periodic_stuck (
    uw_N integer NOT NULL
);
INSERT INTO uw_Periodic_stuck VALUES(2);
COMMIT;
