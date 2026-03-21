#!/bin/bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN touch file
RUN git add file
RUN git commit -m initial

# save one wip commit
RUN "echo one >file"
RUN "$GIT_WIP" save "\"one\""

# should see "one"
RUN "$GIT_WIP" status -l
EXP_grep "branch master has 1 wip commit on refs/wip/master"
EXP_grep " - one ("

# advance the work branch past the wip
RUN git add file
RUN git commit -m one

# wip branch is now behind (or at) the work branch — no wip entries visible
RUN "$GIT_WIP" status -l
EXP_grep "branch master has 0 wip commits on refs/wip/master"

# save two more wip commits on the new work branch HEAD
RUN "echo two >file"
RUN "$GIT_WIP" save "\"two\""

# should see only "two"
RUN "$GIT_WIP" status -l
EXP_grep "branch master has 1 wip commit on refs/wip/master"
EXP_grep " - two ("
EXP_grep -v " - one ("

RUN "echo three >file"
RUN "$GIT_WIP" save "\"three\""

# should see "two" and "three", not "one"
RUN "$GIT_WIP" status -l
EXP_grep "branch master has 2 wip commits on refs/wip/master"
EXP_grep " - two ("
EXP_grep " - three ("
EXP_grep -v " - one ("

echo "OK: $TEST_NAME"
