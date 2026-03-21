#!/bin/bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN "echo base >file_a"
RUN "echo base >file_b"
RUN "echo base >file_c"
RUN git add file_a file_b file_c
RUN git commit -m initial

# -------------------------------------------------------------------------
# modify two files, save only one — only the specified file appears in WIP

RUN "echo mod_a >file_a"
RUN "echo mod_b >file_b"

RUN "$GIT_WIP" save "\"save-a-only\"" -- file_a
RUN git show wip/master --stat

# file_a changed, file_b must not appear
EXP_grep "file_a"
EXP_grep -v "file_b"
EXP_grep -v "file_c"

# verify content in wip tree
RUN git show wip/master:file_a
EXP_grep "^mod_a$"

RUN git show wip/master:file_b
EXP_grep "^base$"   # unchanged in wip tree

# -------------------------------------------------------------------------
# save the other modified file — stacks on previous WIP commit

RUN "$GIT_WIP" save "\"save-b-only\"" -- file_b
RUN git show wip/master --stat

EXP_grep "file_b"
EXP_grep -v "file_a"   # file_a was already at mod_a in the parent; no delta
EXP_grep -v "file_c"

RUN git show wip/master:file_a
EXP_grep "^mod_a$"   # persisted from previous wip commit

RUN git show wip/master:file_b
EXP_grep "^mod_b$"

# -------------------------------------------------------------------------
# save multiple files at once with --

RUN "echo mod2_a >file_a"
RUN "echo mod2_c >file_c"

RUN "$GIT_WIP" save "\"save-a-and-c\"" -- file_a file_c
RUN git show wip/master --stat

EXP_grep "file_a"
EXP_grep "file_c"
EXP_grep -v "file_b"

RUN git show wip/master:file_a
EXP_grep "^mod2_a$"

RUN git show wip/master:file_c
EXP_grep "^mod2_c$"

# -------------------------------------------------------------------------
# untracked file saved explicitly with --

RUN "echo untracked >file_d"

RUN "$GIT_WIP" save "\"save-untracked-file\"" -- file_d
RUN git show wip/master --stat

EXP_grep "file_d"
EXP_grep -v "file_a"
EXP_grep -v "file_b"
EXP_grep -v "file_c"

RUN git show wip/master:file_d
EXP_grep "^untracked$"

# -------------------------------------------------------------------------
# saving an unchanged (already-at-baseline) file reports "no changes"

_RUN "$GIT_WIP" save "\"save-c-nochange\"" -- file_c
EXP_text "no changes"

# -------------------------------------------------------------------------
# --editor mode with unchanged file is silent and exits 0

RUN "$GIT_WIP" save --editor "\"save-c-editor\"" -- file_c
EXP_none

# -------------------------------------------------------------------------
# --editor mode with a changed file succeeds silently

RUN "echo editor_change >file_c"
RUN "$GIT_WIP" save --editor "\"save-c-via-editor\"" -- file_c
EXP_none

RUN git show wip/master:file_c
EXP_grep "^editor_change$"

# -------------------------------------------------------------------------
# after a real commit, a new -- file save starts a fresh WIP stack

RUN git add file_a file_b file_c file_d
RUN git commit -m "\"commit all changes\""

RUN "echo post_commit >file_a"
RUN "$GIT_WIP" save "\"post-commit-a\"" -- file_a

# new wip commit is rooted at the new work HEAD — only file_a in delta
RUN git show wip/master --stat
EXP_grep "file_a"
EXP_grep -v "file_b"
EXP_grep -v "file_c"
EXP_grep -v "file_d"

# status shows exactly 1 wip commit
RUN "$GIT_WIP" status -l
EXP_grep "branch master has 1 wip commit on refs/wip/master"
EXP_grep "post-commit-a"
EXP_grep -v "save-a-only"

echo "OK: $TEST_NAME"
