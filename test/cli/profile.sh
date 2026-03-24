#!/usr/bin/env bash
# profile.sh -- performance profiling test for git-wip save
# Run manually: GIT_WIP=/path/to/git-wip TEST_TREE=/tmp/test-tree ./profile.sh
#
# This test builds a large git repository and measures time for different
# git-wip save scenarios to identify performance bottlenecks.

if [ -z "$GIT_WIP" ] && [ -z "$TEST_TREE" ] ; then
    echo "# GIT_WIP and TEST_TREE not specified, using defaults"
    GIT_WIP=$(pwd)/build/src/git-wip
    TEST_TREE=/tmp/wip-profile
fi

source "$(dirname "$0")/lib.sh"

# -------------------------------------------------------------------------
# Helper: time a command and print its duration

time_cmd() {
    local label="$1"
    shift

    note "=== $label ==="
    local start end duration
    start=$(date +%s%3N)
    eval "$@"
    end=$(date +%s%3N)
    duration=$((end - start))
    printf '%d ms\n' "$duration"
    note ""
}

# -------------------------------------------------------------------------
# Create the test repository with nested directories

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

note "Creating nested directory structure (2560 files)..."
cd "$REPO"

# Create directories {a,b,c,d}/{a,b,c,d}/{a,b,c,d}/{a,b,c,d}
for d1 in a b c d; do
    for d2 in a b c d; do
        for d3 in a b c d; do
            for d4 in a b c d; do
                dir="$REPO/$d1/$d2/$d3/$d4"
                mkdir -p "$dir"
                # Create 10 files in each directory
                for f in 0 1 2 3 4 5 6 7 8 9; do
                    echo "content" > "$dir/$f"
                done
                # Commit each directory separately
                git add -A "$dir"/ >/dev/null
                git commit -m "\"$dir\"" >/dev/null
            done
        done
    done
done

note "Initial commit structure ready:"
RUN git log --oneline
RUN git rev-list --count HEAD
RUN git ls-files
lines="$(wc -l <"$OUT")"
if ! [ "$lines" = 2560 ] ; then
    die "expecting 2560 files but found $files, check $OUT"
fi

# -------------------------------------------------------------------------
# Scenario 1: wip save on all files (all changed)

note "=== SCENARIO 1: wip save on all files (all changed) ==="
cd "$REPO"
for f in $(find . -path ./.git -prune -o -type f -print); do
    echo "test1" > "$f"
done
sync

time_cmd "git-wip save (all files changed)" "$GIT_WIP" save "\"WIP-1\""

# -------------------------------------------------------------------------
# Scenario 2: wip save on one file, all changed (but only save one path)

note "=== SCENARIO 2: wip save on one path, all files changed ==="
git reset --hard
for f in $(find . -path ./.git -prune -o -type f -print); do
    echo "test2" > "$f"
done
sync

time_cmd "git-wip save -- a/b/c/d/0 (one path)" "$GIT_WIP" save "\"WIP-2\"" -- a/b/c/d/0

# -------------------------------------------------------------------------
# Scenario 3: wip save on one file, one changed

note "=== SCENARIO 3: wip save on one file, one changed ==="
git reset --hard
echo "test3" > a/b/c/d/0
sync

time_cmd "git-wip save -- a/b/c/d/0 (single file)" "$GIT_WIP" save "\"WIP-3\"" -- a/b/c/d/0

# -------------------------------------------------------------------------
# Summary

note "=== PROFILING COMPLETE ==="
note "Repository: $REPO"
note "Files: 2560 in 256 directories"
note ""
note "Run 'du -sh $REPO/.git' to see git object database size"

echo "OK: $TEST_NAME"
