PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Sqlprecision_t (
    uw_N real NOT NULL
);
INSERT INTO uw_Sqlprecision_t VALUES(0.1);
INSERT INTO uw_Sqlprecision_t VALUES(3.1415926535897931);
INSERT INTO uw_Sqlprecision_t VALUES(1.0e+300);
INSERT INTO uw_Sqlprecision_t VALUES(1.0e-07);
INSERT INTO uw_Sqlprecision_t VALUES(0.0);
INSERT INTO uw_Sqlprecision_t VALUES(1.2345678901234568e+17);
INSERT INTO uw_Sqlprecision_t VALUES(9.0e+999);
COMMIT;
