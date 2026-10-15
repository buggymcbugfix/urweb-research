PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Tree_s (id INTEGER PRIMARY KEY AUTOINCREMENT);

CREATE TABLE uw_Tree_t (
    uw_Id integer NOT NULL,
    uw_Parent integer,
    uw_Nam text NOT NULL,
    CONSTRAINT uw_Tree_t_pkey PRIMARY KEY (uw_Id),
    CONSTRAINT uw_Tree_t_F FOREIGN KEY (uw_Parent) REFERENCES uw_Tree_t (uw_Id) ON DELETE CASCADE
);

