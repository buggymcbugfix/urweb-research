PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_CookieSec_lastVisit (
    uw_User text NOT NULL,
    uw_When text NOT NULL,
    CONSTRAINT uw_CookieSec_lastVisit_pkey PRIMARY KEY (uw_User)
);
INSERT INTO uw_CookieSec_lastVisit VALUES('eunji','2100-01-01 00:00:08');
INSERT INTO uw_CookieSec_lastVisit VALUES('alice','2100-01-01 00:01:08');
COMMIT;
