The NixOS module `nixos/urweb.nix`, which runs an Ur/Web FastCGI program
as a systemd service behind nginx, and `urweb.buildApp`, which builds
the program as the module expects, tried in a virtual machine:

    nix-build tests/deploy/test.nix

`app` is a small application: a page that counts the rows of a table
and shows three variables of its environment, and one that inserts a
row; `app.nix` is its package, a `buildApp` call.  `test.nix` runs four
instances of it on one machine: `app` in a group of its own, whose
state nobody else can reach, with a variable from `environment` and
two secrets from `credentials`, files outside the store that only root
reads; `shop` and `erp` sharing one database in a directory of a group
(`database.path`, `database.group`), which another user in the group
reads and writes with `shop-db` and `erp-db`, no root, while the
programs go on serving, `erp` with `network` and one of the two
secrets; and `pg`, built for Postgres with the prefix `/pg/` and served
on app's virtual host.  The script fails only when a unit does not come
up.  What it then looks at it records, as `urt` keeps snapshots: the
pages, with the secrets as the programs see them; that no secret is in
a unit file or a unit's environment; the sockets' owners and modes, the
state directories', the databases' and the keys'; that the program's
lines reach the journal; that nothing the sandbox takes away was
needed; and `systemd-analyze security` for the services and for nginx
beside them.  The record is compared with `expected/vm`:

    nix-build tests/deploy/test.nix     # fails with the diff
    tests/deploy/update.sh --vm         # runs the machine and rewrites expected/vm

With KVM a run takes about a minute; without, emulated, some minutes.

What needs no virtual machine is checked against the files under
`expected/`, in seconds:

    nix-build tests/deploy/eval.nix     # fails with the diff
    tests/deploy/update.sh              # rewrites expected/, but for expected/vm

`expected/units/` holds what the module makes of several
configurations: the units, the preStart, `<name>-db`, the users,
nginx's upstreams and locations, and the assertions that fail.
`expected/prestart` is the preStart of a SQLite application run on a
fresh directory and on what can go wrong there (`prestart/` holds its
schemas).  `expected/build-app/` holds the phases and `passthru.urweb`
of `buildApp` for several arguments, and `expected/project-files` the
files `nix/project-files.nix` finds for `.urp` files of this tree.
Store hashes are masked.
