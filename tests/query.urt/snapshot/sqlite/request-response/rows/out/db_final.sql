PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Query_t1 (
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C real NOT NULL,
    uw_D integer NOT NULL
);
INSERT INTO uw_Query_t1 VALUES(1,'first',1.5,1);
INSERT INTO uw_Query_t1 VALUES(2,'second',2.5,0);
CREATE TABLE uw_Query_t2 (
    uw_A real NOT NULL,
    uw_D integer NOT NULL
);
COMMIT;
