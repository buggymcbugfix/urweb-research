The NixOS module `nixos/urweb.nix`, which runs an Ur/Web FastCGI program
as a systemd service behind nginx, and `urweb.buildApp`, which builds
the program as the module expects, tried in a virtual machine:

    nix-build tests/deploy/test.nix

`app` is a small application: a page that counts the rows of a table
and shows two variables of its environment, and one that inserts a row;
`app.nix` is its package, a `buildApp` call.  `test.nix` runs three
instances of it on one machine: `app` in a group of its own, whose
state nobody else can reach, with a variable from `environment` and a
secret from an environment file outside the store; `shop` and `erp`
sharing one database in a directory of a group (`database.path`,
`database.group`), which another user in the group reads and writes
with `shop-db` and `erp-db`, no root, while the programs go on serving,
`erp` with `network`.  It checks the sockets' owners and modes, the
state directories', the databases' and the keys', that the program's
lines reach the journal, that nothing the sandbox takes away was
needed, and prints `systemd-analyze security` for the three services
and for nginx beside them.  With KVM it takes about a minute; without,
emulated, some minutes.
