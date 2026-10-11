PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Crud3_t (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL,
    CONSTRAINT uw_Crud3_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Crud3_t VALUES(1,'goodbye');
CREATE TABLE uw_Crud3_seq (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Crud3_seq',1);
PRAGMA writable_schema=OFF;
COMMIT;
