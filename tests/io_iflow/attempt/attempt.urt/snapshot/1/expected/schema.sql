PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Attempt_fruit (
    uw_Id integer NOT NULL,
    uw_Nam text NOT NULL,
    uw_Secret text NOT NULL,
    CONSTRAINT uw_Attempt_fruit_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Attempt_pub (
    uw_Nam text NOT NULL
);

