-- The second statement fails: the database would be left with t alone.
CREATE TABLE t (x INTEGER);
CREATE TABLE u (;
CREATE TABLE v (y INTEGER);
