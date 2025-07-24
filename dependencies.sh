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
$SUDO apt install -y \
    cmake \
    g++ \
    gcc \
    git \
    googletest \
    libgmock-dev \
    libgtest-dev \
    make \
    ninja-build \

