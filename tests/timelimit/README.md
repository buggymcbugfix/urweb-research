A handler that never ends, under `limit time 2` (a fifth of a second). The
server urt starts forks to daemonize (-d3), and the thread that advances the
clock the limit is read from has to be started after that fork, or the
handler would run for ever. The run shows it stopped, and the server going
on to answer.
