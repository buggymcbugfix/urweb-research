PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Ascdesc_t (
    uw_A integer NOT NULL
);
INSERT INTO uw_Ascdesc_t VALUES(1);
INSERT INTO uw_Ascdesc_t VALUES(2);
INSERT INTO uw_Ascdesc_t VALUES(3);
COMMIT;
