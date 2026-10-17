PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_RoundTrip_channels (
    uw_Client integer NOT NULL,
    uw_Channel integer NOT NULL,
    CONSTRAINT uw_RoundTrip_channels_pkey PRIMARY KEY (uw_Client)
);

