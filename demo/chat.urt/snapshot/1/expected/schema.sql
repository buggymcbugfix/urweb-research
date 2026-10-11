CREATE SEQUENCE uw_Chat_Room_s;

CREATE TABLE uw_Chat_Room_t (
    uw_Id int8 NOT NULL,
    uw_Client int4 NOT NULL,
    uw_Channel int8 NOT NULL,
    CONSTRAINT uw_Chat_Room_t_pkey PRIMARY KEY (uw_Id, uw_Client)
);

CREATE SEQUENCE uw_Chat_s;

CREATE TABLE uw_Chat_t (
    uw_Id int8 NOT NULL,
    uw_Title text NOT NULL,
    uw_Room int8 NOT NULL,
    CONSTRAINT uw_Chat_t_pkey PRIMARY KEY (uw_Id)
);

