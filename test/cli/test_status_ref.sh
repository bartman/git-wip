#!/bin/bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN "echo v1 >file.txt"
RUN git add file.txt
RUN git commit -m initial

# master: two wip commits
RUN "echo m2 >file.txt"
RUN "$GIT_WIP" save "\"master one\""
RUN "echo m3 >file.txt"
RUN "$GIT_WIP" save "\"master two\""

# foo: one wip commit
RUN git checkout -b foo
RUN "echo f1 >foo.txt"
RUN git add foo.txt
RUN git commit -m "\"foo base\""
RUN "echo f2 >>foo.txt"
RUN "$GIT_WIP" save "\"foo one\""

# Status on master from foo branch, across all accepted ref syntaxes
RUN "$GIT_WIP" status master
EXP_grep "branch master has 2 wip commits on refs/wip/master"

RUN "$GIT_WIP" status wip/master
EXP_grep "branch master has 2 wip commits on refs/wip/master"

RUN "$GIT_WIP" status refs/heads/master
EXP_grep "branch master has 2 wip commits on refs/wip/master"

RUN "$GIT_WIP" status refs/wip/master
EXP_grep "branch master has 2 wip commits on refs/wip/master"

# Status on foo across all accepted ref syntaxes
RUN "$GIT_WIP" status foo
EXP_grep "branch foo has 1 wip commit on refs/wip/foo"

RUN "$GIT_WIP" status wip/foo
EXP_grep "branch foo has 1 wip commit on refs/wip/foo"

RUN "$GIT_WIP" status refs/heads/foo
EXP_grep "branch foo has 1 wip commit on refs/wip/foo"

RUN "$GIT_WIP" status refs/wip/foo
EXP_grep "branch foo has 1 wip commit on refs/wip/foo"

echo "OK: $TEST_NAME"
