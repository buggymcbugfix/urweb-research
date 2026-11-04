Handlers whose only side effect is a cookie written, and what the
compiler makes of that. `link`: such a handler in the main module's
signature, a GET link. `rpc`: an RPC that reads the cookie and writes it,
called without and with the cookie signature. `action`: an io action that
reads the cookie and writes it, posted without the signature, with the
one of the page the form came in, and with that one and another cookie.
