#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN "echo v1 >file.txt"
RUN git add file.txt
RUN git commit -m initial

# one wip commit on master
RUN "echo master-wip >file.txt"
RUN "$GIT_WIP" save "\"master snapshot\""

# one wip commit on foo
RUN git checkout -b foo
RUN "echo foo-wip >file.txt"
RUN "$GIT_WIP" save "\"foo snapshot\""

# add an orphaned wip ref (no refs/heads/baz branch)
RUN git update-ref refs/wip/baz HEAD

# list all wip refs (short form)
RUN "$GIT_WIP" list
EXP_grep "^wip/master$"
EXP_grep "^wip/foo$"
EXP_grep "^wip/baz$"

# verbose output includes ahead counts and orphaned refs
RUN "$GIT_WIP" list -v
EXP_grep "^wip/master has 1 commit ahead of master$"
EXP_grep "^wip/foo has 1 commit ahead of foo$"
EXP_grep "^wip/baz is orphaned$"

echo "OK: $TEST_NAME"
