#!/usr/bin/env bash
# test_nvim_single.sh -- Test git-wip with single file saves
source "$(dirname "$0")/lib.sh"

# -------------------------------------------------------------------------
# Setup: create a test repo with one file

create_test_repo

# Create initial file and commit
echo "initial content" > file.txt
git add file.txt
git commit -m "initial commit"

note "Repo created at $REPO"

# -------------------------------------------------------------------------
# Test 1: Single save (one edit + :w)

note "Test 1: Single save"

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

note "Test 2: Two saves"

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

# -------------------------------------------------------------------------
# Test 3: Three saves

note "Test 3: Three saves"

# Make another change and save
run_nvim "edit file.txt" "normal ofourth line" "write"

# Verify exactly 3 WIP commits
"$GIT_WIP" status | grep -q "branch master has 3 wip commit"
note "Exactly 3 WIP commits"

# Verify log shows 3 commits (count lines starting with *)
count=$("$GIT_WIP" log --pretty | grep -c "^[*\>]") || count=0
if [ "$count" -eq 3 ]; then
    note "Log shows 3 WIP commits"
else
    die "Expected 3 WIP commits in log, got $count"
fi

# -------------------------------------------------------------------------
# Test 4: Save with no changes (should be silent in --editor mode)

note "Test 4: Save with no changes"

# Save the same content again
_run_nvim "edit file.txt" "write"

# Should still be 3 WIP commits (no new one created)
"$GIT_WIP" status | grep -q "branch master has 3 wip commit"
note "No new WIP commit for unchanged file"

echo "OK: $TEST_NAME"
