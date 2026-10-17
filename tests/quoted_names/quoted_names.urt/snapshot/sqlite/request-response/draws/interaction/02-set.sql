-- 2000-01-01 00:00:01 UTC GET /Quoted_names/set/10
-- tests/quoted_names/quoted_names.ur:24:4-25:47 (1 row)
UPDATE sqlite_sequence SET seq = 10 WHERE name = 'Quoted_names_s''';
