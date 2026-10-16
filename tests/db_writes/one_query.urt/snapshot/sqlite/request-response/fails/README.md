Both handlers write a row through Insert.row and then fail, so that the
row should be rolled back: rows counts the rows left behind.
