PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Orm1_T_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Orm1_T_t (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    uw_B text NOT NULL
);
CREATE TABLE uw_Orm1_S_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Orm1_S_t (
    uw_Id integer NOT NULL,
    uw_C integer NOT NULL,
    uw_D real NOT NULL
);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Orm1_T_s',6);
INSERT INTO sqlite_sequence VALUES('uw_Orm1_S_s',2);
PRAGMA writable_schema=OFF;
COMMIT;
