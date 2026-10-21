#!/usr/bin/env bash
# Rewrites expected/ from what eval.nix renders now.  expected/vm, the
# machine's record, is kept; with --vm it is rewritten from a run of
# test.nix's machine, which takes minutes.
set -euo pipefail
cd "$(dirname "$0")"
out=$(nix-build --no-out-link eval.nix -A rendered)
if [ "${1:-}" = --vm ]; then
    vm=$(nix-build --no-out-link test.nix -A vm)/observed
elif [ -e expected/vm ]; then
    vm=$(mktemp)
    cp expected/vm "$vm"
else
    vm=
fi
rm -rf expected
cp -r "$out" expected
chmod -R u+w expected
if [ -n "$vm" ]; then
    cp "$vm" expected/vm
fi
