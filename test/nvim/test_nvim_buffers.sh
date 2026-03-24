#!/usr/bin/env bash
# test_nvim_buffers.sh -- Test git-wip with multiple buffers
source "$(dirname "$0")/lib.sh"

# -------------------------------------------------------------------------
# Setup: create a test repo with multiple files

create_test_repo

# Create initial files and commit
echo "initial content A" > file_a.txt
echo "initial content B" > file_b.txt
git add file_a.txt file_b.txt
git commit -m "initial commit"

note "Repo created at $REPO"

# -------------------------------------------------------------------------
# Test 1: Edit both files and save each

note "Test 1: Edit and save both files"

# Edit first file
run_nvim "edit file_a.txt" "normal oline 2 a" "write"

# Verify file_a was saved to WIP
run git show wip/master:file_a.txt | grep -q "line 2 a"
note "file_a changes captured in WIP"

# Edit second file (new WIP commit)
run_nvim "edit file_b.txt" "normal oline 2 b" "write"

# Verify file_b was saved to WIP
run git show wip/master:file_b.txt | grep -q "line 2 b"
note "file_b changes captured in WIP"

# Verify we have 2 WIP commits (one for each save)
"$GIT_WIP" status | grep -q "branch master has 2 wip commit"
note "2 WIP commits for 2 saves"

# -------------------------------------------------------------------------
# Test 2: Use :bufnext to switch between buffers

note "Test 2: Switch between buffers with :bnext"

# Edit file_a
run_nvim "edit file_a.txt" "normal oline 3 a" "write"

# Switch to next buffer
run_nvim "bnext" "edit file_b.txt" "normal oline 3 b" "write"

# Verify we now have 4 WIP commits
"$GIT_WIP" status | grep -q "branch master has 4 wip commit"
note "4 WIP commits after buffer switching"

# Verify latest changes
run git show wip/master:file_a.txt | grep -q "line 3 a"
run git show wip/master:file_b.txt | grep -q "line 3 b"
note "Latest changes in WIP tree"

# -------------------------------------------------------------------------
# Test 3: Edit both files in sequence, verify both captured

note "Test 3: Edit both files in one session"

# Edit both files and save each
run_nvim "edit file_a.txt" "normal oline 4 a" "write"

run_nvim "edit file_b.txt" "normal oline 4 b" "write"

# Verify we have 6 WIP commits now
"$GIT_WIP" status | grep -q "branch master has 6 wip commit"
note "6 WIP commits total"

# -------------------------------------------------------------------------
# Test 4: Untracked file

note "Test 4: Save untracked file"

# Create an untracked file
echo "untracked content" > file_c.txt

# The plugin by default doesn't capture untracked files, so we need to verify
# the file remains untracked in WIP tree
run_nvim "edit file_c.txt" "normal ountracked line 2" "write"

# File should be in WIP tree now (it's untracked but was explicitly saved)
run git show wip/master:file_c.txt | grep -q "untracked line 2"
note "Untracked file captured in WIP tree"

echo "OK: $TEST_NAME"
