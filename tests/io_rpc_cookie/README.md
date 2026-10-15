An io RPC that reads a cookie in one transaction and writes the database in
another is a ReadCookieWrite export, and its request must carry the cookie
signature, as a transaction RPC's must.  The run loads the page, which sets
the cookie and carries the signature, then calls the function with that
signature, with none, with a wrong one, and with the signature of another
cookie value, and reads the rows: only the first call is served.
