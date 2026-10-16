PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Chan (
    uw_Cli integer NOT NULL,
    uw_Ch integer NOT NULL,
    CONSTRAINT uw_Chan_pkey PRIMARY KEY (uw_Cli)
);
INSERT INTO uw_Chan VALUES(0,0);
COMMIT;
