#!/bin/bash

SUDO=sudo
[ "$(id -u)" = 0 ] && SUDO=

function die() {
    echo >&2 "ERROR: $*"
    exit 1
}

function must_have_one_of() {
    for n in "$@" ; do
        if apt policy "$n" >/dev/null 2>&1 ; then
            echo "$n"
            return
        fi
    done
    die "apt cannot find any of these packages: $*"
}

function want_one_of() {
    for n in "$@" ; do
        if apt policy "$n" >/dev/null 2>&1 ; then
            echo "$n"
            return
        fi
    done
}

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------

compiler=""   # empty → auto-select via must_have_one_of

for arg in "$@" ; do
    case "$arg" in
        --compiler=gcc)   compiler=gcc   ;;
        --compiler=gnu)   compiler=gnu   ;;
        --compiler=g++)   compiler=g++   ;;
        --compiler=clang) compiler=clang ;;
        --compiler=*)
            die "unknown --compiler value '${arg#--compiler=}' (expected gcc or clang)" ;;
        -h|--help)
            cat <<'EOF'
Usage: dependencies.sh [--compiler=<gcc|clang>] [-h|--help]

Install build dependencies for git-wip.

Options:
  --compiler=gcc    Install gcc and g++
  --compiler=clang  Install clang
  (no flag)         Install whichever of clang/gcc is available (auto-select)

  -h, --help        Show this help and exit
EOF
            exit 0
            ;;
        *)
            die "unknown argument '$arg'" ;;
    esac
done

# ---------------------------------------------------------------------------
# Compiler packages
# ---------------------------------------------------------------------------

compiler_packages=()

case "$compiler" in
    gcc|gnu|g++)
        compiler_packages+=( gcc g++ )
        ;;
    clang)
        compiler_packages+=( $(must_have_one_of clang) )
        ;;
    "")
        # No preference — pick whatever is available, prefer clang for the
        # C compiler slot and gcc for the C++ slot (matches the old behaviour).
        compiler_packages+=(
            $(must_have_one_of clang gcc)
            $(must_have_one_of clang g++)
        )
        ;;
esac

# ---------------------------------------------------------------------------
# Full package list
# ---------------------------------------------------------------------------

packages=(
    cmake
    $(want_one_of clangd)
    "${compiler_packages[@]}"
    git
    pkg-config
    googletest
    libgmock-dev
    libgtest-dev
    libgit2-dev
    make
    ninja-build
)

set -e -x
$SUDO apt update
$SUDO apt install -y "${packages[@]}"
