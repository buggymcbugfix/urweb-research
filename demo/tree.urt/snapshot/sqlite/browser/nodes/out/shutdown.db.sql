PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Tree_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Tree_t (
    uw_Id integer NOT NULL,
    uw_Parent integer,
    uw_Nam text NOT NULL,
    CONSTRAINT uw_Tree_t_pkey PRIMARY KEY (uw_Id),
    CONSTRAINT uw_Tree_t_F FOREIGN KEY (uw_Parent) REFERENCES uw_Tree_t (uw_Id) ON DELETE CASCADE
);
INSERT INTO uw_Tree_t VALUES(1,NULL,'root');
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Tree_s',3);
PRAGMA writable_schema=OFF;
COMMIT;
