PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Chat_Room_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Chat_Room_t (
    uw_Id integer NOT NULL,
    uw_Client integer NOT NULL,
    uw_Channel integer NOT NULL,
    CONSTRAINT uw_Chat_Room_t_pkey PRIMARY KEY (uw_Id, uw_Client)
);

CREATE TABLE uw_Chat_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Chat_t (
    uw_Id integer NOT NULL,
    uw_Title text NOT NULL,
    uw_Room integer NOT NULL,
    CONSTRAINT uw_Chat_t_pkey PRIMARY KEY (uw_Id)
);

