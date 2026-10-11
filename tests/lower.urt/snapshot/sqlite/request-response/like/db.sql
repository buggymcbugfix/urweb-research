PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE uw_Lower_lower (
    uw_A text NOT NULL
);

INSERT INTO uw_Lower_lower VALUES ('foo'), ('FOO'), ('barFoo'), ('food'), ('ÇFOO');
