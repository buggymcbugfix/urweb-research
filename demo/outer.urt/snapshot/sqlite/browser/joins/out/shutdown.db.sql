PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Outer_t (
    uw_Id integer NOT NULL,
    uw_B text NOT NULL,
    CONSTRAINT uw_Outer_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Outer_t VALUES(1,'one');
INSERT INTO uw_Outer_t VALUES(2,'two');
CREATE TABLE uw_Outer_u (
    uw_Id integer NOT NULL,
    uw_Link integer NOT NULL,
    uw_C text NOT NULL,
    uw_D real,
    CONSTRAINT uw_Outer_u_pkey PRIMARY KEY (uw_Id),
    CONSTRAINT uw_Outer_u_Link FOREIGN KEY (uw_Link) REFERENCES uw_Outer_t (uw_Id)
);
INSERT INTO uw_Outer_u VALUES(10,1,'ten',2.5);
INSERT INTO uw_Outer_u VALUES(11,1,'eleven',0.5);
COMMIT;
