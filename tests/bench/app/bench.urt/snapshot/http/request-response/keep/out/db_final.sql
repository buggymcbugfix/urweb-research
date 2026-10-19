PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Item (
    uw_Id integer NOT NULL,
    uw_Text text NOT NULL,
    CONSTRAINT uw_Item_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Item VALUES(1,'item 1');
INSERT INTO uw_Item VALUES(2,'item 2');
INSERT INTO uw_Item VALUES(3,'item 3');
CREATE TABLE uw_Log (
    uw_N integer NOT NULL,
    uw_Note text NOT NULL
);
INSERT INTO uw_Log VALUES(1,'written');
COMMIT;
