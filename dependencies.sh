#!/bin/bash
set -e

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
elif command -v pacman &>/dev/null; then
    pkg_mgr=pacman
elif command -v nix &>/dev/null; then
    die "Nix detected but no shell.nix found. Run 'nix develop' to start a dev shell."
else
    die "Unsupported system: no apt, dnf, pacman, or nix found. dependencies.sh does not support this OS."
fi

echo "Detected package manager: $pkg_mgr"

# sync the package database once at the start (if needed)
case "$pkg_mgr" in
    apt)
        $SUDO apt update
        ;;
    pacman)
        $SUDO pacman -Sy --noconfirm
        ;;
esac

# ---------------------------------------------------------------------------
# Package manager helpers
# ---------------------------------------------------------------------------

function must_have_one_of() {
    local pkg_names=("$@")
    for n in "${pkg_names[@]}" ; do
        case "$pkg_mgr" in
            apt)
                # apt-cache show exits non-zero when the package is unknown;
                # apt policy always exits 0 so it cannot be used for existence checks.
                if apt-cache show "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
            dnf)
                if dnf list available "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
            pacman)
                # Check if package exists (installed or available in repos)
                if pacman -Si "$n" >/dev/null 2>&1 ; then
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
                # apt-cache show exits non-zero when the package is unknown;
                # apt policy always exits 0 so it cannot be used for existence checks.
                if apt-cache show "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
            dnf)
                if dnf list available "$n" >/dev/null 2>&1 ; then
                    echo "$n"
                    return
                fi
                ;;
            pacman)
                # Check if package exists (installed or available in repos)
                if pacman -Si "$n" >/dev/null 2>&1 ; then
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
coverage=0    # --coverage → install lcov, curl, gpg
static=0      # --static  → install static libs needed for STATIC=1 builds

for arg in "$@" ; do
    case "$arg" in
        --compiler=gcc)   compiler=gcc   ;;
        --compiler=gnu)   compiler=gnu   ;;
        --compiler=g++)   compiler=g++   ;;
        --compiler=clang) compiler=clang ;;
        --compiler=*)
            die "unknown --compiler value '${arg#--compiler=}' (expected gcc or clang)" ;;
        --coverage)
            coverage=1 ;;
        --static)
            static=1 ;;
        -h|--help)
            cat <<'EOF'
Usage: dependencies.sh [--compiler=<gcc|clang>] [--coverage] [--static] [-h|--help]

Install build dependencies for git-wip.

Options:
  --compiler=gcc    Install gcc and g++
  --compiler=clang  Install clang
  (no flag)         Install whichever of clang/gcc is available (auto-select)

  --coverage        Also install coverage tools (lcov, curl, gpg)

  --static          Also install static libraries required for `make STATIC=1`
                    (libllhttp-dev and any other missing static .a files)

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
        # On Arch, gcc includes both C and C++ compilers (no separate g++ package)
        if [ "$pkg_mgr" = "pacman" ]; then
            compiler_packages+=( gcc )
        else
            compiler_packages+=( gcc g++ )
        fi
        ;;
    clang)
        compiler_packages+=( "$(must_have_one_of clang)" )
        ;;
    "")
        # No preference — pick whatever is available, prefer clang for the
        # C compiler slot and gcc for the C++ slot (matches the old behaviour).
        if [ "$pkg_mgr" = "pacman" ]; then
            # On Arch, gcc package includes both C and C++ compilers
            compiler_packages+=(
                "$(must_have_one_of clang gcc)"
            )
        else
            compiler_packages+=(
                "$(must_have_one_of clang gcc)"
                "$(must_have_one_of clang g++)"
            )
        fi
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

# Build tools (different package names between apt, dnf, and pacman)
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
    pacman)
        # Arch uses different package names
        packages+=(
            libgit2
        )
        # Replace base packages with Arch equivalents
        packages=( "${packages[@]/ninja-build/ninja}" )
        packages=( "${packages[@]/pkg-config/pkgconf}" )
        packages=( "${packages[@]/python3/python}" )
        # For clang, ensure C++ standard library is available
        if [ "$compiler" = "clang" ] || [ "$compiler" = "" ]; then
            packages+=( gcc-libs )
        fi
        ;;
esac

# Coverage tools (only when --coverage is requested)
if [ "$coverage" = 1 ]; then
    case "$pkg_mgr" in
        apt)
            packages+=( lcov curl gpg )
            ;;
        dnf)
            packages+=( lcov curl gnupg2 )
            ;;
        pacman)
            packages+=( lcov curl gnupg )
            ;;
    esac
fi

# Static-build extra libs (only when --static is requested)
# Most static .a files come from the -dev packages already installed above.
# The extras needed on Debian/Ubuntu:
#   libgpg-error-dev → libgpg-error.a (libssh2 transitive dep)
#   libzstd-dev      → libzstd.a     (libssh2 transitive dep)
#   libkrb5-dev      → provides libgssapi_krb5.so stubs for the dynamic link
#   libllhttp-dev    → libllhttp.a   (libgit2 HTTP parser)
#                      Present on Debian stable; Ubuntu 24.04 (noble) embeds
#                      llhttp statically inside libgit2.a so the package does
#                      not exist there — detected and skipped automatically.
# Fedora and Arch do not ship libgit2.a / libssh2.a / libssl.a, so the static
# build is not supported on those distros; nothing extra to install.
if [ "$static" = 1 ]; then
    case "$pkg_mgr" in
        apt)
            packages+=( libgpg-error-dev libzstd-dev libkrb5-dev )
            # libllhttp-dev is optional — present on Debian stable, absent on
            # Ubuntu 24.04 (noble embeds llhttp statically inside libgit2.a)
            llhttp_pkg=$(want_one_of libllhttp-dev)
            [ -n "$llhttp_pkg" ] && packages+=( "$llhttp_pkg" )
            ;;
        dnf)
            # Fedora does not ship libgit2.a / libssh2.a / libssl.a, so a
            # fully static build is not supported there.  Nothing to install.
            ;;
        pacman)
            # Arch does not ship libgit2.a either; nothing to install.
            ;;
    esac
fi

set -e -x

case "$pkg_mgr" in
    apt)
        $SUDO apt update
        $SUDO apt install -y "${packages[@]}"
        ;;
    dnf)
        $SUDO dnf install -y "${packages[@]}"
        ;;
    pacman)
        $SUDO pacman -Sy --noconfirm "${packages[@]}"
        ;;
esac
