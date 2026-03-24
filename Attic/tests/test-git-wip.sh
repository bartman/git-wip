#!/bin/bash

set -e

# split the script name into BASE directory, and the name it calls itSELF
BASE=$(realpath ${0%/*})
SELF=${0##*/}

# this tells git where to find our copy of git-wip script
export PATH="$BASE:$PATH"

# keep state for each test in this temporary tree
TEST_TREE=/tmp/git-wip-test-$$
REPO="$TEST_TREE/repo"
CMD="$TEST_TREE/cmd"
OUT="$TEST_TREE/out"
RC="$TEST_TREE/rc"

# ------------------------------------------------------------------------
# some helpful utility functions

die() { echo >&2 "ERROR: $@" ; exit 1 ; }
warn() { echo >&2 "WARNING: $@" ; }
comment() { echo >&2 "# $@" ; }

_RUN() {
	comment "$@"
	[ `pwd` = "$REPO" ] || die "expecting to be in $REPO, not `pwd`"

	set +e
	echo "$@" >"$CMD"
	eval "$@" >"$OUT" 2>&1
	echo "$?" >"$RC"
	set -e
}

RUN() {
	_RUN "$@"
	local rc="`cat $RC`"
	[ "$rc" = 0 ] || handle_error
}

EXP_none() {
	local out="$(head -n1 "$OUT")"
	if [ -n "$out" ] ; then
		warn "out: $out"
		handle_error
	fi
}

EXP_text() {
	local exp="$1"
	local out="$(head -n1 "$OUT")"
	if [ "$out" != "$exp" ] ; then
		warn "exp: $exp"
		warn "out: $out"
		handle_error
	fi
}

EXP_grep() {
	if ! grep -q "$@" < "$OUT" ; then
		warn "grep: $@"
		handle_error
	fi
}

create_test_tree() {
	rm -rf "$REPO"
	mkdir -p "$REPO"
	cd "$REPO"
	RUN git init
}

cleanup_test_tree() {
	rm -rf "$TEST_TREE"
}

handle_error() {
	set +e
	warn "CMD='`cat $CMD`' RC=`cat $RC`"
	cat >&1 "$OUT"
	cleanup_test_tree
	exit 1
}

trap cleanup_test_tree EXIT
trap handle_error INT TERM ERR

# ------------------------------------------------------------------------
# tests

test_general() {
	# init

	create_test_tree
	RUN "echo 1 >README"
	RUN git add README
	RUN git commit -m README

	# run wip w/o changes

	_RUN git wip save
	EXP_text "no changes"

	RUN git wip save --editor
	EXP_none

	RUN git wip save -e
	EXP_none

	# expecting a master branch

	RUN git branch
	EXP_grep "^\* master$"
	EXP_grep -v "wip"

	# not expecting  wip ref at this time

	RUN git for-each-ref
	EXP_grep -v "commit.refs/wip/master$"

	# make changes, store wip

	RUN "echo 2 >README"
	RUN git wip save --editor
	EXP_none

	# expecting a wip ref

	RUN git for-each-ref
	EXP_grep "commit.refs/wip/master$"

	# expecting a log entry

	RUN git wip log
	EXP_grep "^commit "
	EXP_grep "^\s\+WIP$"

	# there should be no wip branch

	RUN git branch
	EXP_grep -v "wip"

	# make changes, store wip

	RUN "echo 3 >README"
	RUN git wip save "\"message2\""
	EXP_none

	# expecting both log entries

	RUN git wip log
	EXP_grep "^commit "
	EXP_grep "^\s\+WIP$"
	EXP_grep "^\s\+message2$"

	# make a commit

	RUN git add -u README
	RUN git commit -m README.2

	# make changes, store wip

	RUN "echo 4 >UNTRACKED"
	RUN "echo 4 >README"
	RUN git wip save "\"message3\""
	EXP_none

	# expecting message3, not message2 or original WIP

	RUN git wip log
	EXP_grep "^commit "
	EXP_grep -v "^\s\+WIP$"
	EXP_grep -v "^\s\+message2$"
	EXP_grep "^\s\+message3$"

	# expecting file changes to README, not UNTRACKED

	RUN git wip log --stat
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
}

test_spaces() {
	# init

	create_test_tree
	RUN "echo 1 >\"s p a c e s\""
	RUN git add "\"s p a c e s\""
	RUN git commit -m "\"s p a c e s\""

	# make changes, store wip

	RUN "echo 2 >\"s p a c e s\""
	RUN git wip save "\"message with spaces\""
	EXP_none

	# expecting a wip ref

	RUN git for-each-ref
	EXP_grep "commit.refs/wip/master$"

	# expecting a log entry

	RUN git wip log
	EXP_grep "^commit "
	EXP_grep "^\s\+message with spaces$"
}

# ------------------------------------------------------------------------
# run tests

TESTS=( test_general test_spaces )

for TEST in "${TESTS[@]}" ; do
	echo "-- $TEST"
	$TEST
	echo "OK"
done

trap - INT TERM ERR
cleanup_test_tree
echo "DONE"
