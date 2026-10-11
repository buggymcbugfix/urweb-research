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

