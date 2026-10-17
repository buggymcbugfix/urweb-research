-- 2000-01-01 00:00:01 UTC GET /NomangleSqlite/main
-- nextval 1/2
INSERT INTO orderSeq VALUES (NULL);
-- nextval 2/2
DELETE FROM orderSeq;
-- tests/nomangleSqlite.ur:20:4-32:17 (1 row)
INSERT INTO person (FirstName, Id, "Order", "Select") VALUES ('Alice2', 2, 3, 's');
-- tests/nomangleSqlite.ur:21:4-32:17 (1 row)
INSERT INTO "order" ("Group", "Limit", OrderId, Person) VALUES ('vip', 1.5, 2, 2);
-- tests/nomangleSqlite.ur:22:4-32:17 (1 row)
UPDATE person SET "Select" = 'updated' WHERE (Id = 2);
