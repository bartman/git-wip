#!/bin/bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN "echo 1 >\"s p a c e s\""
RUN git add "\"s p a c e s\""
RUN git commit -m "\"s p a c e s\""

# make changes, store wip
RUN "echo 2 >\"s p a c e s\""
RUN "$GIT_WIP" save "\"message with spaces\""
EXP_none

# expecting a wip ref
RUN git for-each-ref
EXP_grep "commit.refs/wip/master$"

# expecting a log entry
RUN "$GIT_WIP" log
EXP_grep "^commit "
EXP_grep "^\s\+message with spaces$"

echo "OK: $TEST_NAME"
