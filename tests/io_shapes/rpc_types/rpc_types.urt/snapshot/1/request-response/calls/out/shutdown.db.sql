PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Rpc_types_t (
    uw_N integer NOT NULL,
    uw_S text NOT NULL
);
INSERT INTO uw_Rpc_types_t VALUES(1,'viaAlias');
INSERT INTO uw_Rpc_types_t VALUES(1,'wrapped');
INSERT INTO uw_Rpc_types_t VALUES(123,'partial');
INSERT INTO uw_Rpc_types_t VALUES(1,'a b');
INSERT INTO uw_Rpc_types_t VALUES(1,'b');
INSERT INTO uw_Rpc_types_t VALUES(1,'min');
INSERT INTO uw_Rpc_types_t VALUES(2,'max');
INSERT INTO uw_Rpc_types_t VALUES(0,'');
INSERT INTO uw_Rpc_types_t VALUES(0,'a b c');
INSERT INTO uw_Rpc_types_t VALUES(0,'café');
INSERT INTO uw_Rpc_types_t VALUES(0,'a/b?&=+%');
INSERT INTO uw_Rpc_types_t VALUES(1,'unit');
INSERT INTO uw_Rpc_types_t VALUES(0,'1.5');
INSERT INTO uw_Rpc_types_t VALUES(0,'-2.25');
INSERT INTO uw_Rpc_types_t VALUES(21,'opt');
INSERT INTO uw_Rpc_types_t VALUES(-1,'opt');
INSERT INTO uw_Rpc_types_t VALUES(1,'list');
INSERT INTO uw_Rpc_types_t VALUES(2,'list');
INSERT INTO uw_Rpc_types_t VALUES(3,'list');
INSERT INTO uw_Rpc_types_t VALUES(0,'color');
INSERT INTO uw_Rpc_types_t VALUES(0,'color');
INSERT INTO uw_Rpc_types_t VALUES(0,'color');
INSERT INTO uw_Rpc_types_t VALUES(9,'xb');
INSERT INTO uw_Rpc_types_t VALUES(1,'tryNone');
INSERT INTO uw_Rpc_types_t VALUES(9223372036854775805,'wrapped');
INSERT INTO uw_Rpc_types_t VALUES(-9223372036854775808,'wrapped');
COMMIT;
