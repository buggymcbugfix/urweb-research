PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_sqlcache_rollback_counter (
    uw_Id integer NOT NULL,
    uw_V integer NOT NULL
);
INSERT INTO uw_Io_sqlcache_rollback_counter VALUES(1,0);
COMMIT;
