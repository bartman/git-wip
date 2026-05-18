#!/usr/bin/env bash
# GitVersion.sh - Generate version header from VERSION file + git metadata.
#
# Usage: GitVersion.sh PREFIX OUTPUT
#
# Output version string format:
#   {VERSION}-{YYYYMMDD}-g{HASH}[-dirty]    when .git is available
#   {VERSION}-unknown                       when .git is not available
#
# VERSION is read from the VERSION file at the top of the source tree.
# YYYYMMDD is the committer date of HEAD.
# HASH is the short hash of HEAD.
# -dirty is appended if the working tree has uncommitted changes.

set -e

PREFIX="$1"
OUTPUT="$2"

if [ -z "$PREFIX" ] || [ -z "$OUTPUT" ]; then
    echo "Usage: $0 PREFIX OUTPUT" >&2
    exit 1
fi

# Locate the source tree (the directory containing the VERSION file).  This
# script normally runs with CWD = source root (set by CMake), but be defensive.
SRC_DIR="${SRC_DIR:-$PWD}"
if [ ! -f "$SRC_DIR/VERSION" ]; then
    SRC_DIR="$(cd "$(dirname "$0")/.." && pwd)"
fi

VERSION="$(cat "$SRC_DIR/VERSION" 2>/dev/null || echo unknown)"

if git -C "$SRC_DIR" rev-parse --git-dir >/dev/null 2>&1; then
    DATE="$(git -C "$SRC_DIR" log -1 --format=%cd --date=format:%Y%m%d)"
    HASH="$(git -C "$SRC_DIR" rev-parse --short HEAD)"
    DIRTY=""
    if ! git -C "$SRC_DIR" diff --quiet 2>/dev/null \
        || ! git -C "$SRC_DIR" diff --cached --quiet 2>/dev/null; then
        DIRTY="-dirty"
    fi
    DESCRIBE="${VERSION}-${DATE}-g${HASH}${DIRTY}"
else
    DESCRIBE="${VERSION}-unknown"
fi

# Generate temporary output file
OUTPUT_TMP="${OUTPUT}.tmp"

cat > "$OUTPUT_TMP" << EOF
#pragma once
#define ${PREFIX}VERSION "${DESCRIBE}"
EOF

# Only update the file if it changed
if [ -f "$OUTPUT" ] && cmp -s "$OUTPUT" "$OUTPUT_TMP"; then
    rm -f "$OUTPUT_TMP"
else
    mv "$OUTPUT_TMP" "$OUTPUT"
fi
