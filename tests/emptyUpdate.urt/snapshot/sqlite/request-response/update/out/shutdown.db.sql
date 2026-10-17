PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_EmptyUpdate_a (
    uw_B integer NOT NULL
);
INSERT INTO uw_EmptyUpdate_a VALUES(1);
INSERT INTO uw_EmptyUpdate_a VALUES(2);
COMMIT;
