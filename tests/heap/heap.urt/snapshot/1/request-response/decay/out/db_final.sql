PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_T (
    uw_Id integer NOT NULL,
    uw_Payload text NOT NULL,
    CONSTRAINT uw_T_pkey PRIMARY KEY (uw_Id)
);
COMMIT;
