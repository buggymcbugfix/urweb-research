PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Grid1_t1 (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    CONSTRAINT uw_Grid1_t1_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Grid1_t1 VALUES(0,'none');
INSERT INTO uw_Grid1_t1 VALUES(1,'kettle');
INSERT INTO uw_Grid1_t1 VALUES(2,'cups');
CREATE TABLE uw_Grid1_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Grid1_t (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C integer NOT NULL,
    uw_D integer NOT NULL,
    uw_E integer,
    uw_F integer,
    CONSTRAINT uw_Grid1_t_pkey PRIMARY KEY (uw_Id),
    CONSTRAINT uw_Grid1_t_Foreign FOREIGN KEY (uw_D) REFERENCES uw_Grid1_t1 (uw_Id) ON DELETE CASCADE
);
INSERT INTO uw_Grid1_t VALUES(2,0,'',0,0,NULL,NULL);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Grid1_s',2);
PRAGMA writable_schema=OFF;
COMMIT;
