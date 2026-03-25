.PHONY: all install coverage test help
.DEFAULT_GOAL := all

SHELL := /usr/bin/env bash

define PRINT_HELP_PYSCRIPT
import re, sys

for line in sys.stdin:
	match = re.match(r'^([a-zA-Z_-]+):.*?## (.*)$$', line)
	if match:
		target, help = match.groups()
		print("%-20s %s" % (target, help))
endef
export PRINT_HELP_PYSCRIPT


# build what we built last time, Release the first time, and can be overridden with TYPE= variable
DEFAULT_TYPE = Release
CURRENT_TYPE = $(shell sed -r -n -e 's/CMAKE_BUILD_TYPE:STRING=//p' build/CMakeCache.txt 2>/dev/null)

Q=$(if ${V},,@)

# user can override these
CMAKE ?= cmake
GENERATOR ?= Ninja
PREFIX ?= $(HOME)/.local
REBUILD ?= false
BUILD ?= build
TYPE ?= $(if ${CURRENT_TYPE},${CURRENT_TYPE},${DEFAULT_TYPE})
NPROC ?= $(shell nproc || echo 1)
CC ?= $(shell which clang gcc cc | head -n1)
CXX ?= $(shell which clang g++ c++ | head -n1)
COVERAGE ?= false
# Locate the right gcov-compatible tool to match the compiler:
# - if CC is clang, use llvm-cov (prefer plain symlink, fall back to versioned)
# - otherwise use plain gcov
# Use = (recursive) not := (immediate) so CC override on the command line is respected.
_IS_CLANG = $(shell $(CC) --version 2>/dev/null | grep -c clang)
GCOV_TOOL = $(if $(filter 1,$(_IS_CLANG)),$(shell which llvm-cov 2>/dev/null || ls /usr/bin/llvm-cov-* 2>/dev/null | sort -V | tail -1) gcov,gcov)
$(info ## TYPE=${TYPE} CC=${CC} CXX=${CXX} COVERAGE=${COVERAGE})

# Coverage flag for CMake
COVERAGE_FLAG = $(if $(filter 1 yes true YES TRUE,${COVERAGE}),-DWIP_COVERAGE=ON,)

GIT_WIP      = ${BUILD}/src/git-wip

all: ## [default] build the project (uses TYPE={Release,Debug}, COVERAGE={true,false})
	${Q}${CMAKE} -G ${GENERATOR} -S. -B${BUILD} -DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DCMAKE_BUILD_TYPE="${TYPE}" ${COVERAGE_FLAG}
	${Q}${CMAKE} --build "${BUILD}" --config "${TYPE}" --parallel "${NPROC}"
	${Q}ln -fs "${BUILD}"/compile_commands.json compile_commands.json
	${Q}ln -fs "${GIT_WIP}" .

clean: ## clean out the build directory
	${Q}${CMAKE} --build "${BUILD}" --target clean

distclean: ## remove build directory completely
	${Q}rm -rf "${BUILD}"/

help:
	${Q}python3 -c "$$PRINT_HELP_PYSCRIPT" < $(MAKEFILE_LIST)

test: ## run unit tests (with ctest, uses REBUILD={true,false}, COVERAGE={true,false})
	${Q}$(if $(filter 1 yes true YES TRUE,${REBUILD}),rm -rf "${BUILD}"/)
	${Q}${CMAKE} -G ${GENERATOR} -S. -B${BUILD} -DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DCMAKE_BUILD_TYPE="${TYPE}" ${COVERAGE_FLAG}
	${Q}${CMAKE} --build "${BUILD}" --config "${TYPE}" --parallel "${NPROC}"
	${Q}cd "${BUILD}"/ && ctest -C "${TYPE}" $(if ${CI},--output-on-failure -VV)
	${Q}echo " ✅ Unit tests complete."

coverage: ## check code coverage (with lcov), uses REBUILD={true,false})
	${Q}$(if $(filter 1 yes true YES TRUE,${REBUILD}),rm -rf "${BUILD}"/)
	${Q}${CMAKE} -G ${GENERATOR} -S. -B${BUILD} -DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DCMAKE_BUILD_TYPE="${TYPE}" -DWIP_COVERAGE=ON -DCMAKE_C_COMPILER="${CC}" -DCMAKE_CXX_COMPILER="${CXX}"
	${Q}${CMAKE} --build "${BUILD}" --config "${TYPE}" --parallel "${NPROC}"
	${Q}cd "${BUILD}"/ && ctest -C "${TYPE}" -VV
	${Q}printf '#!/bin/sh\nexec $(GCOV_TOOL) "$$@"\n' > "${BUILD}/gcov-tool.sh" && chmod +x "${BUILD}/gcov-tool.sh"
	${Q}lcov --capture --directory "${BUILD}" --gcov-tool "${BUILD}/gcov-tool.sh" \
		--ignore-errors inconsistent \
		--ignore-errors inconsistent \
		--ignore-errors format \
		--ignore-errors unsupported \
		--output-file coverage.info
	${Q}lcov --remove coverage.info '/usr/*' '*/${BUILD}/_deps/*' '*/test/*' \
		--ignore-errors inconsistent \
		--ignore-errors inconsistent \
		--ignore-errors format \
		--ignore-errors unsupported \
		--ignore-errors corrupt \
		--output-file coverage.info
	${Q}mkdir -p coverage-report
	${Q}genhtml coverage.info --output-directory coverage-report \
		--ignore-errors inconsistent \
		--ignore-errors inconsistent \
		--ignore-errors corrupt \
		--ignore-errors unsupported \
		--ignore-errors category
	${Q}echo " ✅ Coverage report generated in coverage-report/"

install: ## install the package (to the `PREFIX`, uses REBUILD={true,false})
	${Q}$(if $(filter 1 yes true YES TRUE,${REBUILD}),rm -rf "${BUILD}"/)
	${Q}${CMAKE} -G ${GENERATOR} -S. -B${BUILD} -DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DCMAKE_BUILD_TYPE="${TYPE}"
	${Q}${CMAKE} --build "${BUILD}" --config "${TYPE}" --parallel "${NPROC}"
	${Q}${CMAKE} --build "${BUILD}" --target install --config "${TYPE}"

format: ## format the project sources (uses REBUILD={true,false})
	${Q}$(if $(filter 1 yes true YES TRUE,${REBUILD}),rm -rf "${BUILD}"/)
	${Q}${CMAKE} -G ${GENERATOR} -S. -B${BUILD} -DCMAKE_INSTALL_PREFIX="$(PREFIX)" -DCMAKE_BUILD_TYPE="${TYPE}"
	${Q}${CMAKE} --build "${BUILD}" --target clang-format

full-test: ## like test, but with a full rebuild
	${Q}${MAKE} REBUILD=1 test
full-coverage: ## like coverage, but with a full rebuild
	${Q}${MAKE} REBUILD=1 coverage
