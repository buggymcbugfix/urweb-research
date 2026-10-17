PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Log (
    uw_N integer NOT NULL,
    uw_At text NOT NULL,
    uw_What text NOT NULL
);
INSERT INTO uw_Log VALUES(1,'2000-01-01 00:00:00','ten');
INSERT INTO uw_Log VALUES(2,'2000-01-01 00:00:00','twenty-five');
INSERT INTO uw_Log VALUES(3,'2000-01-01 00:00:00','once');
INSERT INTO uw_Log VALUES(4,'2000-01-01 00:00:10','ten');
INSERT INTO uw_Log VALUES(5,'2000-01-01 00:00:20','ten');
INSERT INTO uw_Log VALUES(6,'2000-01-01 00:00:25','twenty-five');
INSERT INTO uw_Log VALUES(7,'2000-01-01 00:00:30','ten');
CREATE TABLE uw_logIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Clients (
    uw_Id integer NOT NULL,
    uw_Ch integer NOT NULL
);
CREATE TABLE uw_clientIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_logIds',7);
INSERT INTO sqlite_sequence VALUES('uw_clientIds',1);
PRAGMA writable_schema=OFF;
COMMIT;
