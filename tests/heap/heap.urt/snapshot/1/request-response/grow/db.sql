CREATE TABLE uw_T (
    uw_Id integer NOT NULL,
    uw_Payload text NOT NULL,
    CONSTRAINT uw_T_pkey PRIMARY KEY (uw_Id)
);
-- 500 rows of some 20 bytes each
WITH RECURSIVE n(i) AS (SELECT 1 UNION ALL SELECT i + 1 FROM n WHERE i < 500)
INSERT INTO uw_T SELECT i, i || ':0123456789abcdef' FROM n;
