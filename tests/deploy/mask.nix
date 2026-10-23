# How the records under expected/ hide what changes without being a change
# of theirs: a store path's hash, which changes with every build of what is
# behind it, and the versions in store names, which change with nixpkgs.
# One definition for eval.nix, which masks at evaluation and in the scripts
# that make its records, and for test.nix, which masks what the machine
# shows.
rec {
  # A store path's hash and the dash after it.  A POSIX extended regular
  # expression, which Nix's builtins.split, sed -E and Python's re read
  # alike.
  hash = "/nix/store/[a-z0-9]{32}-";
  # What it becomes.
  masked = "/nix/store/HASH-";
  # A sed -E script that masks the hashes and drops the versions from the
  # names behind them, so that a bump of nixpkgs leaves the records alone:
  # each segment of such a name that starts with a digit goes, up to the
  # next dash (glibc-locales-2.42-67 is glibc-locales, util-linux-2.42.2-bin
  # is util-linux-bin, app-0 is app).
  unversion = "s|${hash}|${masked}|g; :a; s|(${masked}[^/ ]*)-[0-9][^-/ ]*|\\1|; ta";
}
