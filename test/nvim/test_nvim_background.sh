#!/usr/bin/env bash
# test_nvim_background.sh -- Test git-wip with background=true (async execution)
source "$(dirname "$0")/lib.sh"

# -------------------------------------------------------------------------
# Setup: create a test repo with one file

create_test_repo

# Create initial file and commit
echo "initial content" > file.txt
git add file.txt
git commit -m "initial commit"

note "Repo created at $REPO"

# Set background=true for async execution
export GIT_WIP_TEST_BACKGROUND=true

# -------------------------------------------------------------------------
# Test 1: Single save (one edit + :w)

note "Test 1: Single save with background=true"

# Make a change and save using nvim
run_nvim "edit file.txt" "normal osecond line" "write"

# Verify WIP commit was created
run git for-each-ref | grep -q "refs/wip/master"
note "WIP ref exists after single save"

# Verify the change is in the WIP tree
run git show wip/master:file.txt | grep -q "second line"
note "Change captured in WIP tree"

# Verify exactly 1 WIP commit
"$GIT_WIP" status | grep -q "branch master has 1 wip commit"
note "Exactly 1 WIP commit"

# -------------------------------------------------------------------------
# Test 2: Two saves (two edits)

note "Test 2: Two saves with background=true"

# Make another change and save
run_nvim "edit file.txt" "normal othird line" "write"

# Verify exactly 2 WIP commits
"$GIT_WIP" status | grep -q "branch master has 2 wip commit"
note "Exactly 2 WIP commits"

# Verify latest WIP has third line
run git show wip/master:file.txt | grep -q "third line"
note "Third line in WIP tree"

# Verify first WIP still has second line but not third
run git show wip/master~1:file.txt | grep -q "second line"
note "First WIP still has second line"

echo "OK: $TEST_NAME"