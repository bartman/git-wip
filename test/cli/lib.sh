# lib.sh -- shared helpers for git-wip legacy integration tests
#
# Source this file from each test script; do NOT execute directly.
#
# Required environment variables (set by ctest via CMakeLists.txt):
#   GIT_WIP      path to the git-wip binary under test
#   TEST_TREE    base directory for test artifacts (one subdir per test)
#
# TEST_NAME is derived from the sourcing script's filename (basename without .sh).

set -e

die()  { echo >&2 "ERROR: $*"   ; exit 1 ; }
warn() { echo >&2 "WARNING: $*" ; }
note() { echo >&2 "# $*"        ; }

# ------------------------------------------------------------------------
# Validate environment

[ -z "${GIT_WIP}"   ] && die "GIT_WIP is not set"
[ -x "${GIT_WIP}"   ] || die "GIT_WIP=${GIT_WIP} is not executable"
[ -z "${TEST_TREE}" ] && die "TEST_TREE is not set"

# Derive test name from the calling script's filename
TEST_NAME="$(basename "$0" .sh)"

# ------------------------------------------------------------------------
# Per-test paths

REPO="$TEST_TREE/$TEST_NAME/repo"
CMD="$TEST_TREE/$TEST_NAME/cmd"
OUT="$TEST_TREE/$TEST_NAME/out"
RC="$TEST_TREE/$TEST_NAME/rc"

# Clean before running so each run starts fresh; leave artifacts after for debugging
rm -rf "$TEST_TREE/$TEST_NAME"
mkdir -p "$TEST_TREE/$TEST_NAME"

note "Running $TEST_NAME (artifacts in $TEST_TREE/$TEST_NAME)"

# ------------------------------------------------------------------------
# Test helpers

# Current working directory for _RUN/_RUN_IN; starts at $REPO.
_RUN_CWD=""

_RUN() {
    note "$@"
    [ "$(pwd)" = "$REPO" ] || die "expected cwd=$REPO, got $(pwd)"

    set +e
    printf '%s' "$*" >"$CMD"
    eval "$@" >"$OUT" 2>&1
    printf '%s' "$?" >"$RC"
    set -e
}

RUN() {
    _RUN "$@"
    local rc
    rc="$(cat "$RC")"
    [ "$rc" = 0 ] || handle_error
}

# CD <subdir> — change the working directory for subsequent RUN_IN calls.
# Use CD "" or CD_ROOT to return to $REPO.
CD() {
    local target
    if [ -z "$1" ]; then
        target="$REPO"
    else
        target="$REPO/$1"
    fi
    cd "$target" || die "CD: cannot cd to $target"
    _RUN_CWD="$target"
    note "CD → $(pwd)"
}

CD_ROOT() { CD ""; }

# _RUN_IN — like _RUN but allows cwd to be a subdirectory of $REPO.
_RUN_IN() {
    note "$@"
    local expected="${_RUN_CWD:-$REPO}"
    [ "$(pwd)" = "$expected" ] || die "expected cwd=$expected, got $(pwd)"

    set +e
    printf '%s' "$*" >"$CMD"
    eval "$@" >"$OUT" 2>&1
    printf '%s' "$?" >"$RC"
    set -e
}

# RUN_IN — like RUN but for subdirectory context (set via CD).
RUN_IN() {
    _RUN_IN "$@"
    local rc
    rc="$(cat "$RC")"
    [ "$rc" = 0 ] || handle_error
}

EXP_none() {
    local out
    out="$(head -n1 "$OUT")"
    if [ -n "$out" ]; then
        warn "expected no output, got: $out"
        handle_error
    fi
}

EXP_text() {
    local exp="$1"
    local out
    out="$(head -n1 "$OUT")"
    if [ "$out" != "$exp" ]; then
        warn "exp: $exp"
        warn "out: $out"
        handle_error
    fi
}

EXP_grep() {
    if ! grep -q "$@" <"$OUT"; then
        warn "grep $* — not matched"
        handle_error
    fi
}

create_test_repo() {
    rm -rf "$REPO"
    mkdir -p "$REPO"
    cd "$REPO"
    RUN git init
    # Force branch name to "master" regardless of init.defaultBranch config
    RUN git checkout -b master
}

handle_error() {
    set +e
    warn "CMD='$(cat "$CMD")' RC=$(cat "$RC")"
    cat >&2 "$OUT"
    exit 1
}
