-- 2000-01-01 00:00:05 UTC
-- tests/io_shop/io_shop.ur:31:4-38:17 (1 row)
INSERT INTO uw_Io_shop_payments (uw_Attempt, uw_OrderId, uw_Outcome) VALUES (2, 3, 'approved');
-- tests/io_shop/io_shop.ur:33:8-34:50 (1 row)
UPDATE uw_Io_shop_orders SET uw_Status = 'paid' WHERE (uw_Id = 3);
