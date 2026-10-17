PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Conditionals_t (
    uw_N integer NOT NULL,
    uw_S text NOT NULL
);
INSERT INTO uw_Conditionals_t VALUES(1,'cond');
INSERT INTO uw_Conditionals_t VALUES(7,'caseOpt');
INSERT INTO uw_Conditionals_t VALUES(3,'chosen');
INSERT INTO uw_Conditionals_t VALUES(3,'local');
INSERT INTO uw_Conditionals_t VALUES(2,'local');
INSERT INTO uw_Conditionals_t VALUES(1,'local');
INSERT INTO uw_Conditionals_t VALUES(9,'two');
INSERT INTO uw_Conditionals_t VALUES(2,'two');
COMMIT;
