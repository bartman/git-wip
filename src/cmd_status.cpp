#include "cmd_status.hpp"
#include "git_guards.hpp"

#include "spdlog/spdlog.h"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <format>
#include <iostream>
#include <print>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Format a git_time as a human-readable relative string, e.g. "5 minutes ago"
static std::string relative_time(const git_time &gt) {
    auto commit_tp = std::chrono::system_clock::from_time_t(
        static_cast<time_t>(gt.time));
    auto now = std::chrono::system_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(now - commit_tp);

    auto secs  = diff.count();
    if (secs < 0) secs = 0;

    if (secs < 90)
        return std::format("{} seconds ago", secs);

    auto mins = secs / 60;
    if (mins < 90)
        return std::format("{} minutes ago", mins);

    auto hours = mins / 60;
    if (hours < 36)
        return std::format("{} hours ago", hours);

    auto days = hours / 24;
    if (days < 14)
        return std::format("{} days ago", days);

    auto weeks = days / 7;
    if (weeks < 8)
        return std::format("{} weeks ago", weeks);

    auto months = days / 30;
    if (months < 24)
        return std::format("{} months ago", months);

    return std::format("{} years ago", days / 365);
}

// First line of a commit message
static std::string first_line(const char *msg) {
    if (!msg) return {};
    std::string s(msg);
    auto pos = s.find('\n');
    if (pos != std::string::npos)
        s.resize(pos);
    return s;
}

// Short (7-char) hex OID string
static std::string short_oid(const git_oid *oid) {
    char buf[GIT_OID_MAX_HEXSIZE + 1];
    git_oid_tostr(buf, sizeof(buf), oid);
    buf[7] = '\0';
    return buf;
}

// Full hex OID string
static std::string full_oid(const git_oid *oid) {
    char buf[GIT_OID_MAX_HEXSIZE + 1];
    git_oid_tostr(buf, sizeof(buf), oid);
    return buf;
}

