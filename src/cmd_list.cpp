#include "cmd_list.hpp"

#include "git_guards.hpp"
#include "git_helpers.hpp"
#include "wip_helpers.hpp"

#include "spdlog/spdlog.h"

#include "print_compat.hpp"

#include <string>

int ListCmd::run(int argc, char *argv[]) {
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        if (a == "-v" || a == "--verbose") {
            verbose = true;
        } else if (a == "--help" || a == "-h") {
            std::println("Usage: git-wip list [-v|--verbose]\n");
            //                -                     #
            std::println("    -v, --verbose         # show ahead counts and orphaned refs\n");
            return 0;
        } else {
            spdlog::error("git-wip list: unknown option '{}'", a);
            return 1;
        }
    }

    GitLibGuard git_lib_guard;

    GitRepoGuard repo_guard;
    if (git_repository_open_ext(repo_guard.ptr(), ".", 0, nullptr) < 0) {
        spdlog::error("not a git repository: {}", git_error_str());
        return 1;
    }
    git_repository *repo = repo_guard.get();

    auto wip_refs = find_refs(repo, "refs/wip/");
    spdlog::debug("list: found {} refs under refs/wip/", wip_refs.size());

    for (const auto &wip_ref : wip_refs) {
        const std::string wip_name = strip_prefix(wip_ref, "refs/");

        if (!verbose) {
            std::println("{}", wip_name);
            continue;
        }

        const std::string branch_name = strip_prefix(wip_ref, "refs/wip/");
        auto bn = resolve_branch_names(repo, branch_name);
        if (!bn) {
            std::println("{} is orphaned", wip_name);
            continue;
        }

        auto work_last = resolve_oid(repo, bn->work_ref);
        auto wip_last = resolve_oid(repo, bn->wip_ref);

        // The wip ref came from find_refs(), so wip_last should always resolve.
        // If either lookup fails (or histories are unrelated), report orphaned.
        if (!work_last || !wip_last) {
            std::println("{} is orphaned", wip_name);
            continue;
        }

        auto wip_commits = collect_wip_commits(repo, *wip_last, *work_last);
        if (!wip_commits) {
            std::println("{} is orphaned", wip_name);
            continue;
        }

        std::println("{} has {} commit{} ahead of {}",
                     wip_name,
                     wip_commits->size(),
                     wip_commits->size() == 1 ? "" : "s",
                     bn->work_branch);
    }

    return 0;
}
