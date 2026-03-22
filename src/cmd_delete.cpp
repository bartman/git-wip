#include "cmd_delete.hpp"
#include "git_guards.hpp"
#include "git_helpers.hpp"
#include "string_helpers.hpp"

#include "spdlog/spdlog.h"

#include "print_compat.hpp"
#include <iostream>
#include <optional>
#include <string>

namespace {

int delete_ref(git_repository *repo, const std::string &wip_ref) {
    if (!resolve_oid(repo, wip_ref)) {
        spdlog::error("'{}' has no WIP commits", strip_prefix(wip_ref, "refs/wip/"));
        return 1;
    }

    if (git_reference_remove(repo, wip_ref.c_str()) < 0) {
        spdlog::error("cannot delete ref '{}': {}", wip_ref, git_error_str());
        return 1;
    }

    std::println("deleted {}", strip_prefix(wip_ref, "refs/"));
    return 0;
}

} // namespace

int DeleteCmd::run(int argc, char *argv[]) {
    bool cleanup_mode = false;
    bool yes_mode = false;
    std::optional<std::string> ref_arg;

    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        if (a == "--cleanup") {
            cleanup_mode = true;
        } else if (a == "--yes") {
            yes_mode = true;
        } else if (a == "--help" || a == "-h") {
            std::println("Usage: git-wip delete [--yes] [<ref>]");
            std::println("       git-wip delete --cleanup\n");
            //                -                     #
            std::println("    --yes                 # skip confirmation when deleting current branch wip ref");
            std::println("    --cleanup             # delete orphaned refs/wip/* entries\n");
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            spdlog::error("git-wip delete: unknown option '{}'", a);
            return 1;
        } else if (ref_arg.has_value()) {
            spdlog::error("git-wip delete: only one ref argument is allowed");
            return 1;
        } else {
            ref_arg = a;
        }
    }

    if (cleanup_mode && ref_arg.has_value()) {
        spdlog::error("git-wip delete: --cleanup cannot be used with <ref>");
        return 1;
    }

    GitLibGuard git_lib_guard;
    GitRepoGuard repo_guard;
    if (git_repository_open_ext(repo_guard.ptr(), ".", 0, nullptr) < 0) {
        spdlog::error("not a git repository: {}", git_error_str());
        return 1;
    }
    git_repository *repo = repo_guard.get();

    if (cleanup_mode) {
        auto wip_refs = find_refs(repo, "refs/wip/");
        std::size_t removed = 0;

        for (const auto &wip_ref : wip_refs) {
            auto bn = resolve_branch_names(repo, wip_ref);
            if (!bn) continue;

            if (resolve_oid(repo, bn->work_ref))
                continue; // matching branch exists

            if (git_reference_remove(repo, wip_ref.c_str()) < 0) {
                spdlog::error("cannot delete ref '{}': {}", wip_ref, git_error_str());
                return 1;
            }
            ++removed;
            std::println("deleted {}", strip_prefix(wip_ref, "refs/"));
        }

        std::println("deleted {} orphaned wip ref{}",
                     removed,
                     removed == 1 ? "" : "s");
        return 0;
    }

    std::optional<BranchNames> bn;
    if (ref_arg.has_value())
        bn = resolve_branch_names(repo, ref_arg);
    else
        bn = resolve_branch_names(repo);

    if (!bn) {
        spdlog::error("not on a local branch");
        return 1;
    }

    if (!ref_arg.has_value() && !yes_mode) {
        std::print("About to delete {} [Y/n] ", strip_prefix(bn->wip_ref, "refs/"));
        std::cout.flush();

        std::string input;
        if (!std::getline(std::cin, input))
            return 1;

        if (!(input.empty() || input == "y" || input == "Y"))
            return 1;
    }

    return delete_ref(repo, bn->wip_ref);
}
