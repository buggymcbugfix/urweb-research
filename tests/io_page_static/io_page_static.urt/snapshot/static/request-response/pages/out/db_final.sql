PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_page_static_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_page_static_t VALUES(100);
COMMIT;
