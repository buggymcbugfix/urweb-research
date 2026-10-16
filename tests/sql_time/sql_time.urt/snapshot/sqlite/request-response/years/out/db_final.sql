PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Sql_time_t (
    uw_Id integer NOT NULL,
    uw_T text NOT NULL
);
INSERT INTO uw_Sql_time_t VALUES(1,'999-06-01 00:00:00');
INSERT INTO uw_Sql_time_t VALUES(2,'10000-01-01 00:00:00');
INSERT INTO uw_Sql_time_t VALUES(3,'1969-12-31 23:59:59');
COMMIT;
