#!/usr/bin/env bash
# GitVersion.sh - Generate version header from git describe
# Usage: GitVersion.sh PREFIX OUTPUT

set -e

PREFIX="$1"
OUTPUT="$2"

if [ -z "$PREFIX" ] || [ -z "$OUTPUT" ]; then
    echo "Usage: $0 PREFIX OUTPUT" >&2
    exit 1
fi

# Get git describe output
DESCRIBE="$(git describe --tags --dirty=-dirty 2>/dev/null || echo "unknown")"

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
