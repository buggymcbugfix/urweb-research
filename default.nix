{
  pkgs ? import ./nixpkgs.nix,
  withUrt ? false, # `nix-build --arg withUrt true` to include Urt
}:
# The package and its checks. `nix-build` builds both, and fails if a check
# does; `nix-build -A urweb` builds the package alone, `nix-build -A check`
# the checks.
let
  urweb = pkgs.callPackage ./derivation.nix { inherit withUrt; };
in
{
  inherit urweb;
  check = urweb.tests.check;
}
