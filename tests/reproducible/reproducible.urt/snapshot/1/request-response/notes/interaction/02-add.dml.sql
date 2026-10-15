-- 2100-01-01 00:00:01 UTC POST /add
-- nextval 1/2
INSERT INTO uw_noteIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_noteIds;
-- tests/reproducible/reproducible.ur:32:4-38:28 (1 row)
INSERT INTO uw_Note (uw_Created, uw_Id, uw_Text) VALUES (CURRENT_TIMESTAMP, 1, '1st (somebody)');
-- tests/reproducible/reproducible.ur:34:4-38:28 (1 row)
UPDATE uw_Note SET uw_Text = 'first' WHERE ((uw_Id = 1) AND (uw_Text = '1st (somebody)'));
