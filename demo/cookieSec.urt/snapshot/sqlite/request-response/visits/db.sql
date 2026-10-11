PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_CookieSec_lastVisit (
    uw_User text NOT NULL,
    uw_When text NOT NULL,
    CONSTRAINT uw_CookieSec_lastVisit_pkey PRIMARY KEY (uw_User)
);

