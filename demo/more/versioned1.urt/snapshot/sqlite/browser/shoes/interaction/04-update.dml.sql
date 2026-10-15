-- 2100-01-01 00:01:06.820 UTC POST /Versioned1/update/1/Carlos/45
-- nextval 1/2
INSERT INTO uw_Versioned1_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Versioned1_s;
-- demo/more/versioned.ur:114:12-129:15 (1 row)
INSERT INTO uw_Versioned1_t (uw_Id, uw_Nam, uw_ShoeSize, uw_Version, uw_When) VALUES (1, NULL, 45, 3, CURRENT_TIMESTAMP);
