-- 2100-01-01 00:01:08.900 UTC POST /CookieSec/imHere
-- demo/cookieSec.ur:36:8-38:15 (1 row)
DELETE FROM uw_CookieSec_lastVisit WHERE (uw_User = 'alice');
-- demo/cookieSec.ur:37:8-38:15 (1 row)
INSERT INTO uw_CookieSec_lastVisit (uw_User, uw_When) VALUES ('alice', CURRENT_TIMESTAMP);
