PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Ticks (
    uw_N integer NOT NULL,
    uw_At text NOT NULL
);
INSERT INTO uw_Ticks VALUES(1,'2100-01-01 00:00:00');
CREATE TABLE uw_tickIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_tickIds',1);
PRAGMA writable_schema=OFF;
COMMIT;
