PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_rpc_send_chans (
    uw_Ch integer NOT NULL
);
INSERT INTO uw_Io_rpc_send_chans VALUES(0);
CREATE TABLE uw_Io_rpc_send_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_rpc_send_t VALUES(1);
COMMIT;
