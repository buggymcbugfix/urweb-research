PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE person (
    Id integer NOT NULL,
    FirstName text NOT NULL,
    "Select" text NOT NULL,
    "Order" integer NOT NULL,
    CONSTRAINT person_pkey PRIMARY KEY (Id),
    CONSTRAINT person_UniqueName UNIQUE (FirstName)
);
INSERT INTO person VALUES(1,'Alice1','updated',3);
INSERT INTO person VALUES(2,'Alice2','updated',3);
CREATE TABLE IF NOT EXISTS "order" (
    OrderId integer NOT NULL,
    Person integer NOT NULL,
    "Group" text NOT NULL,
    "Limit" real NOT NULL,
    CONSTRAINT order_pkey PRIMARY KEY (OrderId),
    CONSTRAINT order_Person FOREIGN KEY (Person) REFERENCES person (Id) ON DELETE CASCADE
);
INSERT INTO "order" VALUES(1,1,'vip',1.5);
INSERT INTO "order" VALUES(2,2,'vip',1.5);
CREATE TABLE orderSeq (id INTEGER PRIMARY KEY AUTOINCREMENT);
PRAGMA writable_schema=ON;
CREATE TABLE IF NOT EXISTS sqlite_sequence(name,seq);
DELETE FROM sqlite_sequence;
INSERT INTO sqlite_sequence VALUES('orderSeq',2);
CREATE VIEW personOrders AS
SELECT T_Person.FirstName AS FirstName, T_Order."Group" AS "Group", T_Order."Limit" AS "Limit" FROM person AS T_Person JOIN "order" AS T_Order ON (T_Order.Person = T_Person.Id);
CREATE INDEX order_Person_Group ON "order" (Person, "Group");
PRAGMA writable_schema=OFF;
COMMIT;
