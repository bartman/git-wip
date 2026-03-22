# AGENTS.md - git-wip C++ Rewrite

## Guidance from user

- Use c++23 best practices.  Use CamelCase for classes, use snake_case for method names, variable names, etc.  Use #pragma once in headers.  Use m_ prefix for member variables.
- use manual arg parsing, use spdlog for debug logging (set `WIP_DEBUG=1` to see debug), use libgit2 for git functionality
- build with `make`, test with `make test`
- manage/install dependencies with `dependencies.sh` script
- unit tests go into `test/unit/test_*.cpp`
- CLI integration tests go into `test/cli/test_*.sh` — source `test/cli/lib.sh`, must be executable
- old scripts are in `Attic/` subdirectory, we try to be backward compatible (at least for vim/ emacs/ sublime/ plugins)
- agent will update `AGENTS.md` and `README.md` files with new information, as needed

## Test Infrastructure

### test/cli/lib.sh

Shared bash harness sourced by every CLI test script.  Provides:

- `RUN <cmd>` — run a command, fail the test if it exits non-zero
- `_RUN <cmd>` — run a command without checking exit code (use before `EXP_text`)
- `EXP_none` — assert that the last command produced no output
- `EXP_text <string>` — assert that the first line of output equals `<string>`
- `EXP_grep [opts] <pattern>` — assert that output matches (or with `-v`, does not match) a grep pattern
- `create_test_repo` — init a fresh git repo in `$REPO`, checkout branch `master`
- `handle_error` — print diagnostics and exit 1

Required env vars (set by ctest via `CMakeLists.txt`):
- `GIT_WIP` — path to the binary under test
- `TEST_TREE` — base dir for artifacts; each test gets `$TEST_TREE/$TEST_NAME/`

`TEST_NAME` is derived automatically from `$(basename "$0" .sh)`.

Each test cleans its own subdirectory before running and leaves artifacts after for debugging.

### Adding a new CLI test

1. Create `test/cli/test_<name>.sh` (executable, `chmod +x`)
2. First two lines: `#!/bin/bash` then `source "$(dirname "$0")/lib.sh"`
3. Write test body using `RUN`, `EXP_*`, `create_test_repo`
4. End with `echo "OK: $TEST_NAME"`
5. Add `test_<name>` to the `foreach` list in `test/cli/CMakeLists.txt`

### Quoting in test scripts

All commands go through `eval "$@"` inside `_RUN`.  Multi-word arguments
(commit messages, file names with spaces) must be wrapped in escaped quotes:

```bash
RUN "$GIT_WIP" save "\"message with spaces\""   # correct
RUN git commit -m "\"my commit message\""        # correct
RUN "$GIT_WIP" save "message with spaces"        # WRONG — splits into tokens
```

## Source Layout

```
src/
  main.cpp          # arg dispatch; no-args → save "WIP"
  command.hpp       # abstract Command base class
  git_guards.hpp    # RAII wrappers for libgit2 handles + git_error_str()
  cmd_save.hpp/cpp  # save command
  cmd_log.hpp/cpp   # log command
  cmd_status.hpp/cpp# status command
  cmd_delete.hpp/cpp# delete command (skeleton)
test/cli/
  lib.sh            # shared test harness (not executable)
  test_legacy.sh    # legacy compatibility tests
  test_spaces.sh    # filenames with spaces
  test_status.sh    # status command tests
  test_status2.sh   # status after work-branch advance
  test_save_file.sh # save with explicit file arguments
  CMakeLists.txt    # registers each test_*.sh with ctest
```

## Old Shell Script Analysis (Attic/git-wip)

This section captures the analysis of the original shell script implementation and the requirements for backward compatibility with editor plugins.

### Commands Implemented (original script)

| Command | In old script | Notes |
|---------|--------------|-------|
| (no args) | Yes | Defaults to `save "WIP"` |
| save | Yes | Core functionality |
| log | Yes | Shows WIP history |
| info | **No** | Dies with "info not implemented" |
| delete | **No** | Dies with "delete not implemented" |
| help | Yes | Shows usage |

### Command Syntax (from old script)

```
Usage: git wip [ info | save <message> [ --editor | --untracked | --no-gpg-sign ] | log [ --pretty ] | delete ] [ [--] <file>... ]
```

#### save command

```
git wip save <message> [options] [--] [files...]
```

Options:
- `-e`, `--editor` - Be less verbose, assume called from an editor (suppresses errors when no changes)
- `-u`, `--untracked` - Capture also untracked files
- `-i`, `--ignored` - Capture also ignored files
- `--no-gpg-sign` - Do not sign commit (overrides commit.gpgSign=true)

If no files are specified, all changes to tracked files are saved. If files are specified, only those files are saved.

#### log command

```
git wip log [options] [files...]
```

Options:
- `-p`, `--pretty` - Show a pretty graph with colors
- `-s`, `--stat` - Show diffstat
- `-r`, `--reflog` - Show changes in reflog instead of regular log

### Internal Implementation Details

1. **WIP Branch Naming**: `refs/wip/<branch_name>` where `<branch_name>` is the current local branch (e.g., `refs/wip/master`, `refs/wip/feature`)

