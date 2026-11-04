PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_page_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_page_t VALUES(7);
INSERT INTO uw_Io_page_t VALUES(3);
INSERT INTO uw_Io_page_t VALUES(100);
INSERT INTO uw_Io_page_t VALUES(42);
COMMIT;
