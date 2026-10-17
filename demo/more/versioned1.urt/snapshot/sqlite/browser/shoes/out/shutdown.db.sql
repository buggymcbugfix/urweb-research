PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Versioned1_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Versioned1_t (
    uw_When text NOT NULL,
    uw_Version integer NOT NULL,
    uw_Id integer NOT NULL,
    uw_Nam text,
    uw_ShoeSize integer
);
INSERT INTO uw_Versioned1_t VALUES('2100-01-01 00:00:02',1,1,'Carlos',44);
INSERT INTO uw_Versioned1_t VALUES('2100-01-01 00:00:05',2,2,'Eun-ji',37);
INSERT INTO uw_Versioned1_t VALUES('2100-01-01 00:01:06',3,1,NULL,45);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Versioned1_s',3);
PRAGMA writable_schema=OFF;
COMMIT;
