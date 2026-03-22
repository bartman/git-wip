#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

# these tests are here to make sure we behave the same way as the legacy git-wip shell implementation
# do not add anymore tests here.  Create new tests in test/cli/test_*.sh instead.

create_test_repo
RUN "echo 1 >README"
RUN git add README
RUN git commit -m README

# run wip w/o changes
_RUN "$GIT_WIP" save
EXP_text "no changes"

RUN "$GIT_WIP" save --editor
EXP_none

RUN "$GIT_WIP" save -e
EXP_none

# expecting a master branch
RUN git branch
EXP_grep "^\* master$"
EXP_grep -v "wip"

# not expecting a wip ref at this time
RUN git for-each-ref
EXP_grep -v "commit.refs/wip/master$"

# make changes, store wip
RUN "echo 2 >README"
RUN "$GIT_WIP" save --editor
EXP_none

# expecting a wip ref
RUN git for-each-ref
EXP_grep "commit.refs/wip/master$"

# expecting a log entry
RUN "$GIT_WIP" log
EXP_grep "^commit "
EXP_grep "^\s\+WIP$"

# there should be no wip branch
RUN git branch
EXP_grep -v "wip"

# make changes, store wip
RUN "echo 3 >README"
RUN "$GIT_WIP" save "\"message2\""
EXP_none

# expecting both log entries
RUN "$GIT_WIP" log
EXP_grep "^commit "
EXP_grep "^\s\+WIP$"
EXP_grep "^\s\+message2$"

# make a commit
RUN git add -u README
RUN git commit -m README.2

# make changes, store wip
RUN "echo 4 >UNTRACKED"
RUN "echo 4 >README"
RUN "$GIT_WIP" save "\"message3\""
EXP_none

# expecting message3, not message2 or original WIP
RUN "$GIT_WIP" log
EXP_grep "^commit "
EXP_grep -v "^\s\+WIP$"
EXP_grep -v "^\s\+message2$"
EXP_grep "^\s\+message3$"

# expecting file changes to README, not UNTRACKED
RUN "$GIT_WIP" log --stat
EXP_grep "^commit "
EXP_grep "^ README | 2"
EXP_grep -v "UNTRACKED"

# need to be able to extract latest data from git wip branch
RUN git show HEAD:README
EXP_grep '^3$'
EXP_grep -v '^4$'

RUN git show wip/master:README
EXP_grep -v '^3$'
EXP_grep '^4$'

echo "OK: $TEST_NAME"
