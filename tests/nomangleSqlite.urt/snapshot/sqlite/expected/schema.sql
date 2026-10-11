PRAGMA foreign_keys = ON;
PRAGMA journal_mode = WAL;

CREATE TABLE person (
    Id integer NOT NULL,
    FirstName text NOT NULL,
    "Select" text NOT NULL,
    "Order" integer NOT NULL,
    CONSTRAINT person_pkey PRIMARY KEY (Id),
    CONSTRAINT person_UniqueName UNIQUE (FirstName)
);

CREATE TABLE "order" (
    OrderId integer NOT NULL,
    Person integer NOT NULL,
    "Group" text NOT NULL,
    "Limit" real NOT NULL,
    CONSTRAINT order_pkey PRIMARY KEY (OrderId),
    CONSTRAINT order_Person FOREIGN KEY (Person) REFERENCES person (Id) ON DELETE CASCADE
);

CREATE VIEW personOrders AS
SELECT T_Person.FirstName AS FirstName, T_Order."Group" AS "Group", T_Order."Limit" AS "Limit" FROM person AS T_Person JOIN "order" AS T_Order ON (T_Order.Person = T_Person.Id);

CREATE INDEX order_Person_Group ON "order" (Person, "Group");

CREATE TABLE orderSeq (id INTEGER PRIMARY KEY AUTOINCREMENT);

