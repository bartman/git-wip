#include "cmd_save.hpp"
#include "git_guards.hpp"
#include "git_helpers.hpp"
#include "string_helpers.hpp"
#include "wip_helpers.hpp"

#include "spdlog/spdlog.h"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include "print_compat.hpp"
#include <string>
#include <vector>

int SaveCmd::run(int argc, char *argv[]) {
    // -----------------------------------------------------------------------
    // 1. Parse arguments
    // -----------------------------------------------------------------------
    bool editor_mode  = false;
    bool add_untracked = false;
    bool add_ignored   = false;
    bool no_gpg_sign   = false; // accepted; libgit2 honours commit.gpgSign via config
    std::string message = "WIP";
    std::vector<std::string> files;

    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);

    bool got_message   = false;
    bool past_dashdash = false;
    for (const auto &a : args) {
        if (past_dashdash) {
            files.push_back(a);
        } else if (a == "--") {
            past_dashdash = true;
        } else if (a == "--editor" || a == "-e") {
            editor_mode = true;
        } else if (a == "--untracked" || a == "-u") {
            add_untracked = true;
        } else if (a == "--ignored" || a == "-i") {
            add_ignored = true;
        } else if (a == "--no-gpg-sign") {
            no_gpg_sign = true;
        } else if (a == "--help" || a == "-h") {
            std::println("Usage: git-wip save [<message>] [--editor|-e] [--untracked|-u] [--ignored|-i] [--no-gpg-sign] [-- <file>...]");
            return 0;
        } else if (!a.empty() && a[0] == '-') {
            std::println(std::cerr, "git-wip save: unknown option '{}'", a);
            return 1;
        } else if (!got_message) {
            message     = a;
            got_message = true;
        } else {
            files.push_back(a);
        }
    }

    spdlog::debug("save: message='{}' editor={} untracked={} ignored={} no_gpg_sign={} files={}",
                  message, editor_mode, add_untracked, add_ignored, no_gpg_sign, files.size());

    // -----------------------------------------------------------------------
    // 2. Open repository
    // -----------------------------------------------------------------------
    GitLibGuard git_lib_guard;

    GitRepoGuard repo_guard;
    if (git_repository_open_ext(repo_guard.ptr(), ".", 0, nullptr) < 0) {
        std::println(std::cerr, "git-wip: not a git repository: {}", git_error_str());
        return 1;
    }
    git_repository *repo = repo_guard.get();

    // -----------------------------------------------------------------------
    // 2b. Normalise file paths to be relative to the workdir root.
    //
    //     git_index_add_all() resolves pathspecs against the workdir root,
    //     but the paths supplied on the command line are relative to the
    //     current working directory (which may be a subdirectory).  Convert
    //     each path: cwd/file → absolute → workdir-relative.
    // -----------------------------------------------------------------------
    if (!files.empty()) {
        const char *workdir_cstr = git_repository_workdir(repo);
        if (workdir_cstr) {
            std::filesystem::path workdir = std::filesystem::canonical(workdir_cstr);
            std::filesystem::path cwd     = std::filesystem::current_path();
            for (auto &f : files) {
                std::filesystem::path abs = std::filesystem::weakly_canonical(cwd / f);
                // Make relative to workdir; weakly_canonical handles non-existent files too
                std::error_code ec;
                auto rel = std::filesystem::relative(abs, workdir, ec);
                if (!ec && !rel.empty())
                    f = rel.string();
                spdlog::debug("save: file '{}' → repo-relative '{}'",
                              (cwd / f).string(), f);
            }
        }
    }

    if (git_repository_is_bare(repo)) {
        std::println(std::cerr, "git-wip: cannot use in a bare repository");
        return 1;
    }

    // -----------------------------------------------------------------------
    // 3. Resolve branch names
    // -----------------------------------------------------------------------
    auto bn = resolve_branch_names(repo);
    if (!bn) {
        if (editor_mode) { return 0; }
        std::println(std::cerr, "git-wip: git-wip requires a local branch");
        return 1;
    }

    spdlog::debug("save: work_branch='{}' wip_ref='{}'", bn->work_branch, bn->wip_ref);

    // -----------------------------------------------------------------------
    // 4. Ensure reflog directory exists for the wip branch
    // -----------------------------------------------------------------------
    ensure_reflog_dir(repo, bn->wip_ref);

    // -----------------------------------------------------------------------
    // 5. Resolve work_last
    // -----------------------------------------------------------------------
    auto work_last = resolve_oid(repo, bn->work_ref);
    if (!work_last) {
        if (editor_mode) { return 0; }
        std::println(std::cerr, "git-wip: '{}' branch has no commits.", bn->work_branch);
        return 1;
    }

    spdlog::debug("save: work_last={}", oid_to_hex(&*work_last));

    // -----------------------------------------------------------------------
    // 6. Determine wip_parent
    // -----------------------------------------------------------------------
    auto wip_last = resolve_oid(repo, bn->wip_ref); // nullopt if no wip branch yet

    if (wip_last)
        spdlog::debug("save: wip_last={}", oid_to_hex(&*wip_last));

    auto wip_parent = wip_parent_oid(repo, *work_last, wip_last);
    if (!wip_parent) {
        std::println(std::cerr, "git-wip: '{}' and '{}' are unrelated.",
                     bn->work_branch, bn->wip_ref);
        return 1;
    }

    spdlog::debug("save: wip_parent={}", oid_to_hex(&*wip_parent));

    // -----------------------------------------------------------------------
    // 7. Build new tree (in-memory; never writes to the on-disk index)
    //
    //    Steps:
    //      a) Load the parent commit's tree into the repo's in-memory index
    //      b) Stage changes from the working directory on top
    //      c) Write the tree object to the ODB
    //      d) Restore the real on-disk index
    // -----------------------------------------------------------------------
    git_oid new_tree_oid{};
    {
        GitCommitGuard parent_commit;
        if (git_commit_lookup(parent_commit.ptr(), repo, &*wip_parent) < 0) {
            std::println(std::cerr, "git-wip: cannot look up parent commit: {}", git_error_str());
            return 1;
        }

        GitTreeGuard parent_tree;
        if (git_commit_tree(parent_tree.ptr(), parent_commit.get()) < 0) {
            std::println(std::cerr, "git-wip: cannot get parent tree: {}", git_error_str());
            return 1;
        }

        GitIndexGuard idx_guard;
        if (git_repository_index(idx_guard.ptr(), repo) < 0) {
            std::println(std::cerr, "git-wip: cannot get repo index: {}", git_error_str());
            return 1;
        }
        git_index *idx = idx_guard.get();

        if (git_index_read_tree(idx, parent_tree.get()) < 0) {
            std::println(std::cerr, "git-wip: cannot read parent tree into index: {}", git_error_str());
            return 1;
        }

        // Stage changes
        int stage_rc = 0;
        if (!files.empty()) {
            std::vector<const char *> c_files;
            c_files.reserve(files.size());
            for (const auto &f : files) c_files.push_back(f.c_str());
            git_strarray ps{const_cast<char **>(c_files.data()), c_files.size()};
            stage_rc = git_index_add_all(idx, &ps, GIT_INDEX_ADD_FORCE, nullptr, nullptr);
        } else if (add_ignored) {
            git_strarray dot{nullptr, 0};
            stage_rc = git_index_add_all(idx, &dot, GIT_INDEX_ADD_FORCE, nullptr, nullptr);
        } else if (add_untracked) {
            git_strarray dot{nullptr, 0};
            stage_rc = git_index_add_all(idx, &dot, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr);
        } else {
            git_strarray dot{nullptr, 0};
            stage_rc = git_index_update_all(idx, &dot, nullptr, nullptr);
        }

        if (stage_rc < 0) {
            std::println(std::cerr, "git-wip: cannot stage changes: {}", git_error_str());
            git_index_read(idx, 1);
            return 1;
        }

        if (git_index_write_tree(&new_tree_oid, idx) < 0) {
            std::println(std::cerr, "git-wip: cannot write tree: {}", git_error_str());
            git_index_read(idx, 1);
            return 1;
        }

        spdlog::debug("save: new_tree={}", oid_to_hex(&new_tree_oid));

        git_index_read(idx, /*force=*/1); // restore on-disk index
    }

    // -----------------------------------------------------------------------
    // 8. Check for changes
    // -----------------------------------------------------------------------
    {
        GitCommitGuard parent_commit;
        git_commit_lookup(parent_commit.ptr(), repo, &*wip_parent);
        git_oid parent_tree_oid = *git_commit_tree_id(parent_commit.get());

        if (git_oid_equal(&new_tree_oid, &parent_tree_oid)) {
            spdlog::debug("save: no changes");
            if (editor_mode) { return 0; }
            std::println("no changes");
            return 1;
        }
    }

    spdlog::debug("save: has changes, creating commit");

    // -----------------------------------------------------------------------
    // 9. Create the WIP commit
    // -----------------------------------------------------------------------
    GitTreeGuard new_tree_obj;
    if (git_tree_lookup(new_tree_obj.ptr(), repo, &new_tree_oid) < 0) {
        std::println(std::cerr, "git-wip: cannot look up new tree: {}", git_error_str());
        return 1;
    }

    GitCommitGuard parent_commit_obj;
    if (git_commit_lookup(parent_commit_obj.ptr(), repo, &*wip_parent) < 0) {
        std::println(std::cerr, "git-wip: cannot look up parent commit: {}", git_error_str());
        return 1;
    }

    GitSignatureGuard sig;
    if (git_signature_default(sig.ptr(), repo) < 0)
        git_signature_now(sig.ptr(), "git-wip", "git-wip@localhost");

    git_oid new_commit_oid{};
    {
        const git_commit *parents[] = {parent_commit_obj.get()};
        if (git_commit_create(&new_commit_oid, repo, nullptr,
                              sig.get(), sig.get(), nullptr,
                              message.c_str(), new_tree_obj.get(),
                              1, parents) < 0) {
            std::println(std::cerr, "git-wip: cannot create commit: {}", git_error_str());
            return 1;
        }
    }

    spdlog::debug("save: new_wip={}", oid_to_hex(&new_commit_oid));

    // -----------------------------------------------------------------------
    // 10. Update the wip ref
    // -----------------------------------------------------------------------
    {
        std::string reflog_msg = "git-wip: " + first_line(message.c_str());
        const git_oid *current_id = wip_last ? &*wip_last : nullptr;

        GitReferenceGuard new_ref;
        if (git_reference_create_matching(new_ref.ptr(), repo,
                                          bn->wip_ref.c_str(),
                                          &new_commit_oid, /*force=*/1,
                                          current_id,
                                          reflog_msg.c_str()) < 0) {
            std::println(std::cerr, "git-wip: cannot update ref '{}': {}",
                         bn->wip_ref, git_error_str());
            return 1;
        }
    }

    spdlog::debug("save: SUCCESS");
    return 0;
}
