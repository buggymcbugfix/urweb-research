PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Ref_IR_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Ref_IR_t (
    uw_Id integer NOT NULL,
    uw_Data integer NOT NULL,
    CONSTRAINT uw_Ref_IR_t_pkey PRIMARY KEY (uw_Id)
);
CREATE TABLE uw_Ref_SR_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Ref_SR_t (
    uw_Id integer NOT NULL,
    uw_Data text NOT NULL,
    CONSTRAINT uw_Ref_SR_t_pkey PRIMARY KEY (uw_Id)
);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Ref_IR_s',4);
INSERT INTO sqlite_sequence VALUES('uw_Ref_SR_s',2);
PRAGMA writable_schema=OFF;
COMMIT;
