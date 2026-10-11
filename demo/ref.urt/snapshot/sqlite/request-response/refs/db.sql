PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

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

