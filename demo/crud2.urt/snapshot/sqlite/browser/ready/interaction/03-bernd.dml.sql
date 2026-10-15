-- 2000-01-01 00:00:03.700 UTC POST /Crud2/create
-- nextval 1/2
INSERT INTO uw_Crud2_seq VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Crud2_seq;
-- demo/crud.ur:94:8-108:2 (1 row)
INSERT INTO uw_Crud2_t (uw_Id, uw_Name, uw_Ready) VALUES (2, 'Bernd', 1);
