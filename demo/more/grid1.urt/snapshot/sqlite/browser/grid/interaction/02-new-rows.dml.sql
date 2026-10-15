-- 2100-01-01 00:00:01.750 UTC POST /Grid1/new
-- nextval 1/2
INSERT INTO uw_Grid1_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Grid1_s;
-- demo/more/dbgrid.ur:396:27-397:37 (1 row)
INSERT INTO uw_Grid1_t (uw_A, uw_B, uw_C, uw_D, uw_E, uw_F, uw_Id) VALUES (0, '', 0, 0, NULL, NULL, 1);
-- 2100-01-01 00:00:02.350 UTC POST /Grid1/new
-- nextval 1/2
INSERT INTO uw_Grid1_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Grid1_s;
-- demo/more/dbgrid.ur:396:27-397:37 (1 row)
INSERT INTO uw_Grid1_t (uw_A, uw_B, uw_C, uw_D, uw_E, uw_F, uw_Id) VALUES (0, '', 0, 0, NULL, NULL, 2);
