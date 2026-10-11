PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Shop_ids (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Shop_item (
    uw_Id integer NOT NULL,
    uw_Name text NOT NULL,
    uw_Price real NOT NULL,
    uw_Created text NOT NULL,
    CONSTRAINT uw_Shop_item_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Shop_tag (
    uw_Item integer NOT NULL,
    uw_Tag text NOT NULL,
    CONSTRAINT uw_Shop_tag_pkey PRIMARY KEY (uw_Item, uw_Tag),
    CONSTRAINT uw_Shop_tag_Item FOREIGN KEY (uw_Item) REFERENCES uw_Shop_item (uw_Id)
);

CREATE VIEW uw_Shop_cheap AS
SELECT T_Item.uw_Name AS uw_Name FROM uw_Shop_item AS T_Item WHERE (T_Item.uw_Price < 10);

