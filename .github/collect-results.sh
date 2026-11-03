#!/usr/bin/env bash
#
# Pushes what a run of the checks left behind to a branch of its own,
# ci-results/<run id>/<target>, so that it can be read with git alone:
# the logs and exit statuses in ci-out, and the files under the tests'
# out directories that the run wrote (programs and files over 2 MB left
# out).
#
#   .github/collect-results.sh TARGET
#
# Run from the root of the checkout, in a GitHub Actions job whose
# GH_TOKEN may push to the repository.

set -eu
target=$1
res=$RUNNER_TEMP/results
rm -rf "$res"
mkdir -p "$res"

[ -d ci-out ] && cp -R ci-out/. "$res"/
git rev-parse HEAD > "$res/source"
git status --porcelain > "$res/git-status" 2>&1 || true
git ls-files --others -- tests demo urt | grep '/out/' \
  | while read -r f; do [ -f "$f" ] && [ "$(wc -c < "$f")" -lt 2097152 ] \
      && case $f in *.exe) ;; *) printf '%s\n' "$f" ;; esac; done \
  | tar -cf - -T - | tar -xf - -C "$res"

cd "$res"
git init -q
git add -A
git -c user.name=ci -c user.email=ci@invalid commit -q \
  -m "results: $target, run $GITHUB_RUN_ID, source $(cat source)"
auth=$(printf 'x-access-token:%s' "$GH_TOKEN" | base64 | tr -d '\n')
git -c http.extraheader="AUTHORIZATION: basic $auth" push -q \
  "https://github.com/$GITHUB_REPOSITORY.git" \
  "HEAD:refs/heads/ci-results/$GITHUB_RUN_ID/$target"