2. **First Run Behavior**:
   - Creates a new commit on `wip/<branch>` starting from the current branch HEAD
   - Captures all changes to tracked files
   - Can optionally capture untracked and/or ignored files

3. **Subsequent Run Behavior**:
   - If the work branch has new commits since last WIP:
     - Creates a new WIP commit as a child of the work branch
     - This "resets" the WIP branch to follow the work branch
   - If no new commits on work branch:
     - Continues from the last WIP commit (adds new changes on top)

4. **Tree Building Process** (build_new_tree function):
   - Creates a temporary index file `$GIT_DIR/.git-wip.$$-INDEX`
   - Copies the main git index to the temp index
   - Uses `git read-tree` to populate the index with the parent tree
   - Uses `git add` to stage changes:
     - Default: `git add --update .` (updates tracked files)
     - With `--untracked`: `git add .` (includes untracked)
     - With `--ignored`: `git add -f -A .` (includes ignored)
   - Uses `git write-tree` to create the new tree object

5. **Commit Creation**:
   - Uses `git commit-tree` to create an orphan commit
   - Parent is the determined wip_parent (either last wip commit or work branch HEAD)
   - Commit message is the user-provided message

6. **Reference Update**:
   - Uses `git update-ref` to update the wip branch ref
   - Message format: `git-wip: <first line of message>`
   - Old ref value is passed for safe update (prevent overwriting)

7. **Reflog**:
   - Enables reflog for the wip branch
   - Creates `$GIT_DIR/logs/refs/wip/<branch>` if it doesn't exist
   - Allows recovery of "orphaned" WIP commits via `git reflog`

8. **Editor Mode** (`--editor`):
   - In editor mode, if there are no changes, the script exits quietly (exit code 0)
   - Without editor mode, it reports an error "no changes"

9. **Error Handling**:
   - Requires a working tree (uses `git-sh-setup` functions)
   - Requires being on a local branch (not detached HEAD)
   - Reports soft errors in editor mode (exits 0), hard errors otherwise

## Editor Plugin Commands (Backward Compatibility)

The C++ implementation MUST accept these exact command formats:

### vim (vim/plugin/git-wip.vim, line 53)

```vim
let out = system('cd "' . dir . '" && git wip save "WIP from vim (' . file . ')" ' . wip_opts . ' -- "' . file . '" 2>&1')
```

Full command example:
```
git wip save "WIP from vim (filename)" --editor --no-gpg-sign -- "filename"
```

Key observations:
- Uses `git wip` (space, not hyphen)
- Message format: `WIP from vim (filename)`
- Options: `--editor` and optionally `--no-gpg-sign`
- File argument after `--` delimiter

### emacs (emacs/git-wip.el, line 4)

```lisp
(shell-command (concat "git-wip save \"WIP from emacs: " (buffer-file-name) "\" --editor -- " file-arg))
```

Full command example:
```
git-wip save "WIP from emacs: /path/to/file.el" --editor -- '/path/to/file.el'
```

Key observations:
- Uses `git-wip` (hyphen, not space) - DIFFERENT FROM OTHERS
- Message format: `WIP from emacs: /path/to/file`
- Option: `--editor`
- File argument after `--` delimiter

### sublime (sublime/gitwip.py, line 12-14)

```python
p = Popen(["git", "wip", "save",
           "WIP from ST3: saving %s" % fname,
           "--editor", "--", fname],
```

Full command example:
```
git wip save "WIP from ST3: saving filename" --editor -- filename
```

Key observations:
- Uses `git wip` (space, not hyphen)
- Message format: `WIP from ST3: saving filename`
- Option: `--editor`
- File argument after `--` delimiter

### vim check for git-wip (vim/plugin/git-wip.vim, line 24)

```vim
silent! !git wip -h >/dev/null 2>&1
```

The vim plugin runs `git wip -h` to check if git-wip is installed. This should:
- Either show help and exit 0
- Or at least not error out (the script checks `v:shell_error`)

## Implementation Status (C++)

| Command | Header | Implementation | Status |
|---------|--------|----------------|--------|
| save | cmd_save.hpp | cmd_save.cpp | **Implemented** — full libgit2, passes all tests |
| log | cmd_log.hpp | cmd_log.cpp | **Implemented** — libgit2 range, spawns `git log` |
| status | cmd_status.hpp | cmd_status.cpp | **Implemented** — libgit2 revwalk, `-l`/`-f` flags |
| delete | cmd_delete.hpp | cmd_delete.cpp | **Implemented** — delete one/current/cleanup orphaned wip refs |
| config | — | — | Not implemented |

### git_guards.hpp — RAII wrappers

All libgit2 handle types have lightweight RAII guards in `src/git_guards.hpp`:
`RepoGuard`, `IndexGuard`, `TreeGuard`, `CommitGuard`, `ReferenceGuard`,
`SignatureGuard`, `RevwalkGuard`.  Each exposes `get()` and `ptr()`.
Also provides `inline git_error_str()` for the last libgit2 error message.

Arg parsing is done **manually** in all commands because the
`save` command has a "first positional = message, rest = files" pattern that
is awkward in declarative parsers.  The same manual style was adopted for
consistency across `log` and `status`.

