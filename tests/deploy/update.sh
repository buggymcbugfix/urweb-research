#!/usr/bin/env bash
# Rewrites expected/ from what eval.nix renders now.
set -euo pipefail
cd "$(dirname "$0")"
out=$(nix-build --no-out-link eval.nix -A rendered)
rm -rf expected
cp -r "$out" expected
chmod -R u+w expected
