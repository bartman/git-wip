#!/usr/bin/env bash
# tag-new-release.sh - Tag a new release
#
# Usage: tag-new-release.sh <version>
#
#   1. Validates that <version> is strictly greater (per `sort -V`) than both
#      the current contents of the VERSION file and the most recent annotated
#      git tag.
#   2. Writes <version> to the VERSION file.
#   3. Commits the VERSION file with message "Release <version>".
#   4. Creates an annotated tag `git tag -a -m <version> <version>`.

set -euo pipefail

die() { echo "error: $*" >&2; exit 1; }

[ $# -eq 1 ] || die "usage: $0 <version>"
NEW="$1"

# Run from repo root
cd "$(dirname "$0")"

[ -d .git ] || die "not at the top of a git repo"
[ -f VERSION ] || die "VERSION file missing"

CUR_FILE="$(cat VERSION)"
CUR_TAG="$(git tag --list --sort=-v:refname | head -n1 || true)"

# Strict-greater check via `sort -V`: NEW must sort strictly after the other.
strictly_greater() {
    local new="$1" other="$2"
    [ -n "$other" ] || return 0          # nothing to compare against
    [ "$new" != "$other" ] || return 1
    local top
    top="$(printf '%s\n%s\n' "$new" "$other" | sort -V | tail -n1)"
    [ "$top" = "$new" ]
}

strictly_greater "$NEW" "$CUR_FILE" \
    || die "version '$NEW' is not strictly greater than VERSION file '$CUR_FILE'"
strictly_greater "$NEW" "$CUR_TAG" \
    || die "version '$NEW' is not strictly greater than most recent tag '$CUR_TAG'"

# Refuse to release with a dirty tree (besides VERSION itself, which we are
# about to write).
if ! git diff --quiet || ! git diff --cached --quiet; then
    die "working tree has uncommitted changes; commit or stash them first"
fi

echo "$NEW" > VERSION
git add VERSION
git commit -m "Release $NEW"
git tag -a -m "$NEW" "$NEW"

echo "Tagged release $NEW (commit $(git rev-parse --short HEAD))"
