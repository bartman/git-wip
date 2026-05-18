#!/usr/bin/env bash
source "$(dirname "$0")/lib.sh"

create_test_repo
RUN git config user.email "test@example.com"
RUN git config user.name "Test User"

RUN "echo v1 >file.txt"
RUN git add file.txt
RUN git commit -m initial

# -------------------------------------------------------------------------
# no wip branch yet — log should report error

_RUN "$GIT_WIP" log
EXP_grep "has no WIP commits"

# -------------------------------------------------------------------------
# create 3 wip commits with distinct file changes

RUN "echo v2 >file.txt"
RUN "$GIT_WIP" save "\"WIP one\""

RUN "echo v3 >file.txt"
RUN "$GIT_WIP" save "\"WIP two\""

RUN "echo v4 >file.txt"
RUN "$GIT_WIP" save "\"WIP three\""

# -------------------------------------------------------------------------
# log (no flags) — default full log format

RUN "$GIT_WIP" log
EXP_grep "^commit "
EXP_grep "WIP three"
EXP_grep "WIP two"
EXP_grep "WIP one"

# -------------------------------------------------------------------------
# log -p / --pretty — compact oneline graph format

RUN "$GIT_WIP" log -p
EXP_grep "WIP three"
EXP_grep "WIP two"
EXP_grep "WIP one"
# pretty format includes color codes and relative time
EXP_grep "ago"

RUN "$GIT_WIP" log --pretty
EXP_grep "WIP three"
EXP_grep "ago"

# -------------------------------------------------------------------------
# log -s / --stat — shows file changes

RUN "$GIT_WIP" log -s
EXP_grep "^commit "
EXP_grep "WIP three"
EXP_grep "file.txt"
EXP_grep "changed"

RUN "$GIT_WIP" log --stat
EXP_grep "file.txt"
EXP_grep "changed"

# -------------------------------------------------------------------------
# log -p -s — both pretty and stat

RUN "$GIT_WIP" log -p -s
EXP_grep "WIP three"
EXP_grep "file.txt"
EXP_grep "changed"

# -------------------------------------------------------------------------
# log -<limit> — limit number of entries

RUN "$GIT_WIP" log -1
EXP_grep "WIP three"
EXP_grep -v "WIP two"
EXP_grep -v "WIP one"

RUN "$GIT_WIP" log -2
EXP_grep "WIP three"
EXP_grep "WIP two"
EXP_grep -v "WIP one"

# -------------------------------------------------------------------------
# log --reflog / -r — shows reflog entries

RUN "$GIT_WIP" log -r
EXP_grep "refs/wip/master"
EXP_grep "git-wip:"

RUN "$GIT_WIP" log --reflog
EXP_grep "refs/wip/master"

# -------------------------------------------------------------------------
# log after work branch advances — wip log resets to new base

RUN git add file.txt
RUN git commit -m "\"real commit\""

RUN "echo v5 >file.txt"
RUN "$GIT_WIP" save "\"WIP after commit\""

# now log should show only the new wip commit, not the old ones
RUN "$GIT_WIP" log
EXP_grep "WIP after commit"
EXP_grep -v "WIP three"
EXP_grep -v "WIP two"
EXP_grep -v "WIP one"

# -------------------------------------------------------------------------
# log -h / --help — shows usage

RUN "$GIT_WIP" log -h
EXP_grep "Usage:"
EXP_grep "pretty"
EXP_grep "stat"
EXP_grep "reflog"

RUN "$GIT_WIP" log --help
EXP_grep "Usage:"

echo "OK: $TEST_NAME"
