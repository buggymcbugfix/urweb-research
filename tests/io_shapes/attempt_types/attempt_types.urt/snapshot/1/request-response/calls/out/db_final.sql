PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Attempt_types_t (
    uw_N integer NOT NULL
);
INSERT INTO uw_Attempt_types_t VALUES(3);
INSERT INTO uw_Attempt_types_t VALUES(3);
COMMIT;
