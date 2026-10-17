PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Grid1_t1 (
    uw_Id integer NOT NULL,
    uw_A text NOT NULL,
    CONSTRAINT uw_Grid1_t1_pkey PRIMARY KEY (uw_Id)
);

CREATE TABLE uw_Grid1_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Grid1_t (
    uw_Id integer NOT NULL,
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C integer NOT NULL,
    uw_D integer NOT NULL,
    uw_E integer,
    uw_F integer,
    CONSTRAINT uw_Grid1_t_pkey PRIMARY KEY (uw_Id),
    CONSTRAINT uw_Grid1_t_Foreign FOREIGN KEY (uw_D) REFERENCES uw_Grid1_t1 (uw_Id) ON DELETE CASCADE
);


-- What the D and F columns choose from; a new row's D is 0.
INSERT INTO uw_Grid1_t1 (uw_Id, uw_A) VALUES (0, 'none'), (1, 'kettle'), (2, 'cups');
