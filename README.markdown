# About

git-wip is a script that will manage Work In Progress (or WIP) branches.
WIP branches are mostly throw away but identify points of development
between commits.  The intent is to tie this script into your editor so
that each time you save your file, the git-wip script captures that
state in git.  git-wip also helps you return back to a previous state of
development.

Latest git-wip can be obtained from [github.com](http://github.com/bartman/git-wip)
git-wip was written by [Bart Trojanowski](mailto:bart@jukie.net)

# WIP branches

Wip branches are named after the branch that is being worked on, but are
prefixed with 'wip/'.  For example if you are working on a branch named
'feature' then the git-wip script will only manipulate the 'wip/feature'
branch.

When you run git-wip for the first time, it will capture all changes to
tracked files and all untracked (but not ignored) files, create a
commit, and make a new wip/*topic* branch point to it.

    --- * --- * --- *          <-- topic
                     \
                      *        <-- wip/topic

The next invocation of git-wip after a commit is made will continue to
evolve the work from the last wip/*topic* point.

    --- * --- * --- *          <-- topic
                     \
                      *
                       \
                        *      <-- wip/topic

Whne git-wip is invoked after a commit is made, the state of the
wip/*topic* branch will be reset back to your *topic* branch and the new
changes to the working tree will be caputred on a new commit.

    --- * --- * --- * --- *    <-- topic
                     \     \
                      *     *  <-- wip/topic
                       \
                        *

While the old wip/*topic* work is no longer accessible directly, it can
alwasy be recovered from git-reflog.  In the above example you could use
`wip/topic@{1}` to access the dangling references.

# git-wip command

The git-wip command can be invoked in several differnet ways.

* `git wip`
  
  In this mode, git-wip will create a new commit on the wip/*topic*
  branch (creating it if needed) as described above.

* `git wip save "description"`
  
  Similar to `git wip`, but allows for a custom commit message.

* `git wip log`
  
  Show the list of the work that leads upto the last WIP commit.  This
  is similar to invoking:
  
  `git log --stat wip/$branch...$(git merge-base wip/$branch $branch)`

# editor hooking

To use git-wip effectively, you should tie it into your editor so you
don't have to remember to run git-wip manually.

To add git-wip support to vim add the following to your `.vimrc`.  Doing
so will make it be invoked after every `:w` operation.

    augroup git-wip
      autocmd!
      autocmd BufWritePost * :silent !git wip save "WIP from vim" --editor -- "%"
    augroup END

The `--editor` option puts git-wip into a special mode that will make it
more quiet and not report errors if there were no changes made to the
file.



<!-- vim: set ft=mkd -->
