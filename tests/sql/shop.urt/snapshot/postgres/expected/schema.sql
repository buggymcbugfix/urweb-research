CREATE SEQUENCE uw_Shop_ids;

CREATE TABLE uw_Shop_item (
    uw_Id int8 NOT NULL,
    uw_Name text NOT NULL,
    uw_Price float8 NOT NULL,
    uw_Created timestamp NOT NULL,
    CONSTRAINT uw_Shop_item_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Shop_tag (
    uw_Item int8 NOT NULL,
    uw_Tag text NOT NULL,
    CONSTRAINT uw_Shop_tag_pkey PRIMARY KEY (uw_Item, uw_Tag),
    CONSTRAINT uw_Shop_tag_Item FOREIGN KEY (uw_Item) REFERENCES uw_Shop_item (uw_Id)
);

CREATE VIEW uw_Shop_cheap AS
SELECT T_Item.uw_Name AS uw_Name FROM uw_Shop_item AS T_Item WHERE (T_Item.uw_Price < 10::float8);

