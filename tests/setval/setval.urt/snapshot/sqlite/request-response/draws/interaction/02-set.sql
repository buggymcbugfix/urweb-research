-- 2000-01-01 00:00:01 UTC GET /Setval/set/10
-- tests/setval/setval.ur:11:4-12:47 (1 row)
UPDATE sqlite_sequence SET seq = 10 WHERE name = 'uw_Setval_s';
