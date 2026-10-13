-- 2000-01-01 00:00:01 UTC POST /Batch/doBatch/Cons/1/one/Cons/2/two%20words/Nil
-- demo/batch.ur:15:8-16:19 (1 row)
INSERT INTO uw_Batch_t (uw_A, uw_Id) VALUES ('one', 1);
-- demo/batch.ur:15:8-16:19 (1 row)
INSERT INTO uw_Batch_t (uw_A, uw_Id) VALUES ('two words', 2);
