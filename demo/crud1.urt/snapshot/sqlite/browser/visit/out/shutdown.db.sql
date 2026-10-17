PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Crud1_t1 (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C real NOT NULL,
    uw_D integer NOT NULL,
    CONSTRAINT uw_Crud1_t1_pkey PRIMARY KEY (uw_Id)
);
CREATE TABLE uw_Crud1_seq (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Crud1_seq',1);
PRAGMA writable_schema=OFF;
COMMIT;
