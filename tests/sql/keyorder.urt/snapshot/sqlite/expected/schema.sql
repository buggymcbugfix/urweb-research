PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Keyorder_u (
    uw_X integer NOT NULL,
    uw_Y text NOT NULL,
    uw_Z integer NOT NULL,
    CONSTRAINT uw_Keyorder_u_pkey PRIMARY KEY (uw_Z, uw_X, uw_Y)
);

CREATE TABLE uw_Keyorder_t (
    uw_A integer NOT NULL,
    uw_B text NOT NULL,
    uw_C integer NOT NULL,
    uw_D integer NOT NULL,
    CONSTRAINT uw_Keyorder_t_pkey PRIMARY KEY (uw_C, uw_A, uw_B),
    CONSTRAINT uw_Keyorder_t_Uni UNIQUE (uw_D, uw_C, uw_A),
    CONSTRAINT uw_Keyorder_t_Man UNIQUE (uw_D, uw_C, uw_A),
    CONSTRAINT uw_Keyorder_t_Ref UNIQUE (uw_D, uw_C, uw_A),
    CONSTRAINT uw_Keyorder_t_Fk FOREIGN KEY (uw_A, uw_B, uw_C) REFERENCES uw_Keyorder_u (uw_X, uw_Y, uw_Z)
);

