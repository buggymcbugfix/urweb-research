-- 2100-01-01 00:00:01 UTC POST /Crud3/create
-- nextval 1/2
INSERT INTO uw_Crud3_seq VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Crud3_seq;
-- demo/crud.ur:94:8-108:2 (1 row)
INSERT INTO uw_Crud3_t (uw_Id, uw_Text) VALUES (1, 'hello world');
