# AGENTS.md - git-wip C++ Rewrite

## Guidance from user

- Use c++23 best practices.  Use CamelCase for classes, use snake_case for method names, variable names, etc.  Use #pragma once in headers.  Use m_ prefix for member variables.
- use clipp for arg parsing, use spdlog for debug logging (set `WIP_DEBUG=1` to see debug), use libgit2 for git functionality
- build with `make`, test with `make test`, check CLI option parsing with `make smoke`
- manage/install dependencies with `dependencies.sh` script
- unit tests go into `test/unit/test_*.cpp`
- old scripts are in `Attic/` subdirectory, we try to be backward compatible (at least for vim/ emacs/ sublime/ plugins)

## Old Shell Script Analysis (Attic/git-wip)

This section captures the analysis of the original shell script implementation and the requirements for backward compatibility with editor plugins.

### Commands Implemented

The original bash script supports the following commands:

| Command | Implemented | Notes |
|---------|--------------|-------|
| (no args) | Yes | Defaults to `save "WIP"` |
| save | Yes | Core functionality |
| log | Yes | Shows WIP history |
| info | **No** | Just dies with "info not implemented" |
| delete | **No** | Just dies with "delete not implemented" |
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

Current command implementations (from src/):

| Command | Header | Implementation | Status |
|---------|--------|----------------|--------|
| status | cmd_status.hpp | cmd_status.cpp | Skeleton - parses args but doesn't execute |
| log | cmd_log.hpp | cmd_log.cpp | **Implemented** - resolves branches, merge-base, spawns `git log` |
| save | cmd_save.hpp | cmd_save.cpp | **Implemented** - full libgit2 implementation, passes all legacy tests |
| delete | cmd_delete.hpp | cmd_delete.cpp | Skeleton - parses args but doesn't execute |
| config | - | - | Not implemented (not in main.cpp) |

### save command (cmd_save.cpp) — Implementation Notes

The save command is implemented using libgit2 exclusively (no subprocess spawning).

**Algorithm:**
1. Parse args manually (clipp not used due to "first positional = message, rest = files" pattern)
2. Open repo with `git_repository_open_ext`
3. Resolve HEAD symbolic ref → get work branch short name → compute `refs/wip/<branch>`
4. Ensure `$GIT_DIR/logs/refs/wip/<branch>` exists (for reflog)
5. Resolve `work_last` from HEAD
6. Determine `wip_parent`:
   - If `refs/wip/<branch>` exists: compute merge-base; if `work_last == base` use `wip_last`, else use `work_last`
   - Otherwise: use `work_last`
7. Build new tree (in-memory):
   - Get repo's index handle (via `git_repository_index`)
   - `git_index_read_tree` to populate from parent commit's tree
   - Apply workdir changes: `git_index_update_all` (default), `git_index_add_all` with DEFAULT (untracked), FORCE (ignored), or `git_index_add_bypath` for specific files
   - `git_index_write_tree` to write tree object to ODB
   - `git_index_read(force=1)` to restore real on-disk index
8. Compare new tree OID vs parent commit tree OID — if equal, report "no changes" (or exit 0 in editor mode)
9. Create commit: `git_commit_create` with NULL ref (don't update ref yet)
10. Update ref: `git_reference_create_matching` with `current_id=wip_last` (NULL on first run) and reflog message `"git-wip: <first line>"`

**Important caveat:** `git_index_update_all` / `git_index_add_all` operate on the repo's real in-memory index object, but we do NOT call `git_index_write`, so the on-disk index is never touched. We restore it via `git_index_read(force=1)` after writing the tree.

### log command (cmd_log.cpp) — Implementation Notes

The log command uses libgit2 to compute the range and then spawns `git log` (or `git reflog`) via `std::system()`.

**Algorithm:**
1. Parse args manually
2. Open repo, resolve work branch and wip branch
3. Resolve `work_last` and `wip_last` OIDs
4. Compute `merge-base`; determine stop = `base~1` if base has parents, else `base`
5. Build and exec: `git log [--graph] [--stat] [--pretty=...] <wip_last> <work_last> ^<stop>`

### main.cpp — No-args behavior

When `argc < 2`, main synthesizes `argv = ["save", "WIP"]` and invokes SaveCmd directly, matching the old shell script's default behavior.

## Required Implementation for Backward Compatibility

### 1. save command must support:

- Positional message argument (e.g., `git wip save "message"`)
- `--editor` flag
- `--untracked` flag
- `--ignored` flag
- `--no-gpg-sign` flag
- `--` delimiter for file arguments
- File arguments (optional, multiple allowed)

### 2. log command must support:

- `--pretty` flag
- `--stat` flag
- `--reflog` flag
- File arguments (optional, multiple allowed)

### 3. Main program must support:

- No arguments: invoke save with default message "WIP"
- `help`, `--help`, `-h`: show help
- Command dispatch to subcommands

### 4. Must handle edge cases:

- No changes to save (should exit quietly with --editor, error otherwise)
- Not on a branch (detached HEAD) - should error
- No commits on current branch - should error
- WIP branch unrelated to work branch - should error

## Notes from Code Review

1. The old script uses `git-sh-setup` from git's exec-path for common git functions
2. The C++ rewrite should use libgit2 (already in dependencies.sh)
3. The old script had extensive debug output (controlled by WIP_DEBUG env var)
4. The old script had incomplete implementations for `info` and `delete` commands
5. The emacs plugin uses `git-wip` (with hyphen) while others use `git wip` (with space)
