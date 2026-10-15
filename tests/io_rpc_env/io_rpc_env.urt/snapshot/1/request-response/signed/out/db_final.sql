PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_rpc_env_t (
    uw_N integer NOT NULL,
    uw_S text NOT NULL
);
INSERT INTO uw_Io_rpc_env_t VALUES(1,'alice');
INSERT INTO uw_Io_rpc_env_t VALUES(3,'alice');
INSERT INTO uw_Io_rpc_env_t VALUES(4,'alice');
COMMIT;
