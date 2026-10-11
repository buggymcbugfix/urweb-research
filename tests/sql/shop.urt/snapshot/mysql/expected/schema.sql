CREATE TABLE uw_shop_ids (uw_id INTEGER PRIMARY KEY AUTO_INCREMENT);

CREATE TABLE uw_shop_item (
    uw_id bigint NOT NULL,
    uw_name longtext NOT NULL,
    uw_price double NOT NULL,
    uw_created timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CONSTRAINT uw_shop_item_pkey PRIMARY KEY (uw_id)
);

CREATE TABLE uw_shop_tag (
    uw_item bigint NOT NULL,
    uw_tag longtext NOT NULL,
    CONSTRAINT uw_shop_tag_pkey PRIMARY KEY (uw_item, uw_tag),
    CONSTRAINT uw_shop_tag_Item FOREIGN KEY (uw_item) REFERENCES uw_shop_item (uw_id)
);

CREATE VIEW uw_shop_cheap AS
SELECT T_Item.uw_name AS uw_name FROM uw_shop_item AS T_Item WHERE (T_Item.uw_price < 10);

