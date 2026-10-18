A FastCGI program with its default single thread, and two connections
to it: one kept open and left idle, as a web server with keep-alive
leaves a connection after a request, and a second one carrying a
request while the first idles.  `run.sh` starts the program on a socket
with spawn-fcgi, `client.py` plays the web server, and the transcript
in `expected` says whether the second request is answered.
