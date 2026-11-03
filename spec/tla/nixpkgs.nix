# nixos-26.05, as resolved by https://channels.nixos.org/nixos-26.05/nixexprs.tar.xz
# (`tlaplus` there is TLC 2.19, tla2tools 1.7.4, on a Java 8 runtime); the
# same release as ../rocq/nixpkgs.nix. Pinned apart from ../../nixpkgs.nix
# so that the model is checked with one known TLC, whatever the compiler's
# nixpkgs moves to.
import (fetchTarball {
  url = "https://releases.nixos.org/nixos/26.05/nixos-26.05.10927.78e9c786dc08/nixexprs.tar.xz";
  # nix-prefetch-url --unpack --type sha256 <url>
  sha256 = "0mn78igii7jbp35k8khfsdi6zbcylawc2fwwl9ff7l210mav1s4x";
}) { }
