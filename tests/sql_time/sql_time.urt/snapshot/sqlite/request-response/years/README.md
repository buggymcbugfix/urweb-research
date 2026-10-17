Times at the edges of what SQLite's date functions read, whose years have
four digits: one in the year 999 and one in 10000, each stored and read
back.  Then a time before the epoch that is not a whole number of
seconds, from fromMilliseconds (-1500) and from a URL (-1500000
microseconds), each stored and read back.
