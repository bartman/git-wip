#!/bin/bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN "echo v1 >file.txt"
RUN git add file.txt
RUN git commit -m initial

# -------------------------------------------------------------------------
# no wip branch yet — status should report zero wip commits (exit 0)

RUN "$GIT_WIP" status
EXP_grep "no wip commits"

# -------------------------------------------------------------------------
# create 3 wip commits with distinct file changes

RUN "echo v2 >file.txt"
RUN "$GIT_WIP" save "\"WIP one\""

RUN "echo v3 >file.txt"
RUN "$GIT_WIP" save "\"WIP two\""

RUN "echo v4 >file.txt"
RUN "$GIT_WIP" save "\"WIP three\""

# -------------------------------------------------------------------------
# status (no flags) — summary line only

RUN "$GIT_WIP" status
EXP_grep "branch master has 3 wip commits on refs/wip/master"

# -------------------------------------------------------------------------
# status -l — summary + one line per commit (newest first)

RUN "$GIT_WIP" status -l
EXP_grep "branch master has 3 wip commits on refs/wip/master"
# each commit line: <sha7> - <subject> (<age>)
EXP_grep " - WIP three ("
EXP_grep " - WIP two ("
EXP_grep " - WIP one ("
# order: newest first — line number of "WIP three" must be less than "WIP one"
three_line=$(grep -n "WIP three" "$OUT" | cut -d: -f1)
one_line=$(grep -n "WIP one" "$OUT" | cut -d: -f1)
[ "$three_line" -lt "$one_line" ] || { warn "WIP three not before WIP one in output"; handle_error; }

# -------------------------------------------------------------------------
# status --list is synonymous with -l

RUN "$GIT_WIP" status --list
EXP_grep "branch master has 3 wip commits on refs/wip/master"
EXP_grep " - WIP three ("

# -------------------------------------------------------------------------
# status -f — summary + diff --stat from HEAD to latest wip

RUN "$GIT_WIP" status -f
EXP_grep "branch master has 3 wip commits on refs/wip/master"
# git diff --stat output includes the filename and change counts
EXP_grep "file.txt"
EXP_grep "changed"

# -------------------------------------------------------------------------
# status --files is synonymous with -f

RUN "$GIT_WIP" status --files
EXP_grep "branch master has 3 wip commits on refs/wip/master"
EXP_grep "file.txt"

# -------------------------------------------------------------------------
# status -l -f — per-commit diff --stat interleaved with list lines

RUN "$GIT_WIP" status -l -f
EXP_grep "branch master has 3 wip commits on refs/wip/master"
EXP_grep " - WIP three ("
EXP_grep " - WIP two ("
EXP_grep " - WIP one ("
# each commit's diff --stat should show file.txt changed
EXP_grep "file.txt"
EXP_grep "changed"
# summary comes before any commit lines
summary_line=$(grep -n "3 wip commits" "$OUT" | cut -d: -f1)
first_commit_line=$(grep -n "WIP three" "$OUT" | cut -d: -f1)
[ "$summary_line" -lt "$first_commit_line" ] || { warn "summary not before commit lines"; handle_error; }

echo "OK: $TEST_NAME"
