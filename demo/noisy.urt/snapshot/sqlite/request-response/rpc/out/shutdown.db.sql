PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Noisy_t (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    CONSTRAINT uw_Noisy_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_Noisy_t VALUES(2,'second one');
COMMIT;
