PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Join_t (
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C text
);
INSERT INTO uw_Join_t VALUES(1,'one',NULL);
INSERT INTO uw_Join_t VALUES(2,'two','deux');
COMMIT;
