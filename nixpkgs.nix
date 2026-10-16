import (builtins.fetchTarball {
  url = https://github.com/NixOS/nixpkgs/archive/nixos-unstable.tar.gz;
  # Hash obtained using `nix-prefetch-url --unpack <url>`
  sha256 = "sha256:8S3Kcxs7D4UtxJxSJZz0m14CGhuW0MxfrIwJxeGWGnQ=";
}) { }
