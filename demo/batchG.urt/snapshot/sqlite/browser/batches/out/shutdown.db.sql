PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_BatchG_t (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    uw_B real NOT NULL,
    CONSTRAINT uw_BatchG_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_BatchG_t VALUES(1,'kettle',24.5);
COMMIT;
