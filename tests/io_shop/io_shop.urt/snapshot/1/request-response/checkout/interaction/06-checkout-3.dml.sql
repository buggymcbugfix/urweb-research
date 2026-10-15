-- 2100-01-01 00:00:05 UTC POST /Io_shop/checkout/3
-- nextval 1/2
INSERT INTO uw_Io_shop_orderIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Io_shop_orderIds;
-- tests/io_shop/io_shop.ur:53:16-54:44 (1 row)
INSERT INTO uw_Io_shop_orders (uw_CartId, uw_Id, uw_Status, uw_Total) VALUES (3, 3, 'pending', 250);
-- 2100-01-01 00:00:05 UTC POST /Io_shop/checkout/3
-- tests/io_shop/io_shop.ur:31:4-38:17 (1 row)
INSERT INTO uw_Io_shop_payments (uw_Attempt, uw_OrderId, uw_Outcome) VALUES (1, 3, 'no answer');
