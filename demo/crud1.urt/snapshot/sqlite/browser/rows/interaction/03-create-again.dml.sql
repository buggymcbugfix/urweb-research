-- 2000-01-01 00:00:02.650 UTC POST /Crud1/create
-- nextval 1/2
INSERT INTO uw_Crud1_seq VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Crud1_seq;
-- demo/crud.ur:94:8-108:2 (1 row)
INSERT INTO uw_Crud1_t1 (uw_A, uw_B, uw_C, uw_D, uw_Id) VALUES (2, 'second', 2.25, 0, 2);
