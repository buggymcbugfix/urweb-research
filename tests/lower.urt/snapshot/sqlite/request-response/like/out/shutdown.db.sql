PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Lower_lower (
    uw_A text NOT NULL
);
INSERT INTO uw_Lower_lower VALUES('foo');
INSERT INTO uw_Lower_lower VALUES('FOO');
INSERT INTO uw_Lower_lower VALUES('barFoo');
INSERT INTO uw_Lower_lower VALUES('food');
INSERT INTO uw_Lower_lower VALUES('ÇFOO');
COMMIT;