### save command (cmd_save.cpp)

Uses libgit2 exclusively (no subprocess spawning).

**Algorithm:**
1. Parse args manually — first non-option positional = message, remainder after `--` (or bare paths) = files
2. Open repo with `git_repository_open_ext`
3. Resolve HEAD → work branch short name → `refs/wip/<branch>`
4. Ensure `$GIT_DIR/logs/refs/wip/<branch>` exists (for reflog)
5. Resolve `work_last` from HEAD
6. Determine `wip_parent`:
   - If `refs/wip/<branch>` exists: compute merge-base; if `work_last == base` use `wip_last`, else use `work_last`
   - Otherwise: use `work_last`
7. Build new tree (in-memory, never touches the on-disk index):
   - `git_repository_index` → `git_index_read_tree` from parent commit tree
   - Stage changes: `git_index_update_all` (default) / `git_index_add_all` with `DEFAULT` (untracked) / `FORCE` (ignored or specific files)
   - `git_index_write_tree` → OID of new tree object
   - `git_index_read(force=1)` to restore the real on-disk index
8. Compare new tree OID vs parent tree OID — equal → "no changes" (exit 0 in editor mode, exit 1 otherwise)
9. `git_commit_create` with `ref=NULL` (does not update any ref yet)
10. `git_reference_create_matching` with `current_id=wip_last` (NULL on first run) and reflog message `"git-wip: <first line>"`

**Specific-file behaviour:** when files are listed after `--`, `git_index_add_all`
is called with `GIT_INDEX_ADD_FORCE` and a pathspec of exactly those files.
Only the listed files are updated in the wip tree; all others reflect the
parent wip commit state.  Untracked files can be captured this way too.

### log command (cmd_log.cpp)

Uses libgit2 to compute the range, then spawns `git log` via `std::system()`.

**Algorithm:**
1. Resolve work branch and wip branch; look up `work_last` and `wip_last`
2. `git_merge_base` → `base`; stop = `base~1` if base has parents, else `base`
3. `std::system("git log [--graph] [--stat] [--pretty=...] <wip_last> <work_last> ^<stop>")`

### status command (cmd_status.cpp)

Uses libgit2 for all commit enumeration; spawns `git diff --stat` via
`std::system()` for the `-f`/`--files` output.

**Algorithm:**
1. Resolve `work_last` and `wip_last`; if no wip ref → print "no wip commits", exit 0
2. `git_merge_base(wip_last, work_last)` → `base`
3. If `base != work_last`: the work branch has advanced past the wip branch — there are **0 current wip commits** (the next `save` would reset).  Print "0 wip commits", exit 0.
4. Otherwise walk from `wip_last` hiding `work_last` (== base) to collect the wip-only commits (newest first via `GIT_SORT_TOPOLOGICAL`)
5. Print summary: `branch <name> has <N> wip commit(s) on refs/wip/<name>`
6. `-l`/`--list`: for each commit print `<sha7> - <subject> (<age>)`
7. `-f`/`--files`: `git diff --stat <work_last> <wip_last>`
8. `-l -f` combined: per-commit `git diff --stat <commit>^ <commit>` after each list line

**Key bug that was fixed:** originally hid only `work_last` in the revwalk.
When the work branch advances (new real commit), `work_last` is no longer an
ancestor of `wip_last`, so hiding it has no effect and stale wip commits
remain visible.  The fix: hide the merge-base, and short-circuit to "0 commits"
when `merge_base != work_last`.

### main.cpp — No-args behavior

When `argc < 2`, synthesises `argv = ["save", "WIP"]` and invokes `SaveCmd`
directly, matching the old shell script's default behaviour.

## Required Implementation for Backward Compatibility

### 1. save command must support:

- Positional message argument (e.g., `git wip save "message"`)
- `--editor` / `-e` flag
- `--untracked` / `-u` flag
- `--ignored` / `-i` flag
- `--no-gpg-sign` flag
- `--` delimiter for file arguments
- File arguments (optional, multiple allowed)

### 2. log command must support:

- `--pretty` / `-p` flag
- `--stat` / `-s` flag
- `--reflog` / `-r` flag
- File arguments (optional, multiple allowed)

### 3. status command must support:

- `--list` / `-l` flag — one line per wip commit
- `--files` / `-f` flag — diff --stat of wip changes
- Combination of `-l -f` — per-commit diff interleaved with list
- Optional `<ref>` argument (defaults to current branch), where `<ref>` may be:
  - `<branch>`
  - `wip/<branch>`
  - `refs/heads/<branch>`
  - `refs/wip/<branch>`

### 4. Main program must support:

- No arguments: invoke save with default message "WIP"
- `help`, `--help`, `-h`: show help
- Command dispatch to subcommands

### 5. Must handle edge cases:

- No changes to save (exit 0 quietly with `--editor`, print "no changes" + exit 1 otherwise)
- Not on a branch (detached HEAD) — error
- No commits on current branch — error
- WIP branch unrelated to work branch — error
- Work branch has advanced past wip branch — status shows 0 wip commits
