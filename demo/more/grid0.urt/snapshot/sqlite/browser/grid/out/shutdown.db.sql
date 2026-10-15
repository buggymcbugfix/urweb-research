PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Grid0_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Grid0_t (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    CONSTRAINT uw_Grid0_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Grid0_t VALUES(2,42);
INSERT INTO uw_Grid0_t VALUES(3,0);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Grid0_s',3);
PRAGMA writable_schema=OFF;
COMMIT;
