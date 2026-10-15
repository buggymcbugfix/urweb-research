-- 2000-01-01 00:00:01.550 UTC POST /Ref/mutate
-- nextval 1/2
INSERT INTO uw_Ref_IR_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Ref_IR_s;
-- demo/refFun.ur:14:8-15:17 (1 row)
INSERT INTO uw_Ref_IR_t (uw_Data, uw_Id) VALUES (3, 1);
-- nextval 1/2
INSERT INTO uw_Ref_IR_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Ref_IR_s;
-- demo/refFun.ur:14:8-15:17 (1 row)
INSERT INTO uw_Ref_IR_t (uw_Data, uw_Id) VALUES (7, 2);
-- nextval 1/2
INSERT INTO uw_Ref_SR_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Ref_SR_s;
-- demo/refFun.ur:14:8-15:17 (1 row)
INSERT INTO uw_Ref_SR_t (uw_Data, uw_Id) VALUES ('hi', 1);
-- demo/ref.ur:14:4-28:2 (1 row)
UPDATE uw_Ref_IR_t SET uw_Data = 10 WHERE (uw_Id = 2);
-- $/basis.urs:720:0-720:33 (1 row)
DELETE FROM uw_Ref_IR_t WHERE (uw_Id = 1);
-- $/basis.urs:720:0-720:33 (1 row)
DELETE FROM uw_Ref_IR_t WHERE (uw_Id = 2);
-- demo/ref.ur:22:4-28:2 (1 row)
DELETE FROM uw_Ref_SR_t WHERE (uw_Id = 1);
