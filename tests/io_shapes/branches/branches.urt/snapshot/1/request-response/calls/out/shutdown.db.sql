PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Branches_t (
    uw_N integer NOT NULL,
    uw_S text NOT NULL
);
INSERT INTO uw_Branches_t VALUES(0,'down');
INSERT INTO uw_Branches_t VALUES(0,'pong');
COMMIT;
