PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Sql_time_t (
    uw_Id integer NOT NULL,
    uw_T text NOT NULL
);

INSERT INTO uw_Sql_time_t VALUES (1, '2000-01-01 12:34:56');
INSERT INTO uw_Sql_time_t VALUES (2, '2000-01-01 12:34:56.789');
INSERT INTO uw_Sql_time_t VALUES (3, '2000-01-01T12:34:56Z');
INSERT INTO uw_Sql_time_t VALUES (4, '2000-01-01 14:34:56+02:00');
INSERT INTO uw_Sql_time_t VALUES (5, '2000-01-01 12:34');
INSERT INTO uw_Sql_time_t VALUES (6, '2000-01-01');
INSERT INTO uw_Sql_time_t VALUES (7, 'yesterday');
