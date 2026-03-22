#include "cmd_status.hpp"
#include "git_guards.hpp"
#include "git_helpers.hpp"
#include "string_helpers.hpp"
#include "wip_helpers.hpp"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <iostream>
#include "print_compat.hpp"
#include <string>
#include <vector>

int StatusCmd::run(int argc, char *argv[]) {
    // -----------------------------------------------------------------------
    // 1. Parse arguments
    // -----------------------------------------------------------------------
    bool list_mode  = false;
    bool files_mode = false;

    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        if (a == "-l" || a == "--list") {
            list_mode = true;
        } else if (a == "-f" || a == "--files") {
            files_mode = true;
        } else if (a == "--help" || a == "-h") {
            std::println("Usage: git-wip status [-l|--list] [-f|--files]\n");
            //                -                     #
            std::println("    -l, --list            # show each wip commit (short sha, subject, age)");
            std::println("    -f, --files           # show diff --stat of wip changes\n");
            return 0;
        } else {
            spdlog::error("git-wip status: unknown option '{}'", a);
            return 1;
        }
    }

    spdlog::debug("status: list={} files={}", list_mode, files_mode);

    // -----------------------------------------------------------------------
    // 2. Open repository
    // -----------------------------------------------------------------------
    GitLibGuard git_lib_guard;

    GitRepoGuard repo_guard;
    if (git_repository_open_ext(repo_guard.ptr(), ".", 0, nullptr) < 0) {
        spdlog::error("not a git repository: {}", git_error_str());
        return 1;
    }
    git_repository *repo = repo_guard.get();

    // -----------------------------------------------------------------------
    // 3. Resolve branch names
    // -----------------------------------------------------------------------
    auto bn = resolve_branch_names(repo);
    if (!bn) {
        spdlog::error("not on a local branch");
        return 1;
    }

    spdlog::debug("status: work_branch='{}' wip_ref='{}'", bn->work_branch, bn->wip_ref);

    // -----------------------------------------------------------------------
    // 4. Resolve work_last and wip_last OIDs
    // -----------------------------------------------------------------------
    auto work_last = resolve_oid(repo, bn->work_ref);
    if (!work_last) {
        spdlog::error("branch '{}' has no commits", bn->work_branch);
        return 1;
    }

    auto wip_last = resolve_oid(repo, bn->wip_ref);
    if (!wip_last) {
        std::println("branch {} has no wip commits", bn->work_branch);
        return 0;
    }

    spdlog::debug("status: work_last={} wip_last={}",
                  oid_to_hex(&*work_last), oid_to_hex(&*wip_last));

    // -----------------------------------------------------------------------
    // 5. Collect the WIP-only commits (newest first)
    // -----------------------------------------------------------------------
    auto wip_commits = collect_wip_commits(repo, *wip_last, *work_last);
    if (!wip_commits) {
        spdlog::error("cannot enumerate wip commits: {}", git_error_str());
        return 1;
    }

    spdlog::debug("status: {} wip commit(s)", wip_commits->size());

    // -----------------------------------------------------------------------
    // 6. Summary line
    // -----------------------------------------------------------------------
    std::println("branch {} has {} wip commit{} on {}",
                 bn->work_branch,
                 wip_commits->size(),
                 wip_commits->size() == 1 ? "" : "s",
                 bn->wip_ref);
    std::cout.flush();

    // -----------------------------------------------------------------------
    // 7. Optional detail modes
    // -----------------------------------------------------------------------
    if (list_mode) {
        for (const auto &oid : *wip_commits) {
            GitCommitGuard commit;
            if (git_commit_lookup(commit.ptr(), repo, &oid) < 0) continue;

            const git_signature *author = git_commit_author(commit.get());
            std::println("{} - {} ({})",
                         oid_to_short_hex(&oid),
                         first_line(git_commit_message(commit.get())),
                         relative_time(author->when.time));
            std::cout.flush();

            if (files_mode) {
                // per-commit diff --stat against its parent
                std::string hex = oid_to_hex(&oid);
                std::system(("git diff --stat " + hex + "^ " + hex).c_str());
            }
        }
    } else if (files_mode) {
        // diff --stat from work branch HEAD to latest wip tip
        std::system(("git diff --stat " +
                     oid_to_hex(&*work_last) + " " +
                     oid_to_hex(&*wip_last)).c_str());
    }

    return 0;
}
