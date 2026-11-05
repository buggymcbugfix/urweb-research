PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_page_commit_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_page_commit_t VALUES(2);
INSERT INTO uw_Io_page_commit_t VALUES(4);
INSERT INTO uw_Io_page_commit_t VALUES(5);
INSERT INTO uw_Io_page_commit_t VALUES(6);
INSERT INTO uw_Io_page_commit_t VALUES(2);
INSERT INTO uw_Io_page_commit_t VALUES(7);
INSERT INTO uw_Io_page_commit_t VALUES(11);
COMMIT;
