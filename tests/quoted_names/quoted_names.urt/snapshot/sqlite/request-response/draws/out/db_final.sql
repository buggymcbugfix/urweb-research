PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE IF NOT EXISTS "Quoted_names_t'" (
    N integer NOT NULL
);
CREATE TABLE IF NOT EXISTS "Quoted_names_s'" (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('Quoted_names_s''',11);
PRAGMA writable_schema=OFF;
COMMIT;
