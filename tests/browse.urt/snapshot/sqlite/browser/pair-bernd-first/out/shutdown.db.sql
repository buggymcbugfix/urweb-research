PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Notes (
    uw_Id integer NOT NULL,
    uw_What text NOT NULL,
    CONSTRAINT uw_Notes_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Notes VALUES(1,'from Alice');
CREATE TABLE uw_noteIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_noteIds',1);
PRAGMA writable_schema=OFF;
COMMIT;
