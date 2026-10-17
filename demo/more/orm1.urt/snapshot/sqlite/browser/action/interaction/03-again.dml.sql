-- 2100-01-01 00:00:02.150 UTC POST /Orm1/action
-- nextval 1/2
INSERT INTO uw_Orm1_T_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_T_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_T_t (uw_A, uw_B, uw_Id) VALUES (3, 'Hi', 4);
-- demo/more/orm1.ur:15:4-44:2 (1 row)
UPDATE uw_Orm1_T_t SET uw_A = 3, uw_B = 'Bye' WHERE (uw_Id = 4);
-- nextval 1/2
INSERT INTO uw_Orm1_T_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_T_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_T_t (uw_A, uw_B, uw_Id) VALUES (4, 'Why', 5);
-- nextval 1/2
INSERT INTO uw_Orm1_T_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_T_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_T_t (uw_A, uw_B, uw_Id) VALUES (66, 'Hi', 6);
-- nextval 1/2
INSERT INTO uw_Orm1_S_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Orm1_S_s;
-- demo/more/orm.ur:42:8-43:31 (1 row)
INSERT INTO uw_Orm1_S_t (uw_C, uw_D, uw_Id) VALUES (4, 45.67, 2);
-- $/basis.urs:722:0-722:33 (1 row)
DELETE FROM uw_Orm1_T_t WHERE (uw_Id = 4);
-- $/basis.urs:722:0-722:33 (1 row)
DELETE FROM uw_Orm1_T_t WHERE (uw_Id = 5);
-- $/basis.urs:722:0-722:33 (1 row)
DELETE FROM uw_Orm1_T_t WHERE (uw_Id = 6);
-- demo/more/orm1.ur:31:4-44:2 (1 row)
DELETE FROM uw_Orm1_S_t WHERE (uw_Id = 2);
