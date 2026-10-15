PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Note (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL,
    CONSTRAINT uw_Note_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Note VALUES(2,'touched');
COMMIT;
