-- 2000-01-01 00:00:01 UTC POST /Io_shop/checkout/1
-- nextval 1/2
INSERT INTO uw_Io_shop_orderIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Io_shop_orderIds;
-- tests/io_shop/io_shop.ur:53:16-54:44 (1 row)
INSERT INTO uw_Io_shop_orders (uw_CartId, uw_Id, uw_Status, uw_Total) VALUES (1, 1, 'pending', 60);
-- 2000-01-01 00:00:01 UTC POST /Io_shop/checkout/1
-- tests/io_shop/io_shop.ur:31:4-38:17 (1 row)
INSERT INTO uw_Io_shop_payments (uw_Attempt, uw_OrderId, uw_Outcome) VALUES (1, 1, 'approved');
-- tests/io_shop/io_shop.ur:33:8-34:50 (1 row)
UPDATE uw_Io_shop_orders SET uw_Status = 'paid' WHERE (uw_Id = 1);
-- $/basis.urs:728:0-728:33 (2 rows)
DELETE FROM uw_Io_shop_cart WHERE (uw_Id = 1);
