# About

[![CI](https://github.com/bartman/git-wip/actions/workflows/ci.yml/badge.svg)](https://github.com/bartman/git-wip/actions)

`git-wip` manages **Work In Progress** (or **WIP**) branches.
WIP branches are mostly throw-away but capture points of development
between commits.  The intent is to tie `git-wip` into your editor so
that each time you save a file the current working-tree state is
snapshotted in git.  `git-wip` also helps you return to a previous
state of development.

Latest `git-wip` can be obtained from [github.com](http://github.com/bartman/git-wip).
`git-wip` was written by [Bart Trojanowski](mailto:bart@jukie.net).
You can find out more from the original [blog post](http://www.jukie.net/bart/blog/save-everything-with-git-wip).

> **Note:** `git-wip` was originally a bash script (2009).
> This repository is a [C++ rewrite](http://www.jukie.net/bart/blog/git-wip-cpp-rewrite/);
> tag [v0.2](https://github.com/bartman/git-wip/releases/tag/v0.2) is the last bash version.
> The script was moved to `Attic/` and is no longer maintained.

---

## TL;DR

```sh
$ make
$ ./dependencies.sh         # assumes you're on a Debian-based distro, and you have `sudo`
$ make install              # installs to ~/.local by default
$ cd my-project
$ git wip                   # snapshot working tree → wip/master
```

---

## How WIP branches work

WIP branches are named after the current branch, prefixed with `wip/`.
If you are working on `feature`, the WIP branch is `wip/feature` (stored
as `refs/wip/feature`).

**First snapshot** — creates a commit on `wip/<branch>` rooted at the
current branch HEAD:

```
--- * --- * --- *          ← feature
                 \
                  W        ← wip/feature
```

**Subsequent snapshots** — stacks new WIP commits on top:

```
--- * --- * --- *          ← feature
                 \
                  W
                   \
                    W      ← wip/feature
```

**After a real commit** — the next `git wip` detects that the work branch
has advanced and resets the WIP branch to start from the new HEAD:

```
--- * --- * --- * --- *    ← feature
                 \     \
                  W     W  ← wip/feature
                   \
                    W  (reachable via reflog as wip/feature@{1})
```

Old WIP commits are never deleted; they remain reachable through
`git reflog show wip/<branch>`.

---

## Commands

### `git wip`

Snapshot the working tree with the default message `"WIP"`.
Equivalent to `git wip save "WIP"`.

### `git wip [--version | -v | version]`

Show the version string (from `git describe --tags --dirty=-dirty` at build time).

```
$ git wip --version
v0.2-83-g95a6648-dirty
```

### `git wip save [<message>] [options] [-- <file>...]`

Create a new WIP commit.

| Option | Description |
|---|---|
| `-e`, `--editor` | Quiet mode — exit 0 silently when there are no changes (for editor hooks) |
| `-u`, `--untracked` | Also capture untracked files |
| `-i`, `--ignored` | Also capture ignored files |
| `--no-gpg-sign` | Do not GPG-sign the commit (overrides `commit.gpgSign = true`) |

If `<file>...` arguments are given, only those files are snapshotted.
Otherwise all tracked files are updated.

### `git wip status [-l] [-f] [<ref>]`

Show the status of the WIP branch for the current work branch, or another branch.

```
$ git wip status
branch master has 5 wip commits on refs/wip/master
```

| Option | Description |
|---|---|
| `-l`, `--list` | List each WIP commit: short SHA, subject, and age |
| `-f`, `--files` | Show a `git diff --stat` of changes relative to the work branch HEAD |

`<ref>` is optional and can be any of:

- `<branch>`
- `wip/<branch>`
- `refs/heads/<branch>`
- `refs/wip/<branch>`

If `<ref>` is omitted, `git wip status` uses the current branch.

`-l` and `-f` can be combined; each commit line is then followed by its
own per-commit diff stat.

```
$ git wip status -l
branch master has 5 wip commits on refs/wip/master
1d146bf - WIP (53 minutes ago)
a3f901c - save file.c (50 minutes ago)
...

$ git wip status -f
branch master has 5 wip commits on refs/wip/master
 src/main.c | 14 ++++++++------
 1 file changed, 8 insertions(+), 6 deletions(-)

$ git wip status -l -f
branch master has 5 wip commits on refs/wip/master
1d146bf - WIP (53 minutes ago)
 src/main.c |  2 +-
 1 file changed, 1 insertion(+), 1 deletion(-)
...
```

### `git wip log [options]`

Show the git log for the current WIP branch.

| Option | Description |
|---|---|
| `-p`, `--pretty` | Compact colourised graph output |
| `-s`, `--stat` | Include per-commit diff stat |
| `-r`, `--reflog` | Show the reflog instead of the commit graph (useful for recovering old WIP stacks) |

### `git wip list [-v]`

List all WIP refs in the repository.

```
$ git wip list
wip/master
wip/feature
```

| Option | Description |
|---|---|
| `-v`, `--verbose` | Show how many WIP commits are ahead of the matching work branch; report orphaned WIP refs |

Verbose output example:

```
$ git wip list -v
wip/master has 5 commits ahead of master
wip/feature has 1 commit ahead of feature
wip/old-branch is orphaned
```

### `git wip delete [--yes] [<ref>]`

Delete WIP refs.

- With `<ref>`, deletes that branch's WIP ref (same ref formats as `status`):
  - `<branch>`
  - `wip/<branch>`
  - `refs/heads/<branch>`
  - `refs/wip/<branch>`
- With no `<ref>`, deletes the current branch's WIP ref and asks for confirmation.
- `--yes` skips the confirmation prompt (only relevant when `<ref>` is omitted).

```
$ git wip delete
About to delete wip/master [Y/n]
```

### `git wip delete --cleanup`

Delete orphaned WIP refs (any `refs/wip/<branch>` where `refs/heads/<branch>`
does not exist).

---

## Building

Requires: a C++23 compiler, CMake ≥ 3.26, Ninja, and `libgit2-dev`.

```sh
$ ./dependencies.sh # install packages needed to build
$ make              # Release build → build/src/git-wip
$ make TYPE=Debug   # Debug build
$ make test         # Build + run all tests (unit + CLI integration)
$ make install      # Install to ~/.local  (override with PREFIX=...)
```

Dependencies (`spdlog`) are fetched automatically by CMake via
`FetchContent`.  `libgit2` must be installed system-wide (e.g.
`apt install libgit2-dev`).

---

## Installation

```sh
$ git clone https://github.com/bartman/git-wip.git
$ cd git-wip
$ ./dependencies.sh
$ make
$ make install          # → ~/.local/bin/git-wip
```

Or copy the binary manually:

```sh
$ cp build/src/git-wip ~/bin/
```

---

## Editor integration

### vim

The vim plugin shells out to `git wip` on every file save, so the `git-wip`
binary must be installed and on your `PATH` before the plugin will do anything.
Easiest way to do this is to run `make install`.

Verify you're ready with:

```sh
$ git wip -h
```

**(1)** With [lazy.nvim](https://github.com/folke/lazy.nvim) (Neovim only):

```lua
{
    "bartman/git-wip",
    opts = {
        gpg_sign = false,
        untracked = true,
        ignored = false,
        filetypes = { "*" },
    },
},
```

**(2)** With [Vundle](https://github.com/gmarik/Vundle.vim) (Vim only):

```vim
Bundle 'bartman/git-wip', {'rtp': 'vim/'}
```

**(3)** Copy the plugin directly:

Not really recommended (or supported), but if you want to you could copy it to your Neovim plugin directory...

```sh
$ cp lua/git-wip/init.vim ~/.config/nvim/plugin/lua/git-wip.lua
```
and then in your `~/.config/nvim/init.lua` you will need to:
```lua
require("git-wip").setup({})
```

Meanwhile if you use Vim, you can do this:
```
$ cp vim/plugin/git-wip.vim ~/.vim/plugin/
```
and Vim will pick it up from there.

**(4)** Or add an autocommand to your `.vimrc`:

This bypasses a plugin, just uses the executable.

```vim
augroup git-wip
  autocmd!
  autocmd BufWritePost * :silent !cd "`dirname "%"`" && git wip save "WIP from vim" --editor -- "`basename "%"`"
augroup END
```

The `--editor` flag makes `git-wip` silent when there are no changes.

### emacs

Add to your `.emacs`:

```lisp
(load "/{path_to_git-wip}/emacs/git-wip.el")
```

Or copy the contents of `emacs/git-wip.el` directly into your `.emacs`.

### Sublime Text

A Sublime Text plugin is provided in the `sublime/` directory.

---

## Recovery

Find the commit you want to recover:

```sh
$ git wip status -l           # show current WIP stack
$ git reflog show wip/master  # show full history including reset points
$ git log -g -p wip/master    # inspect with diffs
```

Check out the files from a WIP commit (HEAD stays where it is):

```sh
$ git checkout wip/master -- .   # restore entire tree
$ git checkout <sha> -- path/to/file  # restore a single file
```

The changes land in the index and working tree.  Review with:

```sh
$ git diff --cached
```

Adjust with `git reset` / `git add -p` as needed, then commit.

# related

- [wip.rs](https://github.com/dlight/wip.rs) is a Rust executable that watches for changes in your work, and invokes `git-wip` as needed. (by [dlight](https://github.com/dlight)).
