#!/usr/bin/env bash
set -eu -o pipefail

nix-shell --pure --run 'urweb_configure && make && make update-snapshot'
