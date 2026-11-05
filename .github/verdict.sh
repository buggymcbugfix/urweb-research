#!/usr/bin/env bash
#
# A job's verdict, from the exit statuses in ci-out. A build that failed
# is annotated with the end of its log, since no test ran to say so; the
# checks annotate their own failures as they run.
#
#   .github/verdict.sh

b=$(cat ci-out/build.status 2>/dev/null || echo none)
if [ "$b" != 0 ]; then
  text=$(tail -n 40 ci-out/build.log 2>/dev/null | awk '{ gsub(/%/, "%25"); gsub(/\r/, "%0D"); printf "%s%%0A", $0 }')
  echo "::error title=The build failed (status $b)::${text:-no build log}"
  exit 1
fi
c=$(cat ci-out/check.status 2>/dev/null || echo none)
if [ "$c" != 0 ]; then
  echo "The checks failed (status $c): see the annotations, and the log of the step that ran them."
  exit 1
fi
