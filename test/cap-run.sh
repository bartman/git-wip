#!/usr/bin/env bash
set -euo pipefail
echo "# $0 $*"
quiet=false
[ "$1" = --quiet ] && { quiet=true ; shift ; }
out=$1 ; shift
echo "# $* > $out"
if $quiet ; then
    exec "$@" >"$out" 2>&1
else
    exec "$@" 2>&1 | tee "$out"
fi
