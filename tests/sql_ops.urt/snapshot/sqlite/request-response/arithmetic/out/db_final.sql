PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Sql_ops_t (
    uw_A integer NOT NULL,
    uw_B real NOT NULL
);
INSERT INTO uw_Sql_ops_t VALUES(1,1.5);
INSERT INTO uw_Sql_ops_t VALUES(-2,0.1);
INSERT INTO uw_Sql_ops_t VALUES(3000000000,1.0e+200);
COMMIT;
