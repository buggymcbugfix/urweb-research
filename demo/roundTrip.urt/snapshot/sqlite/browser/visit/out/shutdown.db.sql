PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_RoundTrip_channels (
    uw_Client integer NOT NULL,
    uw_Channel integer NOT NULL,
    CONSTRAINT uw_RoundTrip_channels_pkey PRIMARY KEY (uw_Client)
);
INSERT INTO uw_RoundTrip_channels VALUES(0,0);
INSERT INTO uw_RoundTrip_channels VALUES(1,4294967296);
COMMIT;
