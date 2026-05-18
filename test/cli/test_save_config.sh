#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

# Test that git config options work for git-wip save

create_test_repo
RUN "echo 1 >README"
RUN git add README
RUN git commit -m "\"initial commit\""

# -----------------------------------------------------------------------------
# Test 1: git-wip.save = untracked
# -----------------------------------------------------------------------------
note "Test 1: git-wip.save = untracked"

RUN git config git-wip.save untracked

# Create an untracked file
RUN "echo content >UNTRACKED"

# Without the config, untracked files would NOT be captured.
# With git-wip.save=untracked, they should be captured.
RUN "$GIT_WIP" save "\"test untracked config\""

# Verify untracked file was captured
RUN git show wip/master:UNTRACKED
EXP_text "content"

# Clean up for next test
RUN git update-ref -d refs/wip/master
RUN rm UNTRACKED

# -----------------------------------------------------------------------------
# Test 2: git-wip.save = tracked (explicit default)
# -----------------------------------------------------------------------------
note "Test 2: git-wip.save = tracked"

RUN git config git-wip.save tracked

# Create untracked file
RUN "echo content >UNTRACKED2"

# Make a change to tracked file
RUN "echo 2 >README"

RUN "$GIT_WIP" save "\"test tracked config\""

# Verify tracked file was captured
RUN git show wip/master:README
EXP_text "2"

# Verify untracked file was NOT captured
_RUN git show wip/master:UNTRACKED2
EXP_grep "not in"

# Clean up
RUN git update-ref -d refs/wip/master
RUN rm UNTRACKED2
RUN "echo 1 >README"

# -----------------------------------------------------------------------------
# Test 3: Command line overrides config
# -----------------------------------------------------------------------------
note "Test 3: Command line overrides config"

# Config says tracked only
RUN git config git-wip.save tracked

# Create untracked file
RUN "echo override >OVERRIDE"
RUN "echo 3 >README"

# But command line says --untracked
RUN "$GIT_WIP" save "\"override test\"" --untracked

# Verify untracked file WAS captured (command line wins)
RUN git show wip/master:OVERRIDE
EXP_text "override"

# Clean up
RUN git update-ref -d refs/wip/master
RUN rm OVERRIDE
RUN "echo 1 >README"

# -----------------------------------------------------------------------------
# Test 4: --no-untracked overrides git-wip.save = untracked
# -----------------------------------------------------------------------------
note "Test 4: --no-untracked overrides config"

RUN git config git-wip.save untracked

# Create untracked file
RUN "echo should-not-capture >NOCAPTURE"
RUN "echo 4 >README"

# Command line explicitly disables untracked
RUN "$GIT_WIP" save "\"no-untracked test\"" --no-untracked

# Verify untracked file was NOT captured
_RUN git show wip/master:NOCAPTURE
EXP_grep "not in"

# Verify tracked file was captured
RUN git show wip/master:README
EXP_text "4"

# Clean up
RUN git update-ref -d refs/wip/master
RUN rm NOCAPTURE
RUN "echo 1 >README"

# -----------------------------------------------------------------------------
# Test 5: git-wip.save = all
# -----------------------------------------------------------------------------
note "Test 5: git-wip.save = all"

RUN git config git-wip.save all

# Create files
RUN "echo untracked >UNTRACKED_ALL"
RUN "echo 5 >README"

# Create an ignored file
RUN "echo ignored >IGNORED"
RUN "echo IGNORED >.gitignore"
RUN git add .gitignore
RUN git commit -m "\"add gitignore\""

# Save with config=all
RUN "$GIT_WIP" save "\"all test\""

# Verify both untracked and ignored were captured
RUN git show wip/master:UNTRACKED_ALL
EXP_text "untracked"

RUN git show wip/master:IGNORED
EXP_text "ignored"

# Clean up
RUN git update-ref -d refs/wip/master
RUN rm UNTRACKED_ALL IGNORED

echo "OK: $TEST_NAME"
