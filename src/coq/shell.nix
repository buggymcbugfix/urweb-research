# nix-shell --pure --run make
{
  pkgs ? import ./nixpkgs.nix,
}:
pkgs.mkShell {
  packages = with pkgs.rocqPackages; [
    rocq-core
    stdlib
  ];
}
