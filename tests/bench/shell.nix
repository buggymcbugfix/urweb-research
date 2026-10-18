# The compiler's shell (../../shell.nix) with what the benchmark runs
# beside the application: nginx, wrk, python3 with psutil for the
# processes' CPU times, and on Linux taskset for pinning. spawn-fcgi comes
# with the compiler's shell, which has it for the tests, as the patched
# package derivation.nix makes of it.
#
#   nix-shell tests/bench/shell.nix --pure --run 'python3 tests/bench/bench.py'
{ pkgs ? import ../../nixpkgs.nix }:
let
  base = import ../../shell.nix { inherit pkgs; };
  # First on the PATH, ahead of the shell's own python3.
  python = pkgs.python3.withPackages (p: [ p.psutil ]);
in
base.overrideAttrs (old: {
  nativeBuildInputs = [ python ] ++ old.nativeBuildInputs
    ++ (with pkgs; [ nginx wrk ])
    ++ pkgs.lib.optional pkgs.stdenv.isLinux pkgs.util-linux;
})
