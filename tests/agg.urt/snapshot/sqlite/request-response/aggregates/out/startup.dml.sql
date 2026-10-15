-- 2100-01-01 00:00:00 UTC
-- tests/agg.ur:12:5-19:65 (1 row)
INSERT INTO uw_Agg_t1 (uw_A, uw_B, uw_C) VALUES (1, 'a', 1);
-- tests/agg.ur:13:5-19:65 (1 row)
INSERT INTO uw_Agg_t1 (uw_A, uw_B, uw_C) VALUES (2, 'b', 2);
-- tests/agg.ur:14:5-19:65 (1 row)
INSERT INTO uw_Agg_t1 (uw_A, uw_B, uw_C) VALUES (50, 'c', 99);
-- tests/agg.ur:15:5-19:65 (1 row)
INSERT INTO uw_Agg_t2 (uw_A, uw_D, uw_E) VALUES (1, 1, NULL);
-- tests/agg.ur:16:5-19:65 (1 row)
INSERT INTO uw_Agg_t2 (uw_A, uw_D, uw_E) VALUES (1, 2, 'a');
-- tests/agg.ur:17:5-19:65 (1 row)
INSERT INTO uw_Agg_t2 (uw_A, uw_D, uw_E) VALUES (1, 3, NULL);
-- tests/agg.ur:18:5-19:65 (1 row)
INSERT INTO uw_Agg_t2 (uw_A, uw_D, uw_E) VALUES (1, 3, 'b');
-- tests/agg.ur:18:5-19:65 (1 row)
INSERT INTO uw_Agg_t2 (uw_A, uw_D, uw_E) VALUES (1, 3, 'c');
