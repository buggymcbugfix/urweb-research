-- 2100-01-01 00:00:02.600 UTC POST /Versioned1/insert/1/Carlos/44
-- nextval 1/2
INSERT INTO uw_Versioned1_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Versioned1_s;
-- demo/more/versioned.ur:32:8-40:66 (1 row)
INSERT INTO uw_Versioned1_t (uw_Id, uw_Nam, uw_ShoeSize, uw_Version, uw_When) VALUES (1, 'Carlos', 44, 1, CURRENT_TIMESTAMP);
