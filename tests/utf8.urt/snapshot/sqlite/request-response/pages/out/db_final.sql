PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Utf8_t (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL
);
INSERT INTO uw_Utf8_t VALUES(1,'abc');
INSERT INTO uw_Utf8_t VALUES(2,'çãó');
INSERT INTO uw_Utf8_t VALUES(3,'が');
INSERT INTO uw_Utf8_t VALUES(4,'漢');
INSERT INTO uw_Utf8_t VALUES(5,'カ');
INSERT INTO uw_Utf8_t VALUES(6,'وظيفية');
COMMIT;
