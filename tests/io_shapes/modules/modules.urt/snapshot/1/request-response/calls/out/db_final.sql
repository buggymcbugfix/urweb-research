PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Modules_t (
    uw_N integer NOT NULL,
    uw_S text NOT NULL
);
INSERT INTO uw_Modules_t VALUES(1,'M.f');
INSERT INTO uw_Modules_t VALUES(1,'M.f');
INSERT INTO uw_Modules_t VALUES(2,'M.Inner.g');
INSERT INTO uw_Modules_t VALUES(2,'F1');
INSERT INTO uw_Modules_t VALUES(3,'F2');
INSERT INTO uw_Modules_t VALUES(0,'int:5');
INSERT INTO uw_Modules_t VALUES(0,'string:5!');
INSERT INTO uw_Modules_t VALUES(0,'bool:True');
INSERT INTO uw_Modules_t VALUES(7,'runTwice');
INSERT INTO uw_Modules_t VALUES(7,'runTwice');
INSERT INTO uw_Modules_t VALUES(1,'a:before');
INSERT INTO uw_Modules_t VALUES(1,'M.f');
INSERT INTO uw_Modules_t VALUES(1,'b:before');
INSERT INTO uw_Modules_t VALUES(2,'M.f');
INSERT INTO uw_Modules_t VALUES(3,'M.Inner.g');
INSERT INTO uw_Modules_t VALUES(2,'b:after');
INSERT INTO uw_Modules_t VALUES(2,'a:after');
COMMIT;
