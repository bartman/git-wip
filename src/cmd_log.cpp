#include "cmd_log.hpp"
#include "git_guards.hpp"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <format>
#include <iostream>
#include <print>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// LogCmd::run
// ---------------------------------------------------------------------------

int LogCmd::run(int argc, char *argv[]) {
    // -----------------------------------------------------------------------
    // 1. Parse arguments
    // -----------------------------------------------------------------------
    bool pretty = false;
    bool stat = false;
    bool reflog_mode = false;
    std::vector<std::string> files;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);

    bool past_dashdash = false;
    for (const auto &a : args) {
        if (past_dashdash) {
            files.push_back(a);
        } else if (a == "--") {
            past_dashdash = true;
        } else if (a == "--pretty" || a == "-p") {
            pretty = true;
        } else if (a == "--stat" || a == "-s") {
            stat = true;
        } else if (a == "--reflog" || a == "-r") {
            reflog_mode = true;
        } else if (a == "--help" || a == "-h") {
            std::println("Usage: git-wip log [--pretty|-p] [--stat|-s] [--reflog|-r] [-- <file>...]");
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            std::println(std::cerr, "git-wip log: unknown option '{}'", a);
            return 1;
        } else {
            files.push_back(a);
        }
    }

    spdlog::debug("log: pretty={} stat={} reflog={} files={}", pretty, stat, reflog_mode, files.size());

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
    // 3. Get work branch and wip branch
    // -----------------------------------------------------------------------
    ReferenceGuard head_ref;
    if (git_repository_head(head_ref.ptr(), repo) < 0 ||
        !git_reference_is_branch(head_ref.get())) {
        std::println(std::cerr, "git-wip: not on a local branch");
        git_libgit2_shutdown();
        return 1;
    }

    const char *head_name = git_reference_name(head_ref.get());
    std::string work_branch_short = head_name;
    const std::string heads_prefix = "refs/heads/";
    if (work_branch_short.substr(0, heads_prefix.size()) == heads_prefix)
        work_branch_short = work_branch_short.substr(heads_prefix.size());

    std::string wip_branch = "refs/wip/" + work_branch_short;

    spdlog::debug("log: work_branch='{}' wip_branch='{}'", work_branch_short, wip_branch);

    // -----------------------------------------------------------------------
    // 4. Resolve work_last and wip_last
    // -----------------------------------------------------------------------
    git_oid work_last_oid{};
    if (git_reference_name_to_id(&work_last_oid, repo, head_name) < 0) {
        std::println(std::cerr, "git-wip: '{}' branch has no commits.", work_branch_short);
        git_libgit2_shutdown();
        return 1;
    }

    git_oid wip_last_oid{};
    if (git_reference_name_to_id(&wip_last_oid, repo, wip_branch.c_str()) < 0) {
        std::println(std::cerr, "git-wip: '{}' has no WIP commits.", work_branch_short);
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("log: work_last={} wip_last={}",
                  git_oid_tostr_s(&work_last_oid), git_oid_tostr_s(&wip_last_oid));

    // -----------------------------------------------------------------------
    // 5. Build and exec git log / git reflog command
    //
    //    Mirrors the old shell script:
    //      git log [--graph] [--stat] [--pretty=...] <wip_last> <work_last> ^<stop>
    //    where stop = base if base has no parents, else base~1
    // -----------------------------------------------------------------------

    // Convert OIDs to strings for command-line use
    char work_last_str[GIT_OID_MAX_HEXSIZE + 1];
    char wip_last_str[GIT_OID_MAX_HEXSIZE + 1];
    git_oid_tostr(work_last_str, sizeof(work_last_str), &work_last_oid);
    git_oid_tostr(wip_last_str, sizeof(wip_last_str), &wip_last_oid);

    if (reflog_mode) {
        // git reflog [--stat] [--pretty=...] <wip_branch>
        std::string cmd = "git reflog";
        if (stat) cmd += " --stat";
        if (pretty) {
            cmd += " --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr)%Creset'";
            cmd += " --abbrev-commit --date=relative";
        }
        cmd += " ";
        cmd += wip_branch;
        spdlog::debug("log: running: {}", cmd);
        git_libgit2_shutdown();
        return std::system(cmd.c_str());
    }

    // Regular log: find merge-base to determine the stop point
    git_oid base_oid{};
    if (git_merge_base(&base_oid, repo, &wip_last_oid, &work_last_oid) < 0) {
        std::println(std::cerr, "git-wip: cannot find merge base: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("log: base={}", git_oid_tostr_s(&base_oid));

    // Determine stop: if the merge-base commit has a parent, use base~1; else use base itself
    std::string stop_arg;
    {
        CommitGuard base_commit;
        if (git_commit_lookup(base_commit.ptr(), repo, &base_oid) == 0 &&
            git_commit_parentcount(base_commit.get()) > 0) {
            char base_str[GIT_OID_MAX_HEXSIZE + 1];
            git_oid_tostr(base_str, sizeof(base_str), &base_oid);
            stop_arg = std::string(base_str) + "~1";
        } else {
            char base_str[GIT_OID_MAX_HEXSIZE + 1];
            git_oid_tostr(base_str, sizeof(base_str), &base_oid);
            stop_arg = base_str;
        }
    }

    spdlog::debug("log: stop={}", stop_arg);

    // Build git log command
    std::string cmd = "git log";
    if (pretty) {
        cmd += " --graph";
        cmd += " --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr)%Creset'";
        cmd += " --abbrev-commit --date=relative";
    }
    if (stat) {
        cmd += " --stat";
    }

    // Add any extra file args
    for (const auto &f : files) {
        cmd += " -- ";
        cmd += f;
    }

    cmd += " ";
    cmd += wip_last_str;
    cmd += " ";
    cmd += work_last_str;
    cmd += " ^";
    cmd += stop_arg;

    spdlog::debug("log: running: {}", cmd);

    git_libgit2_shutdown();
    return std::system(cmd.c_str());
}
