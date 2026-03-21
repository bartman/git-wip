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

packages=(
    cmake
    $(want_one_of clangd)
    $(must_have_one_of "clang" "gcc")
    $(must_have_one_of "clang" "g++")
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

