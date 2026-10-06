Two requests that each read what the other writes: one of them has to fail at COMMIT.
The application runs with -t 4 (args), since one thread would serve them one after the other.
