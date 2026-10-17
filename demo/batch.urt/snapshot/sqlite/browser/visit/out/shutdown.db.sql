PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Batch_t (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    CONSTRAINT uw_Batch_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Batch_t VALUES(8,'eight');
COMMIT;
