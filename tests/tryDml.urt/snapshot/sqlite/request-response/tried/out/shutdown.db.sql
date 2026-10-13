PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_TryDml_t (
    uw_Id integer NOT NULL,
    CONSTRAINT uw_TryDml_t_pkey PRIMARY KEY (uw_Id)
);
INSERT INTO uw_TryDml_t VALUES(0);
INSERT INTO uw_TryDml_t VALUES(1);
INSERT INTO uw_TryDml_t VALUES(2);
INSERT INTO uw_TryDml_t VALUES(3);
COMMIT;
