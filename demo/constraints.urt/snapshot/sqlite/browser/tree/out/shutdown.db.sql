PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Constraints_t (
    uw_Id integer NOT NULL,
    uw_Nam text NOT NULL,
    uw_Parent integer,
    CONSTRAINT uw_Constraints_t_pkey PRIMARY KEY (uw_Id),
    CONSTRAINT uw_Constraints_t_Nam UNIQUE (uw_Nam),
    CONSTRAINT uw_Constraints_t_Id CHECK (uw_Id >= 0),
    CONSTRAINT uw_Constraints_t_Parent FOREIGN KEY (uw_Parent) REFERENCES uw_Constraints_t (uw_Id)
);
INSERT INTO uw_Constraints_t VALUES(1,'root',NULL);
INSERT INTO uw_Constraints_t VALUES(2,'leaf',1);
COMMIT;
