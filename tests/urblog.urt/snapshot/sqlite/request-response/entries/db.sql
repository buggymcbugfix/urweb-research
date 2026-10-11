PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Urblog_entry (
    uw_Id integer NOT NULL,
    uw_Title text NOT NULL,
    uw_Created text NOT NULL,
    uw_Author text NOT NULL,
    uw_Body text NOT NULL,
    CONSTRAINT uw_Urblog_entry_pkey PRIMARY KEY (uw_Id)
);

INSERT INTO uw_Urblog_entry VALUES (1, 'First', '2000-01-01 00:00:00', 'vil', 'Hello, <world> & co');
