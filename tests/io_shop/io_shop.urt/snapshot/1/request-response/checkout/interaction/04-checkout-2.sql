-- 2000-01-01 00:00:03 UTC POST /Io_shop/checkout/2
-- nextval 1/2
INSERT INTO uw_Io_shop_orderIds VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Io_shop_orderIds;
-- tests/io_shop/io_shop.ur:53:16-54:44 (1 row)
INSERT INTO uw_Io_shop_orders (uw_CartId, uw_Id, uw_Status, uw_Total) VALUES (2, 2, 'pending', 150);
-- 2000-01-01 00:00:03 UTC POST /Io_shop/checkout/2
-- tests/io_shop/io_shop.ur:31:4-38:17 (1 row)
INSERT INTO uw_Io_shop_payments (uw_Attempt, uw_OrderId, uw_Outcome) VALUES (1, 2, 'declined');
-- $/basis.urs:728:0-728:33 (1 row)
UPDATE uw_Io_shop_orders SET uw_Status = 'declined' WHERE (uw_Id = 2);
