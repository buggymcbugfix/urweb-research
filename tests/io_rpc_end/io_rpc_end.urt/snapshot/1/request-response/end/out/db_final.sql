PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_rpc_end_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_rpc_end_t VALUES(100);
INSERT INTO uw_Io_rpc_end_t VALUES(1);
COMMIT;
