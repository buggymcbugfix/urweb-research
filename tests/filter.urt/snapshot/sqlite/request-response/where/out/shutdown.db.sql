PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Filter_t (
    uw_A integer NOT NULL,
    uw_B real NOT NULL
);
INSERT INTO uw_Filter_t VALUES(1,2.0);
INSERT INTO uw_Filter_t VALUES(2,1.0);
INSERT INTO uw_Filter_t VALUES(3,3.0);
INSERT INTO uw_Filter_t VALUES(4,4.0);
INSERT INTO uw_Filter_t VALUES(44,4.4);
COMMIT;
