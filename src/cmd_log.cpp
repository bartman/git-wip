#include "cmd_log.hpp"
#include "git_guards.hpp"
#include "git_helpers.hpp"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <iostream>
#include <print>
#include <string>
#include <vector>

int LogCmd::run(int argc, char *argv[]) {
    // -----------------------------------------------------------------------
    // 1. Parse arguments
    // -----------------------------------------------------------------------
    bool pretty     = false;
    bool stat       = false;
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
    // 3. Resolve branch names
    // -----------------------------------------------------------------------
    auto bn = resolve_branch_names(repo);
    if (!bn) {
        std::println(std::cerr, "git-wip: not on a local branch");
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("log: work_branch='{}' wip_ref='{}'", bn->work_branch, bn->wip_ref);

    // -----------------------------------------------------------------------
    // 4. Resolve work_last and wip_last OIDs
    // -----------------------------------------------------------------------
    auto work_last = resolve_oid(repo, bn->work_ref);
    if (!work_last) {
        std::println(std::cerr, "git-wip: '{}' branch has no commits.", bn->work_branch);
        git_libgit2_shutdown();
        return 1;
    }

    auto wip_last = resolve_oid(repo, bn->wip_ref);
    if (!wip_last) {
        std::println(std::cerr, "git-wip: '{}' has no WIP commits.", bn->work_branch);
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("log: work_last={} wip_last={}",
                  oid_to_hex(&*work_last), oid_to_hex(&*wip_last));

    // -----------------------------------------------------------------------
    // 5. Build and exec git log / git reflog
    // -----------------------------------------------------------------------
    const std::string pretty_fmt =
        " --pretty=format:'%Cred%h%Creset -%C(yellow)%d%Creset %s %Cgreen(%cr)%Creset'"
        " --abbrev-commit --date=relative";

    if (reflog_mode) {
        std::string cmd = "git reflog";
        if (stat)   cmd += " --stat";
        if (pretty) cmd += pretty_fmt;
        cmd += " " + bn->wip_ref;
        spdlog::debug("log: running: {}", cmd);
        git_libgit2_shutdown();
        return std::system(cmd.c_str());
    }

    // Compute merge-base to determine the stop point.
    git_oid base_oid{};
    if (git_merge_base(&base_oid, repo, &*wip_last, &*work_last) < 0) {
        std::println(std::cerr, "git-wip: cannot find merge base: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("log: base={}", oid_to_hex(&base_oid));

    // stop = base~1 if base has parents, else base itself
    std::string stop_arg;
    {
        CommitGuard base_commit;
        std::string base_hex = oid_to_hex(&base_oid);
        if (git_commit_lookup(base_commit.ptr(), repo, &base_oid) == 0 &&
            git_commit_parentcount(base_commit.get()) > 0)
            stop_arg = base_hex + "~1";
        else
            stop_arg = base_hex;
    }

    spdlog::debug("log: stop={}", stop_arg);

    std::string cmd = "git log";
    if (pretty) cmd += " --graph" + pretty_fmt;
    if (stat)   cmd += " --stat";
    for (const auto &f : files) cmd += " -- " + f;
    cmd += " " + oid_to_hex(&*wip_last);
    cmd += " " + oid_to_hex(&*work_last);
    cmd += " ^" + stop_arg;

    spdlog::debug("log: running: {}", cmd);
    git_libgit2_shutdown();
    return std::system(cmd.c_str());
}
