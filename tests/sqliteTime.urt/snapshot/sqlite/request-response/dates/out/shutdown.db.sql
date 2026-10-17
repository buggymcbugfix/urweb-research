PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE uw_SqliteTime_dates (
    uw_Date text NOT NULL
);
INSERT INTO uw_SqliteTime_dates VALUES('2003-01-02 04:06:07');
INSERT INTO uw_SqliteTime_dates VALUES('2003-01-02 04:06:07');
COMMIT;
