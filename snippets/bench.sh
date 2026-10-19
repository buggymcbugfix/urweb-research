#!/usr/bin/env bash
set -eu -o pipefail

nix-shell --pure --run 'urweb_configure && make -C src/c && make -C src/c install && make mlton'
mkdir -p out/bench
nix-shell tests/bench/shell.nix --pure --run "python3 tests/bench/bench.py --tsv out/bench/$(date +%Y-%m-%d__%H_%M_%S).tsv"
