PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_outside_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_outside_t VALUES(1);
INSERT INTO uw_Io_outside_t VALUES(2);
COMMIT;
