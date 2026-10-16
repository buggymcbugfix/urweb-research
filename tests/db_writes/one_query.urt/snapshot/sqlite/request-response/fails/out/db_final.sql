PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_One_query_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_One_query_t VALUES(1);
COMMIT;
