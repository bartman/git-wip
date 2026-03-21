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
