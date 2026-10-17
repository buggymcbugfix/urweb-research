Times at the edges of what SQLite's date functions read, whose years have
four digits: one in the year 999 and one in 10000, each stored and read
back.  Then fromMilliseconds of a negative number that is not a whole
number of seconds, which gives a time whose microseconds are out of range,
4294467296, and the same time from a URL, as -1500000 microseconds, which
gives the same.
