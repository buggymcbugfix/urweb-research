PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Io_shop_orderIds (id INTEGER PRIMARY KEY AUTOINCREMENT);
CREATE TABLE uw_Io_shop_cart (
    uw_Id integer NOT NULL,
    uw_Item text NOT NULL,
    uw_Qty integer NOT NULL,
    uw_Price integer NOT NULL
);
INSERT INTO uw_Io_shop_cart VALUES(2,'lamp',1,150);
INSERT INTO uw_Io_shop_cart VALUES(3,'desk',1,250);
CREATE TABLE uw_Io_shop_orders (
    uw_Id integer NOT NULL,
    uw_CartId integer NOT NULL,
    uw_Total integer NOT NULL,
    uw_Status text NOT NULL,
    CONSTRAINT uw_Io_shop_orders_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Io_shop_orders VALUES(1,1,60,'paid');
INSERT INTO uw_Io_shop_orders VALUES(2,2,150,'declined');
INSERT INTO uw_Io_shop_orders VALUES(3,3,250,'paid');
CREATE TABLE uw_Io_shop_payments (
    uw_OrderId integer NOT NULL,
    uw_Attempt integer NOT NULL,
    uw_Outcome text NOT NULL
);
INSERT INTO uw_Io_shop_payments VALUES(1,1,'approved');
INSERT INTO uw_Io_shop_payments VALUES(2,1,'declined');
INSERT INTO uw_Io_shop_payments VALUES(3,1,'no answer');
INSERT INTO uw_Io_shop_payments VALUES(3,2,'approved');
CREATE TABLE uw_Io_shop_watchers (
    uw_Ch integer NOT NULL
);
INSERT INTO uw_Io_shop_watchers VALUES(0);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('uw_Io_shop_orderIds',3);
PRAGMA writable_schema=OFF;
COMMIT;
