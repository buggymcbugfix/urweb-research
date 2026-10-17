PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_heap_limit_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_heap_limit_t VALUES(28);
INSERT INTO uw_Io_heap_limit_t VALUES(20);
COMMIT;
