# nix-shell --pure --run 'make check'
{
  pkgs ? import ./nixpkgs.nix,
}:
pkgs.mkShell {
  packages = [ pkgs.tlaplus ];
}
