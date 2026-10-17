PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_rpc_attempts_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_rpc_attempts_t VALUES(3);
INSERT INTO uw_Io_rpc_attempts_t VALUES(4);
INSERT INTO uw_Io_rpc_attempts_t VALUES(5);
INSERT INTO uw_Io_rpc_attempts_t VALUES(7);
INSERT INTO uw_Io_rpc_attempts_t VALUES(9);
COMMIT;
