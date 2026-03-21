#!/bin/bash

SUDO=sudo
[ "$(id -u)" = 0 ] && SUDO=

function best() {
    for n in "$@" ; do
        if apt policy "$n" >/dev/null 2>&1 ; then
            echo "$n"
            return
        fi
    done
}

set -e -x
$SUDO apt update
$SUDO apt install -y \
    cmake \
    clangd \
    clang \
    g++ \
    gcc \
    git \
    pkg-config \
    googletest \
    libgmock-dev \
    libgtest-dev \
    libgit2-dev \
    make \
    ninja-build \