// ---------------------------------------------------------------------------
// StatusCmd::run
// ---------------------------------------------------------------------------

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
            std::println("Usage: git-wip status [-l|--list] [-f|--files]");
            std::println("  -l, --list   show each wip commit (short sha, subject, age)");
            std::println("  -f, --files  show diff --stat of wip changes");
            return 0;
        } else {
            std::println(std::cerr, "git-wip status: unknown option '{}'", a);
            return 1;
        }
    }

    spdlog::debug("status: list={} files={}", list_mode, files_mode);

    // -----------------------------------------------------------------------
    // 2. Open repository
    // -----------------------------------------------------------------------
    git_libgit2_init();

    RepoGuard repo_guard;
    if (git_repository_open_ext(repo_guard.ptr(), ".", 0, nullptr) < 0) {
        std::println(std::cerr, "git-wip: not a git repository: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }
    git_repository *repo = repo_guard.get();

    // -----------------------------------------------------------------------
    // 3. Resolve work branch and wip branch names
    // -----------------------------------------------------------------------
    ReferenceGuard head_ref;
    if (git_repository_head(head_ref.ptr(), repo) < 0 ||
        !git_reference_is_branch(head_ref.get())) {
        std::println(std::cerr, "git-wip: not on a local branch");
        git_libgit2_shutdown();
        return 1;
    }

    const char *head_name = git_reference_name(head_ref.get());
    std::string work_branch = head_name;
    const std::string heads_prefix = "refs/heads/";
    if (work_branch.substr(0, heads_prefix.size()) == heads_prefix)
        work_branch = work_branch.substr(heads_prefix.size());

    std::string wip_ref = "refs/wip/" + work_branch;

    spdlog::debug("status: work_branch='{}' wip_ref='{}'", work_branch, wip_ref);

    // -----------------------------------------------------------------------
    // 4. Resolve work_last and wip_last OIDs
    // -----------------------------------------------------------------------
    git_oid work_last_oid{};
    if (git_reference_name_to_id(&work_last_oid, repo, head_name) < 0) {
        std::println(std::cerr, "git-wip: branch '{}' has no commits", work_branch);
        git_libgit2_shutdown();
        return 1;
    }

    git_oid wip_last_oid{};
    if (git_reference_name_to_id(&wip_last_oid, repo, wip_ref.c_str()) < 0) {
        std::println("branch {} has no wip commits", work_branch);
        git_libgit2_shutdown();
        return 0;
    }

    spdlog::debug("status: work_last={} wip_last={}",
                  git_oid_tostr_s(&work_last_oid), git_oid_tostr_s(&wip_last_oid));

    // -----------------------------------------------------------------------
    // 5. Walk from wip_last back to (but not including) the merge-base of
    //    wip_last and work_last, to collect only the WIP-specific commits.
    //
    //    Hiding work_last alone is wrong when the work branch has advanced
    //    past the wip branch: in that case work_last is NOT an ancestor of
    //    wip_last, so hiding it has no effect and the walk traverses back
    //    through commits that belong to the work branch history.
    //
    //    The merge-base is the point where the two branches diverged (i.e.
    //    the work branch commit that the wip branch was originally built on).
    //    Hiding it correctly stops the walk right at that boundary regardless
    //    of whether the work branch has since advanced.
    // -----------------------------------------------------------------------
    git_oid merge_base_oid{};
    if (git_merge_base(&merge_base_oid, repo, &wip_last_oid, &work_last_oid) < 0) {
        std::println(std::cerr, "git-wip: cannot find merge base: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("status: merge_base={}", git_oid_tostr_s(&merge_base_oid));

    // If work_last is NOT the merge-base, the work branch has advanced past
    // the point where the wip branch was built.  The next `save` would reset
    // the wip branch to start from the new work_last, so there are effectively
    // 0 current wip commits to show.
    if (!git_oid_equal(&merge_base_oid, &work_last_oid)) {
        std::println("branch {} has 0 wip commits on {}", work_branch, wip_ref);
        git_libgit2_shutdown();
        return 0;
    }

    // work_last == merge_base: wip commits are stacked on top of work_last.
    // Walk from wip_last and hide work_last (== merge_base) to collect only
    // the wip-specific commits.
    RevwalkGuard walk;
    if (git_revwalk_new(walk.ptr(), repo) < 0) {
        std::println(std::cerr, "git-wip: cannot create revwalk: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }

    git_revwalk_sorting(walk.get(), GIT_SORT_TOPOLOGICAL);
    git_revwalk_push(walk.get(), &wip_last_oid);
    git_revwalk_hide(walk.get(), &work_last_oid);

    std::vector<git_oid> wip_commits; // newest first
    {
        git_oid oid{};
        while (git_revwalk_next(&oid, walk.get()) == 0)
            wip_commits.push_back(oid);
    }

    spdlog::debug("status: {} wip commit(s)", wip_commits.size());

    // -----------------------------------------------------------------------
    // 6. Summary line
    // -----------------------------------------------------------------------
    std::println("branch {} has {} wip commit{} on {}",
                 work_branch,
                 wip_commits.size(),
                 wip_commits.size() == 1 ? "" : "s",
                 wip_ref);
    std::cout.flush();

    // -----------------------------------------------------------------------
    // 7. -l / --list  — one line per commit
    //    -f / --files  — diff --stat of wip changes
    //    -l -f combined — per-commit diff --stat interleaved with list lines
    // -----------------------------------------------------------------------
    if (list_mode) {
        for (const auto &oid : wip_commits) {
            CommitGuard commit;
            if (git_commit_lookup(commit.ptr(), repo, &oid) < 0) continue;

            std::string sha     = short_oid(&oid);
            std::string subject = first_line(git_commit_message(commit.get()));
            const git_signature *author = git_commit_author(commit.get());
            std::string age     = relative_time(author->when);

            std::println("{} - {} ({})", sha, subject, age);
            std::cout.flush();

            if (files_mode) {
                // per-commit diff --stat against its parent
                std::string wip_full = full_oid(&oid);
                std::string cmd = "git diff --stat " + wip_full + "^ " + wip_full;
                std::system(cmd.c_str());
            }
        }
    } else if (files_mode) {
        // -f only: diff --stat from branch HEAD to latest wip commit
        std::string work_full = full_oid(&work_last_oid);
        std::string wip_full  = full_oid(&wip_last_oid);
        std::string cmd = "git diff --stat " + work_full + " " + wip_full;
        std::system(cmd.c_str());
    }

    git_libgit2_shutdown();
    return 0;
}
