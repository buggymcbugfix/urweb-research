PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_rpc_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_rpc_t VALUES(21);
CREATE TABLE uw_Io_rpc_chans (
    uw_Ch integer NOT NULL
);
INSERT INTO uw_Io_rpc_chans VALUES(0);
COMMIT;
