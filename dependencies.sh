#!/bin/bash

SUDO=sudo
[ "$(id -u)" = 0 ] && SUDO=

function die() {
    echo >&2 "ERROR: $*"
    exit 1
}

# ---------------------------------------------------------------------------
# Detect package manager
# ---------------------------------------------------------------------------

if command -v nix &>/dev/null && [ -e ~/.nix-profile/etc/profile.d/nix.sh ]; then
    # shellcheck disable=SC1091
    source ~/.nix-profile/etc/profile.d/nix.sh
fi

if command -v nix-shell &>/dev/null || command -v nix &>/dev/null; then
    if [ -f shell.nix ] || [ -f default.nix ]; then
        die "Nix detected. Run 'nix develop' to enter a dev shell."
    fi
fi

pkg_mgr=""
if command -v apt &>/dev/null; then
    pkg_mgr=apt
elif command -v dnf &>/dev/null; then
    pkg_mgr=dnf
elif command -v nix &>/dev/null; then
    die "Nix detected but no shell.nix found. Run 'nix develop' to start a dev shell."
else
    die "Unsupported system: no apt, dnf, or nix found. dependencies.sh does not support this OS."
fi

echo "Detected package manager: $pkg_mgr"

# ---------------------------------------------------------------------------
# Package manager helpers
# ---------------------------------------------------------------------------

function must_have_one_of() {
    local pkg_names=("$@")
    for n in "${pkg_names[@]}" ; do
        case "$pkg_mgr" in
            apt)
                if apt policy "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
            dnf)
                if dnf list installed "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
        esac
    done
    die "$pkg_mgr cannot find any of these packages: ${pkg_names[*]}"
}

function want_one_of() {
    local pkg_names=("$@")
    for n in "${pkg_names[@]}" ; do
        case "$pkg_mgr" in
            apt)
                if apt policy "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
            dnf)
                if dnf list installed "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
        esac
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
        compiler_packages+=( "$(must_have_one_of clang)" )
        ;;
    "")
        # No preference — pick whatever is available, prefer clang for the
        # C compiler slot and gcc for the C++ slot (matches the old behaviour).
        compiler_packages+=(
            "$(must_have_one_of clang gcc)"
            "$(must_have_one_of clang g++)"
        )
        ;;
esac

# ---------------------------------------------------------------------------
# Full package list
# ---------------------------------------------------------------------------

packages=(
    cmake
    git
    make
    ninja-build
    pkg-config
    python3
)

# Optional packages (only install if available)
want_one_of clangd && packages+=($(want_one_of clangd))
#want_one_of valgrind && packages+=($(want_one_of valgrind))

# Compiler packages
packages+=("${compiler_packages[@]}")

# Build tools (different package names between apt and dnf)
case "$pkg_mgr" in
    apt)
        packages+=(
            googletest
            libgmock-dev
            libgtest-dev
            libgit2-dev
        )
        ;;
    dnf)
        packages+=(
            gtest-devel
            gmock-devel
            libgit2-devel
        )
        ;;
esac

set -e -x

case "$pkg_mgr" in
    apt)
        $SUDO apt update
        $SUDO apt install -y "${packages[@]}"
        ;;
    dnf)
        $SUDO dnf install -y "${packages[@]}"
        ;;
esac
