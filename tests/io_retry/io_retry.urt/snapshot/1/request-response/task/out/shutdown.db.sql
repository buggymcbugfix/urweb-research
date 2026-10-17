PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_retry_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_retry_t VALUES(1);
INSERT INTO uw_Io_retry_t VALUES(2);
INSERT INTO uw_Io_retry_t VALUES(4);
INSERT INTO uw_Io_retry_t VALUES(6);
INSERT INTO uw_Io_retry_t VALUES(7);
COMMIT;
