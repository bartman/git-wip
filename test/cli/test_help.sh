#!/usr/bin/env bash

source "$(dirname "$0")/lib.sh"

create_test_repo

# Test main help commands

_RUN "$GIT_WIP" help 2>/dev/null

EXP_grep "."

_RUN "$GIT_WIP" --help 2>/dev/null

EXP_grep "."

_RUN "$GIT_WIP" -h 2>/dev/null

EXP_grep "."

_RUN "$GIT_WIP" --version 2>/dev/null

EXP_grep "."

# Test per-command help

for cmd in save status log delete ; do

  _RUN "$GIT_WIP" $cmd --help 2>/dev/null

  EXP_grep "."

  _RUN "$GIT_WIP" $cmd -h 2>/dev/null

  EXP_grep "."

done

echo "OK: $TEST_NAME"