Failed transactions of an io task give back what they allocated: twenty of
them allocate 4 MiB each and fail, under a heap limit of 320 MB of which
the runner's starting heap takes 256 MiB, and a 32 MiB string is built
afterwards.  The run waits for the second tick.
