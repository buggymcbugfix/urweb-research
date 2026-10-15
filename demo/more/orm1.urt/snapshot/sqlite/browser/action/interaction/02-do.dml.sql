-- 2100-01-01 00:00:01.550 UTC POST /Orm1/action
-- nextval 1/2
INSERT INTO uw_Orm1_T_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_T_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_T_t (uw_A, uw_B, uw_Id) VALUES (3, 'Hi', 1);
-- demo/more/orm1.ur:15:4-44:2 (1 row)
UPDATE uw_Orm1_T_t SET uw_A = 3, uw_B = 'Bye' WHERE (uw_Id = 1);
-- nextval 1/2
INSERT INTO uw_Orm1_T_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_T_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_T_t (uw_A, uw_B, uw_Id) VALUES (4, 'Why', 2);
-- nextval 1/2
INSERT INTO uw_Orm1_T_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_T_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_T_t (uw_A, uw_B, uw_Id) VALUES (66, 'Hi', 3);
-- nextval 1/2
INSERT INTO uw_Orm1_S_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_S_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_S_t (uw_C, uw_D, uw_Id) VALUES (1, 45.67, 1);
-- $/basis.urs:720:0-720:33 (1 row)
DELETE FROM uw_Orm1_T_t WHERE (uw_Id = 1);
-- $/basis.urs:720:0-720:33 (1 row)
DELETE FROM uw_Orm1_T_t WHERE (uw_Id = 2);
-- $/basis.urs:720:0-720:33 (1 row)
DELETE FROM uw_Orm1_T_t WHERE (uw_Id = 3);
-- demo/more/orm1.ur:31:4-44:2 (1 row)
DELETE FROM uw_Orm1_S_t WHERE (uw_Id = 1);
