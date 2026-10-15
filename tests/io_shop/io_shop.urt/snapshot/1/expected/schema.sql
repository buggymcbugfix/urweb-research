PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Io_shop_orderIds (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Io_shop_cart (
    uw_Id integer NOT NULL,
    uw_Item text NOT NULL,
    uw_Qty integer NOT NULL,
    uw_Price integer NOT NULL
);

CREATE TABLE uw_Io_shop_orders (
    uw_Id integer NOT NULL,
    uw_CartId integer NOT NULL,
    uw_Total integer NOT NULL,
    uw_Status text NOT NULL,
    CONSTRAINT uw_Io_shop_orders_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Io_shop_payments (
    uw_OrderId integer NOT NULL,
    uw_Attempt integer NOT NULL,
    uw_Outcome text NOT NULL
);

CREATE TABLE uw_Io_shop_watchers (
    uw_Ch integer NOT NULL
);

