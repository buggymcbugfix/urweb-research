-- 2000-01-01 00:00:00 UTC GET /Setval/set/5
-- tests/setval/setval.ur:11:4-12:47 (0 rows)
UPDATE sqlite_sequence SET seq = 5 WHERE name = 'uw_Setval_s';
-- tests/setval/setval.ur:11:4-12:47 (1 row)
INSERT INTO sqlite_sequence (name, seq) VALUES ('uw_Setval_s', 5);
