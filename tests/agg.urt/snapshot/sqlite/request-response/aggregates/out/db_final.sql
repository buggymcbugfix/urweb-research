PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Agg_t1 (
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C real NOT NULL
);
INSERT INTO uw_Agg_t1 VALUES(1,'a',1.0);
INSERT INTO uw_Agg_t1 VALUES(2,'b',2.0);
INSERT INTO uw_Agg_t1 VALUES(50,'c',99.0);
CREATE TABLE uw_Agg_t2 (
    uw_A real NOT NULL,
    uw_D integer NOT NULL,
    uw_E text
);
INSERT INTO uw_Agg_t2 VALUES(1.0,1,NULL);
INSERT INTO uw_Agg_t2 VALUES(1.0,2,'a');
INSERT INTO uw_Agg_t2 VALUES(1.0,3,NULL);
INSERT INTO uw_Agg_t2 VALUES(1.0,3,'b');
INSERT INTO uw_Agg_t2 VALUES(1.0,3,'c');
COMMIT;
