PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_rpc_cookie_t (
    uw_N integer NOT NULL,
    uw_S text NOT NULL
);
INSERT INTO uw_Io_rpc_cookie_t VALUES(1,'visitor');
COMMIT;
