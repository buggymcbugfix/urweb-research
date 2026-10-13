-- 2000-01-01 00:00:01 UTC POST /Chat/create
-- nextval 1/2
INSERT INTO uw_Chat_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Chat_s;
-- nextval 1/2
INSERT INTO uw_Chat_Room_s VALUES (NULL);
-- nextval 2/2
DELETE FROM uw_Chat_Room_s;
-- demo/chat.ur:73:12-74:19 (1 row)
INSERT INTO uw_Chat_t (uw_Id, uw_Room, uw_Title) VALUES (1, 1, 'lounge');
