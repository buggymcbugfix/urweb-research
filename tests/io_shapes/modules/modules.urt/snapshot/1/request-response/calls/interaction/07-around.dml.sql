-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:54:4-54:79 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (1, 'a:before');
-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:9:24-9:88 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (1, 'M.f');
-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:54:4-54:79 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (1, 'b:before');
-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:9:24-9:88 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (2, 'M.f');
-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:15:28-15:98 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (3, 'M.Inner.g');
-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:56:4-56:78 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (2, 'b:after');
-- 2100-01-01 00:00:06 UTC POST /Modules/useAround/1
-- tests/io_shapes/modules/modules.ur:56:4-56:78 (1 row)
INSERT INTO uw_Modules_t (uw_N, uw_S) VALUES (2, 'a:after');
