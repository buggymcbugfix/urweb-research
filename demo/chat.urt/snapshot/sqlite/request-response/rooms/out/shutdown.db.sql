PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Chat_Room_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Chat_Room_t (
    uw_Id integer NOT NULL,
    uw_Client integer NOT NULL,
    uw_Channel integer NOT NULL,
    CONSTRAINT uw_Chat_Room_t_pkey PRIMARY KEY (uw_Id, uw_Client)
);
INSERT INTO uw_Chat_Room_t VALUES(1,0,0);
INSERT INTO uw_Chat_Room_t VALUES(1,1,4294967296);
CREATE TABLE uw_Chat_s (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Chat_t (
    uw_Id integer NOT NULL,
    uw_Title text NOT NULL,
    uw_Room integer NOT NULL,
    CONSTRAINT uw_Chat_t_pkey PRIMARY KEY (uw_Id)
);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Chat_s',1);
INSERT INTO sqlite_sequence VALUES('uw_Chat_Room_s',1);
PRAGMA writable_schema=OFF;
COMMIT;
