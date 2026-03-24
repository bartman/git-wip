# lib.sh -- shared helpers for git-wip Neovim integration tests
#
# Source this file from each test script; do NOT execute directly.
#
# Required environment variables (set by ctest via CMakeLists.txt):
#   GIT_WIP     path to the git-wip binary under test
#   NVIM        path to the nvim binary
#   TEST_TREE   base directory for test artifacts (one subdir per test)
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
[ -z "${NVIM}"      ] && die "NVIM is not set"

# Check if nvim is available, skip if not
if ! command -v "${NVIM}" &>/dev/null; then
    note "nvim not found at ${NVIM} - skipping test"
    exit 0
fi

# Get the repo root (parent of test/nvim/)
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

# Derive test name from the calling script's filename
TEST_NAME="$(basename "$0" .sh)"

# ------------------------------------------------------------------------
# Per-test paths

REPO="$TEST_TREE/$TEST_NAME/repo"
NVIM_CMD="$TEST_TREE/$TEST_NAME/nvim_cmd"
NVIM_OUT="$TEST_TREE/$TEST_NAME/nvim_out"
NVIM_RC="$TEST_TREE/$TEST_NAME/nvim_rc"
NVIM_LOG="$TEST_TREE/$TEST_NAME/nvim_log"

# Clean before running so each run starts fresh; leave artifacts after for debugging
rm -rf "$TEST_TREE/$TEST_NAME"
mkdir -p "$TEST_TREE/$TEST_NAME"

note "Running $TEST_NAME (artifacts in $TEST_TREE/$TEST_NAME)"

# ------------------------------------------------------------------------
# Git helpers (from cli lib.sh)

create_test_repo() {
    rm -rf "$REPO"
    mkdir -p "$REPO"
    cd "$REPO"
    git init
    git config user.email "test@example.com"
    git config user.name "Test User"
    # Force branch name to "master" regardless of init.defaultBranch config
    git checkout -b master
}

# Git helper functions (simplified from cli/lib.sh)
_run() {
    note "$@"
    [ "$(pwd)" = "$REPO" ] || die "expected cwd=$REPO, got $(pwd)"

    set +e
    eval "$@"
    local rc=$?
    set -e
    return $rc
}

run() {
    _run "$@" || die "command failed: $*"
}

# ------------------------------------------------------------------------
# Neovim helpers

# _run_nvim -- Run nvim with sandboxed config, capturing output and exit code
# Usage: _run_nvim [ex-command]...
# Each argument is executed as a separate -c command in nvim
# Always appends quit! at the end to ensure headless nvim exits
_run_nvim() {
    # Build the -c arguments from positional parameters
    local nvim_args=()
    for cmd in "$@"; do
        nvim_args+=(-c "$cmd")
    done

    note "nvim -c ${*//$'\n'/ }"

    # Create sandboxed init.lua that loads our plugin
    cat >"$REPO/init.lua" <<INITLUA
-- Sandboxed Neovim config for git-wip testing
-- Load the plugin from the repo root
local repo_root = os.getenv("REPO_ROOT") or "${REPO_ROOT}"

-- Add the lua/?.lua path so require("git-wip") finds lua/git-wip/init.lua
package.path = repo_root .. "/lua/?.lua;" .. repo_root .. "/lua/?/init.lua;" .. package.path

local git_wip_path = os.getenv("GIT_WIP") or "git-wip"
require("git-wip").setup({
    git_wip_path = git_wip_path,
    untracked = false,
    ignored = false,
    filetypes = { "*" },
})
INITLUA

    set +e
    printf '%s' "nvim ${nvim_args[*]} -c quit!" >"$NVIM_CMD"

    # Run nvim with a watchdog timeout (10 seconds max)
    # This prevents tests from hanging indefinitely
    GIT_WIP="$GIT_WIP" \
    REPO_ROOT="$REPO_ROOT" \
    timeout 10 "$NVIM" --headless -u "$REPO/init.lua" "${nvim_args[@]}" -c "quit!" >"$NVIM_OUT" 2>&1
    local rc=$?
    
    # timeout returns 124 if killed, 125 if timeout command itself failed
    if [ $rc -eq 124 ]; then
        echo "TIMEOUT: nvim was killed after 10 seconds" >>"$NVIM_OUT"
    fi
    printf '%s' "$rc" >"$NVIM_RC"
    set -e
}

# run_nvim -- Run nvim and fail if it exits non-zero
# Note: git-wip runs asynchronously via vim.system, so we add a delay
# to ensure the command completes before checking results.
run_nvim() {
    _run_nvim "$@"
    local rc
    rc="$(cat "$NVIM_RC")"
    if [ "$rc" != 0 ]; then
        handle_error
    fi
    # Wait for async git-wip to complete
    sleep 0.5
}

handle_error() {
    set +e
    warn "CMD='$(cat "$NVIM_CMD")' RC=$(cat "$NVIM_RC")"
    cat >&2 "$NVIM_OUT"
    exit 1
}
