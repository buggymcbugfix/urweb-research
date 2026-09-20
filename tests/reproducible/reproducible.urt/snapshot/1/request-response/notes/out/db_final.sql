PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Note (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL,
    uw_Created text NOT NULL,
    CONSTRAINT uw_Note_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Note VALUES(1,'first','2000-01-01 00:00:01');
CREATE TABLE uw_noteIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_noteIds',1);
PRAGMA writable_schema=OFF;
COMMIT;
