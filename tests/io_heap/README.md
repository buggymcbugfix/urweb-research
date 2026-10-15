Failed transactions of an io task give back what they allocated: twenty of
them allocate 4 MiB each and fail, under a heap limit of 100 MB, and a
32 MiB string is built afterwards, which would not fit if they had not.  The run waits for the second tick.
