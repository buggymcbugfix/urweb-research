PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Sql_char_t (
    uw_Id integer NOT NULL,
    uw_C text NOT NULL
);
INSERT INTO uw_Sql_char_t VALUES(1,'é');
COMMIT;
