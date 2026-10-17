-- 2100-01-01 00:00:04.250 UTC POST /CookieSec/imHere
-- demo/cookieSec.ur:36:8-38:15 (0 rows)
DELETE FROM uw_CookieSec_lastVisit WHERE (uw_User = 'alice');
-- demo/cookieSec.ur:37:8-38:15 (1 row)
INSERT INTO uw_CookieSec_lastVisit (uw_User, uw_When) VALUES ('alice', CURRENT_TIMESTAMP);
