PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Crud2_t (
    uw_Id integer NOT NULL,
    uw_Name text NOT NULL,
    uw_Ready integer NOT NULL,
    CONSTRAINT uw_Crud2_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Crud2_t VALUES(1,'Ada',1);
INSERT INTO uw_Crud2_t VALUES(2,'Bob',1);
CREATE TABLE uw_Crud2_seq (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Crud2_seq',2);
PRAGMA writable_schema=OFF;
COMMIT;
