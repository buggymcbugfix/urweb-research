PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_User_user (
    uw_A integer NOT NULL
);
INSERT INTO uw_User_user VALUES(1);
INSERT INTO uw_User_user VALUES(2);
INSERT INTO uw_User_user VALUES(3);
COMMIT;
