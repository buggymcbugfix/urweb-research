PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_channel_chans (
    uw_Ch integer NOT NULL
);
INSERT INTO uw_Io_channel_chans VALUES(0);
CREATE TABLE uw_Io_channel_sent (
    uw_N integer NOT NULL
);
INSERT INTO uw_Io_channel_sent VALUES(1);
COMMIT;
