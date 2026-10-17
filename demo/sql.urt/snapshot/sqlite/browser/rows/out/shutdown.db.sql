PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_Sql_t (
    uw_A integer NOT NULL,
    uw_B real NOT NULL,
    uw_C text NOT NULL,
    uw_D integer NOT NULL,
    CONSTRAINT uw_Sql_t_pkey PRIMARY KEY (uw_A)
);
INSERT INTO uw_Sql_t VALUES(2,0.75,'pear',0);
COMMIT;
