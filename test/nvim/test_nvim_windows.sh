#!/usr/bin/env bash
# test_nvim_windows.sh -- Test git-wip with multiple windows
# Tests window-related operations in Neovim
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
# Test 1: Edit file in one window (single window)

note "Test 1: Single window edit"

run_nvim "edit file_a.txt" "normal oline 2 a" "write"

# Verify WIP commit was created
run git for-each-ref | grep -q "refs/wip/master"
note "WIP ref exists"

# Verify the change is in the WIP tree
run git show wip/master:file_a.txt | grep -q "line 2 a"
note "Change captured in WIP tree"

# -------------------------------------------------------------------------
# Test 2: Multiple saves (simulates multiple window edits over time)

note "Test 2: Multiple saves"

run_nvim "edit file_a.txt" "normal oline 3 a" "write"
run_nvim "edit file_b.txt" "normal oline 2 b" "write"

# Verify we have 3 WIP commits now
"$GIT_WIP" status | grep -q "branch master has 3 wip commit"
note "3 WIP commits"

# -------------------------------------------------------------------------
# Test 3: Buffer in window, then switch buffers

note "Test 3: Buffer switching"

run_nvim "edit file_a.txt" "badd file_b.txt" "bnext" "normal oline 3 b" "write"

# Verify we have 4 WIP commits now
"$GIT_WIP" status | grep -q "branch master has 4 wip commit"
note "4 WIP commits after buffer switch"

# -------------------------------------------------------------------------
# Test 4: Edit multiple files in sequence (simulates working in multiple windows)

note "Test 4: Multiple file edits"

run_nvim "edit file_a.txt" "normal oline 4 a" "write"
run_nvim "edit file_b.txt" "normal oline 4 b" "write"

# Verify we have 6 WIP commits now
"$GIT_WIP" status | grep -q "branch master has 6 wip commit"
note "6 WIP commits"

# Verify both files have changes
run git show wip/master:file_a.txt | grep -q "line 4 a"
run git show wip/master:file_b.txt | grep -q "line 4 b"
note "Both files have changes in WIP tree"

echo "OK: $TEST_NAME"
