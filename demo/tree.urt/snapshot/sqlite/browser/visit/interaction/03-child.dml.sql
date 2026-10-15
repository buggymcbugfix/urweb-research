-- 2000-01-01 00:00:02.980 UTC POST /Tree/add/Some/1
-- nextval 1/2
INSERT INTO uw_Tree_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Tree_s;
-- demo/tree.ur:32:4-33:11 (1 row)
INSERT INTO uw_Tree_t (uw_Id, uw_Nam, uw_Parent) VALUES (2, 'shelf', 1);
