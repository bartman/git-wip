#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN "echo v1 >file.txt"
RUN git add file.txt
RUN git commit -m initial

# master wip
RUN "echo m2 >file.txt"
RUN "$GIT_WIP" save "\"master one\""

# foo wip
RUN git checkout -b foo
RUN "echo f1 >foo.txt"
RUN git add foo.txt
RUN git commit -m "\"foo base\""
RUN "echo f2 >>foo.txt"
RUN "$GIT_WIP" save "\"foo one\""

# orphaned wip ref
RUN git update-ref refs/wip/orphan HEAD

# cleanup removes only orphaned refs
RUN "$GIT_WIP" delete --cleanup
EXP_grep "deleted wip/orphan"
EXP_grep "deleted 1 orphaned wip ref"

RUN git for-each-ref
EXP_grep "refs/wip/master$"
EXP_grep "refs/wip/foo$"
EXP_grep -v "refs/wip/orphan$"

# delete a single wip ref by explicit ref name
RUN "$GIT_WIP" delete refs/wip/foo
EXP_grep "deleted wip/foo"
RUN git for-each-ref
EXP_grep -v "refs/wip/foo$"
EXP_grep "refs/wip/master$"

# delete current branch wip with confirmation: 'n' aborts
RUN git checkout -f master
_RUN "printf 'n\n' | \"$GIT_WIP\" delete"
EXP_grep "About to delete wip/master \[Y/n\]"
RUN git for-each-ref
EXP_grep "refs/wip/master$"

# empty response confirms delete
RUN "printf '\n' | \"$GIT_WIP\" delete"
EXP_grep "deleted wip/master"
RUN git for-each-ref
EXP_grep -v "refs/wip/master$"

# recreate and delete with --yes (no prompt)
RUN "echo m3 >file.txt"
RUN "$GIT_WIP" save "\"master two\""
RUN "$GIT_WIP" delete --yes
EXP_grep "deleted wip/master"
EXP_grep -v "About to delete"
RUN git for-each-ref
EXP_grep -v "refs/wip/master$"

echo "OK: $TEST_NAME"
