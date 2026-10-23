# `nix-prefetch-url --unpack <url>`.
import (fetchTarball {
  url = "https://github.com/NixOS/nixpkgs/archive/09063bd1f2b36fd9fd793c36dfade7bd40a93dfc.tar.gz";
  sha256 = "sha256-Mh0PUaIv4nFY+8wAGonaEh8iBzwsmdNlwjVHmFg1Biw=";
}) { }
