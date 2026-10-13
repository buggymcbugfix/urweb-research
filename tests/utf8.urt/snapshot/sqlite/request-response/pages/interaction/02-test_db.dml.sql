-- 2000-01-01 00:00:01 UTC GET /Utf8/test_db
-- tests/utf8.ur:1464:4-1492:3 (1 row)
INSERT INTO uw_Utf8_t (uw_Id, uw_Text) VALUES (1, 'abc');
-- tests/utf8.ur:1467:4-1492:3 (1 row)
INSERT INTO uw_Utf8_t (uw_Id, uw_Text) VALUES (2, 'çãó');
-- tests/utf8.ur:1470:4-1492:3 (1 row)
INSERT INTO uw_Utf8_t (uw_Id, uw_Text) VALUES (3, 'が');
-- tests/utf8.ur:1473:4-1492:3 (1 row)
INSERT INTO uw_Utf8_t (uw_Id, uw_Text) VALUES (4, '漢');
-- tests/utf8.ur:1476:4-1492:3 (1 row)
INSERT INTO uw_Utf8_t (uw_Id, uw_Text) VALUES (5, 'カ');
-- tests/utf8.ur:1479:4-1492:3 (1 row)
INSERT INTO uw_Utf8_t (uw_Id, uw_Text) VALUES (6, 'وظيفية');
