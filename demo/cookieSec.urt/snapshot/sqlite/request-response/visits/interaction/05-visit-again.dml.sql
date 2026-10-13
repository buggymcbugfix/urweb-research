-- 2000-01-01 00:00:04 UTC POST /CookieSec/imHere
-- demo/cookieSec.ur:36:8-38:15 (1 row)
DELETE FROM uw_CookieSec_lastVisit WHERE (uw_User = 'vil');
-- demo/cookieSec.ur:37:8-38:15 (1 row)
INSERT INTO uw_CookieSec_lastVisit (uw_User, uw_When) VALUES ('vil', CURRENT_TIMESTAMP);
