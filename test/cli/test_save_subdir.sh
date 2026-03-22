#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

# -------------------------------------------------------------------------
# Set up: a/b/file committed at baseline
RUN mkdir -p a/b
RUN "echo base >a/b/file"
RUN git add a/b/file
RUN git commit -m "\"initial\""

# -------------------------------------------------------------------------
# Save from repo root using full relative path a/b/file

RUN "echo 'from root' >a/b/file"
RUN_IN "$GIT_WIP" save "\"from root\"" -- a/b/file

RUN_IN "$GIT_WIP" status -l -f
EXP_grep "1 wip commit"
EXP_grep "from root"
EXP_grep "a/b/file"

RUN git show wip/master:a/b/file
EXP_grep "^from root$"

# -------------------------------------------------------------------------
# Save from subdirectory a/ using path b/file

CD a
RUN_IN "echo 'from a' >b/file"
RUN_IN "$GIT_WIP" save "\"from a\"" -- b/file

CD_ROOT
RUN_IN "$GIT_WIP" status -l -f
EXP_grep "2 wip commits"
EXP_grep "from a"
EXP_grep "from root"
EXP_grep "a/b/file"

RUN git show wip/master:a/b/file
EXP_grep "^from a$"

# -------------------------------------------------------------------------
# Save from subdirectory a/b/ using bare filename file

CD a/b
RUN_IN "echo 'from b' >file"
RUN_IN "$GIT_WIP" save "\"from b\"" -- file

CD_ROOT
RUN_IN "$GIT_WIP" status -l -f
EXP_grep "3 wip commits"
EXP_grep "from b"
EXP_grep "from a"
EXP_grep "from root"
EXP_grep "a/b/file"

RUN git show wip/master:a/b/file
EXP_grep "^from b$"

# -------------------------------------------------------------------------
# Verify --editor mode from a subdirectory (no changes → silent exit 0)

CD a/b
RUN_IN "$GIT_WIP" save --editor "\"no-change-editor\"" -- file
EXP_none

# -------------------------------------------------------------------------
# Verify that saving from subdir does NOT corrupt the real index

CD_ROOT
RUN git status --short
# a/b/file should show as modified (M) in the working tree, not staged
EXP_grep " M a/b/file"

echo "OK: $TEST_NAME"
