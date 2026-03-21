#pragma once

// wip_helpers.hpp — higher-level helpers that encode the core git-wip
// branching logic shared between the save, log, and status commands.

#include "git_guards.hpp"
#include "git_helpers.hpp"

#include <optional>
#include <vector>

// ---------------------------------------------------------------------------
// wip_parent_oid
//
// Determine the commit that a new WIP commit should be parented on, given:
//   work_last  — current HEAD of the work branch
//   wip_last   — current tip of refs/wip/<branch> (nullopt if none)
//
// Rules (matching the original shell script):
//   • No wip branch yet  → parent = work_last
//   • merge_base(wip_last, work_last) == work_last
//     (work branch hasn't advanced)  → parent = wip_last  (stack on top)
//   • Otherwise (work branch has new commits) → parent = work_last  (reset)
//
// Returns std::nullopt if the two branches are completely unrelated (no
// common ancestor), which is an error the caller should report.
// ---------------------------------------------------------------------------
inline std::optional<git_oid> wip_parent_oid(
    git_repository         *repo,
    const git_oid          &work_last,
    const std::optional<git_oid> &wip_last)
{
    if (!wip_last.has_value())
        return work_last;   // first save — root from work branch HEAD

    git_oid base{};
    if (git_merge_base(&base, repo, &*wip_last, &work_last) < 0)
        return std::nullopt; // unrelated histories

    // If work_last IS the merge-base, the work branch hasn't moved since
    // the last wip save — keep stacking.
    if (git_oid_equal(&base, &work_last))
        return *wip_last;

    // Work branch has advanced — reset the wip stack.
    return work_last;
}

// ---------------------------------------------------------------------------
// collect_wip_commits
//
// Walk from `wip_last` backwards, stopping at (but not including) `work_last`,
// and return the OIDs in topological order (newest first).
//
// Returns an empty vector when the work branch has advanced past the wip
// branch (merge_base != work_last), mirroring the status-command logic:
// there are 0 "current" wip commits in that situation.
//
// Returns std::nullopt on a libgit2 error.
// ---------------------------------------------------------------------------
inline std::optional<std::vector<git_oid>> collect_wip_commits(
    git_repository *repo,
    const git_oid  &wip_last,
    const git_oid  &work_last)
{
    // Compute merge-base to decide whether the wip stack is still "live".
    git_oid base{};
    if (git_merge_base(&base, repo, &wip_last, &work_last) < 0)
        return std::nullopt;

    // Work branch has advanced past the wip branch → 0 current commits.
    if (!git_oid_equal(&base, &work_last))
        return std::vector<git_oid>{};

    RevwalkGuard walk;
    if (git_revwalk_new(walk.ptr(), repo) < 0)
        return std::nullopt;

    git_revwalk_sorting(walk.get(), GIT_SORT_TOPOLOGICAL);
    git_revwalk_push(walk.get(), &wip_last);
    git_revwalk_hide(walk.get(), &work_last);

    std::vector<git_oid> result;
    git_oid oid{};
    while (git_revwalk_next(&oid, walk.get()) == 0)
        result.push_back(oid);

    return result;
}
