What `urweb.buildApp` and `nix/project-files.nix` make of their
arguments is checked, without a virtual machine, against the files under
`expected/`, in seconds:

    nix-build tests/deploy/eval.nix     # fails with the diff
    tests/deploy/update.sh              # rewrites expected/

`expected/build-app/` holds the phases and `passthru.urweb` of
`buildApp` for several arguments, and `expected/project-files` the files
`nix/project-files.nix` finds for `.urp` files of this tree.  Store
hashes are masked.

`app` is a small application: a page that counts the rows of a table
and shows three variables of its environment, and one that inserts a row.
`app.nix` is its package, a `buildApp` call.
