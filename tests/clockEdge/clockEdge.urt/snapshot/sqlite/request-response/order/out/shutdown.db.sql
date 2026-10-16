PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Log (
    uw_N integer NOT NULL,
    uw_At text NOT NULL,
    uw_What text NOT NULL
);
INSERT INTO uw_Log VALUES(1,'2100-01-01 00:00:00','ten');
INSERT INTO uw_Log VALUES(2,'2100-01-01 00:00:00','twenty-five');
INSERT INTO uw_Log VALUES(3,'2100-01-01 00:00:00','once');
INSERT INTO uw_Log VALUES(4,'2100-01-01 00:00:10','ten');
INSERT INTO uw_Log VALUES(5,'2100-01-01 00:00:20','ten');
INSERT INTO uw_Log VALUES(6,'2100-01-01 00:00:25','twenty-five');
INSERT INTO uw_Log VALUES(7,'2100-01-01 00:00:30','ten');
INSERT INTO uw_Log VALUES(8,'2100-01-01 00:00:40','ten');
INSERT INTO uw_Log VALUES(9,'2100-01-01 00:00:50','ten');
INSERT INTO uw_Log VALUES(10,'2100-01-01 00:00:50','twenty-five');
INSERT INTO uw_Log VALUES(11,'2100-01-01 00:01:00','ten');
CREATE TABLE uw_logIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Clients (
    uw_Id integer NOT NULL,
    uw_Ch integer NOT NULL
);
CREATE TABLE uw_clientIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_logIds',11);
PRAGMA writable_schema=OFF;
COMMIT;
