PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_DynChannel_channels (
    uw_Id integer NOT NULL,
    uw_Channel integer NOT NULL
);
INSERT INTO uw_DynChannel_channels VALUES(0,0);
COMMIT;
