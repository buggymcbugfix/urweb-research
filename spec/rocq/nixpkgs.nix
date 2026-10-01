# nixos-26.05, as resolved by https://channels.nixos.org/nixos-26.05/nixexprs.tar.xz
# (`rocqPackages` there is Rocq 9.1.1 with rocq-stdlib 9.0.0). Pinned apart
# from ../../nixpkgs.nix so that the proofs are checked against one known
# Rocq, whatever the compiler's nixpkgs moves to.
import (fetchTarball {
  url = "https://releases.nixos.org/nixos/26.05/nixos-26.05.10927.78e9c786dc08/nixexprs.tar.xz";
  # nix-prefetch-url --unpack --type sha256 <url>
  sha256 = "0mn78igii7jbp35k8khfsdi6zbcylawc2fwwl9ff7l210mav1s4x";
}) { }
