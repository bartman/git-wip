#include "cmd_save.hpp"

#include "clipp.h"
#include "spdlog/spdlog.h"

#include <git2.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <format>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// RAII wrappers for libgit2 objects
// ---------------------------------------------------------------------------

struct RepoGuard {
    git_repository *m_repo = nullptr;
    ~RepoGuard() {
        if (m_repo)
            git_repository_free(m_repo);
    }
    git_repository *get() { return m_repo; }
    git_repository **ptr() { return &m_repo; }
};

struct IndexGuard {
    git_index *m_idx = nullptr;
    ~IndexGuard() {
        if (m_idx)
            git_index_free(m_idx);
    }
    git_index *get() { return m_idx; }
    git_index **ptr() { return &m_idx; }
};

struct TreeGuard {
    git_tree *m_tree = nullptr;
    ~TreeGuard() {
        if (m_tree)
            git_tree_free(m_tree);
    }
    git_tree *get() { return m_tree; }
    git_tree **ptr() { return &m_tree; }
};

struct CommitGuard {
    git_commit *m_commit = nullptr;
    ~CommitGuard() {
        if (m_commit)
            git_commit_free(m_commit);
    }
    git_commit *get() { return m_commit; }
    git_commit **ptr() { return &m_commit; }
};

struct ReferenceGuard {
    git_reference *m_ref = nullptr;
    ~ReferenceGuard() {
        if (m_ref)
            git_reference_free(m_ref);
    }
    git_reference *get() { return m_ref; }
    git_reference **ptr() { return &m_ref; }
};

struct SignatureGuard {
    git_signature *m_sig = nullptr;
    ~SignatureGuard() {
        if (m_sig)
            git_signature_free(m_sig);
    }
    git_signature *get() { return m_sig; }
    git_signature **ptr() { return &m_sig; }
};

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string git_error_str() {
    const git_error *e = git_error_last();
    return e ? e->message : "(unknown error)";
}

// ---------------------------------------------------------------------------
// SaveCmd implementation
// ---------------------------------------------------------------------------

int SaveCmd::run(int argc, char *argv[]) {
    // -----------------------------------------------------------------------
    // 1. Parse arguments
    // -----------------------------------------------------------------------
    bool editor_mode = false;
    bool add_untracked = false;
    bool add_ignored = false;
    bool no_gpg_sign = false;  // accepted but currently unused (libgit2 handles via config)
    std::string message = "WIP"; // default, overridden by positional arg
    std::vector<std::string> files;

    // clipp group: first non-option positional arg is the message, subsequent
    // ones (after optional --) are file paths.
    // We parse manually-ish because clipp doesn't handle "first positional =
    // message, rest = files" cleanly; we pre-process argv instead.

    // Collect raw args (skip argv[0] which is the command name "save")
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i)
        args.emplace_back(argv[i]);

    {
        bool got_message = false;
        bool past_dashdash = false;

        for (size_t i = 0; i < args.size(); ++i) {
            const auto &a = args[i];
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
                // First non-option positional arg is the commit message
                message = a;
                got_message = true;
            } else {
                // Subsequent non-option positional args are files (like the old
                // script does when it detects a file path without --)
                files.push_back(a);
            }
        }
    }

    spdlog::debug("save: message='{}' editor={} untracked={} ignored={} no_gpg_sign={} files={}",
                  message, editor_mode, add_untracked, add_ignored, no_gpg_sign, files.size());

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

    if (git_repository_is_bare(repo)) {
        std::println(std::cerr, "git-wip: cannot use in a bare repository");
        git_libgit2_shutdown();
        return 1;
    }

    // -----------------------------------------------------------------------
    // 3. Get the work branch name
    // -----------------------------------------------------------------------
    ReferenceGuard head_ref;
    if (git_repository_head(head_ref.ptr(), repo) < 0) {
        // HEAD is unborn or detached
        const git_error *e = git_error_last();
        if (e && e->klass == GIT_ERROR_REFERENCE) {
            // Unborn HEAD (no commits yet) — soft error
            if (editor_mode) {
                git_libgit2_shutdown();
                return 0;
            }
            std::println(std::cerr, "git-wip: branch has no commits");
            git_libgit2_shutdown();
            return 1;
        }
        if (editor_mode) {
            git_libgit2_shutdown();
            return 0;
        }
        std::println(std::cerr, "git-wip: git-wip requires a branch");
        git_libgit2_shutdown();
        return 1;
    }

    if (!git_reference_is_branch(head_ref.get())) {
        if (editor_mode) {
            git_libgit2_shutdown();
            return 0;
        }
        std::println(std::cerr, "git-wip: git-wip requires a local branch (not detached HEAD)");
        git_libgit2_shutdown();
        return 1;
    }

    const char *head_name = git_reference_name(head_ref.get()); // e.g. "refs/heads/master"
    std::string work_branch_short = head_name;
    // strip "refs/heads/" prefix
    const std::string heads_prefix = "refs/heads/";
    if (work_branch_short.substr(0, heads_prefix.size()) == heads_prefix)
        work_branch_short = work_branch_short.substr(heads_prefix.size());

    std::string wip_branch = "refs/wip/" + work_branch_short;

    spdlog::debug("save: work_branch='{}' wip_branch='{}'", work_branch_short, wip_branch);

    // -----------------------------------------------------------------------
    // 4. Ensure reflog directory exists for the wip branch
    // -----------------------------------------------------------------------
    {
        const char *git_dir = git_repository_path(repo); // ends with "/"
        std::filesystem::path reflog_path =
            std::filesystem::path(git_dir) / "logs" / wip_branch;
        std::error_code ec;
        std::filesystem::create_directories(reflog_path.parent_path(), ec);
        if (!ec) {
            // touch the file
            if (!std::filesystem::exists(reflog_path, ec)) {
                std::ofstream touch(reflog_path, std::ios::app);
            }
        }
    }

    // -----------------------------------------------------------------------
    // 5. Get work branch HEAD commit
    // -----------------------------------------------------------------------
    git_oid work_last_oid{};
    if (git_reference_name_to_id(&work_last_oid, repo, head_name) < 0) {
        if (editor_mode) {
            git_libgit2_shutdown();
            return 0;
        }
        std::println(std::cerr, "git-wip: '{}' branch has no commits.", work_branch_short);
        git_libgit2_shutdown();
        return 1;
    }

    spdlog::debug("save: work_last={}", git_oid_tostr_s(&work_last_oid));

    // -----------------------------------------------------------------------
    // 6. Determine wip_parent
    //    - If wip branch exists: check merge-base; if work_last is ahead,
    //      reset parent to work_last; otherwise continue from wip_last.
    //    - If wip branch doesn't exist: use work_last.
    // -----------------------------------------------------------------------
    git_oid wip_last_oid{};
    bool wip_last_valid = false;
    git_oid wip_parent_oid{};

    {
        int rc = git_reference_name_to_id(&wip_last_oid, repo, wip_branch.c_str());
        if (rc == 0) {
            wip_last_valid = true;
            spdlog::debug("save: wip_last={}", git_oid_tostr_s(&wip_last_oid));

            git_oid base_oid{};
            if (git_merge_base(&base_oid, repo, &wip_last_oid, &work_last_oid) < 0) {
                std::println(std::cerr, "git-wip: '{}' and '{}' are unrelated.",
                             work_branch_short, wip_branch);
                git_libgit2_shutdown();
                return 1;
            }

            spdlog::debug("save: merge_base={}", git_oid_tostr_s(&base_oid));

            // If the merge-base equals work_last, work branch hasn't advanced,
            // so continue stacking on wip_last.  Otherwise reset to work_last.
            if (git_oid_equal(&base_oid, &work_last_oid)) {
                wip_parent_oid = wip_last_oid;
            } else {
                wip_parent_oid = work_last_oid;
            }
        } else {
            // wip branch doesn't exist yet; start from work_last
            wip_parent_oid = work_last_oid;
        }
    }

    spdlog::debug("save: wip_parent={}", git_oid_tostr_s(&wip_parent_oid));

    // -----------------------------------------------------------------------
    // 7. Build new tree
    //
    //    The old script:
    //      1. copies the main index to a temp file
    //      2. git read-tree <wip_parent>   (populate index from parent tree)
    //      3. git add --update .           (update tracked files from workdir)
    //         or git add .                 (with --untracked)
    //         or git add -f -A .           (with --ignored)
    //      4. git write-tree               (produce tree OID)
    //
    //    We replicate this with libgit2:
    //      - Open a fresh in-memory index (not connected to repo's real index)
    //      - Read the wip_parent tree into it
    //      - Apply working directory changes on top
    //      - Write tree
    // -----------------------------------------------------------------------
    git_oid new_tree_oid{};
    {
        // Get the parent commit's tree
        CommitGuard parent_commit;
        if (git_commit_lookup(parent_commit.ptr(), repo, &wip_parent_oid) < 0) {
            std::println(std::cerr, "git-wip: cannot look up parent commit: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }

        TreeGuard parent_tree;
        if (git_commit_tree(parent_tree.ptr(), parent_commit.get()) < 0) {
            std::println(std::cerr, "git-wip: cannot get parent tree: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }

        // Create a fresh in-memory index and populate it from the parent tree
        IndexGuard wip_index;
        if (git_index_new(wip_index.ptr()) < 0) {
            std::println(std::cerr, "git-wip: cannot create index: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }

        // Associate the index with the repo so git_index_add_all can use the workdir
        // libgit2 in-memory indexes can't be associated; we need the repo's index
        // as a base, but we must NOT write it back.  Instead, we:
        //   - Open the real on-disk index (read-only snapshot)
        //   - Then replicate the read-tree + add logic on top of the parent tree
        //
        // The cleanest approach that matches the shell script:
        //   - Use git_index_new (in-memory)
        //   - git_index_read_tree to populate from parent
        //   - git_index_add_all / git_index_update_all on the workdir
        //
        // BUT: git_index_add_all/update_all fail on a bare (non-associated) index.
        // We need a repo-associated index.  We'll open a second index from the
        // on-disk path but manipulate it without writing it back.

        // Free the in-memory one; use a path-based index associated with the repo
        git_index_free(wip_index.m_idx);
        wip_index.m_idx = nullptr;

        // Open the index from the standard path
        const char *git_dir = git_repository_path(repo);
        std::string index_path = std::string(git_dir) + "index";

        if (git_index_open(wip_index.ptr(), index_path.c_str()) < 0) {
            std::println(std::cerr, "git-wip: cannot open index: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }

        // Associate the index with the repository so workdir operations work
        // (git_index_open gives an index that does NOT know its owning repo;
        //  but git_index_add_all/update_all need the repo's workdir path.
        //  The trick: use git_repository_index to get the real index, then
        //  read_tree into it from our parent and use update_all, but NEVER
        //  write it back to disk.)
        // Actually, git_index_open works for read-tree and write-tree; for
        // add_all/update_all we need the repo's workdir.  We'll get the repo's
        // index, reset it to the parent tree, do our staging, write the tree,
        // and then restore the real index from disk.

        // Better approach: use the real repo index but:
        //   1. Snapshot the current index contents
        //   2. read_tree from parent into it (in-memory)
        //   3. add_all / update_all
        //   4. write_tree (in-memory -> ODB, no disk write)
        //   5. Restore index from disk (hard read)
        // This way the on-disk index is never changed.

        git_index_free(wip_index.m_idx);
        wip_index.m_idx = nullptr;

        // Get the repo's actual index (in-memory handle, not writing to disk)
        if (git_repository_index(wip_index.ptr(), repo) < 0) {
            std::println(std::cerr, "git-wip: cannot get repo index: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }
        git_index *idx = wip_index.get();

        // Read parent tree into index (in-memory; does NOT touch the index file)
        if (git_index_read_tree(idx, parent_tree.get()) < 0) {
            std::println(std::cerr, "git-wip: cannot read parent tree into index: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }

        // Stage working directory changes on top
        if (!files.empty()) {
            // Specific files requested: git add -f <files>
            std::vector<const char *> c_files;
            c_files.reserve(files.size());
            for (const auto &f : files)
                c_files.push_back(f.c_str());
            git_strarray pathspec{const_cast<char **>(c_files.data()),
                                  c_files.size()};
            if (git_index_add_all(idx, &pathspec, GIT_INDEX_ADD_FORCE, nullptr, nullptr) < 0) {
                std::println(std::cerr, "git-wip: cannot stage files: {}", git_error_str());
                // restore index
                git_index_read(idx, /*force=*/1);
                git_libgit2_shutdown();
                return 1;
            }
        } else if (add_ignored) {
            // git add -f -A .   -> add everything including ignored
            git_strarray dot{nullptr, 0};
            if (git_index_add_all(idx, &dot, GIT_INDEX_ADD_FORCE, nullptr, nullptr) < 0) {
                std::println(std::cerr, "git-wip: cannot stage files: {}", git_error_str());
                git_index_read(idx, 1);
                git_libgit2_shutdown();
                return 1;
            }
        } else if (add_untracked) {
            // git add .   -> add tracked + untracked (but not ignored)
            git_strarray dot{nullptr, 0};
            if (git_index_add_all(idx, &dot, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr) < 0) {
                std::println(std::cerr, "git-wip: cannot stage files: {}", git_error_str());
                git_index_read(idx, 1);
                git_libgit2_shutdown();
                return 1;
            }
        } else {
            // Default: git add --update .   -> only update tracked files
            git_strarray dot{nullptr, 0};
            if (git_index_update_all(idx, &dot, nullptr, nullptr) < 0) {
                std::println(std::cerr, "git-wip: cannot update index: {}", git_error_str());
                git_index_read(idx, 1);
                git_libgit2_shutdown();
                return 1;
            }
        }

        // Write the tree object to the ODB (does NOT write the index file)
        if (git_index_write_tree(&new_tree_oid, idx) < 0) {
            std::println(std::cerr, "git-wip: cannot write tree: {}", git_error_str());
            git_index_read(idx, 1);
            git_libgit2_shutdown();
            return 1;
        }

        spdlog::debug("save: new_tree={}", git_oid_tostr_s(&new_tree_oid));

        // Restore the real on-disk index so we haven't changed user's staging area
        git_index_read(idx, /*force=*/1);
    }

    // -----------------------------------------------------------------------
    // 8. Check whether the new tree differs from the parent commit's tree
    // -----------------------------------------------------------------------
    {
        CommitGuard parent_commit;
        if (git_commit_lookup(parent_commit.ptr(), repo, &wip_parent_oid) < 0) {
            std::println(std::cerr, "git-wip: cannot look up parent commit: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }
        git_oid parent_tree_oid = *git_commit_tree_id(parent_commit.get());

        if (git_oid_equal(&new_tree_oid, &parent_tree_oid)) {
            spdlog::debug("save: no changes");
            if (editor_mode) {
                git_libgit2_shutdown();
                return 0;
            }
            std::println("no changes");
            git_libgit2_shutdown();
            return 1;
        }
    }

    spdlog::debug("save: has changes, creating commit");

    // -----------------------------------------------------------------------
    // 9. Create the WIP commit
    // -----------------------------------------------------------------------
    TreeGuard new_tree_obj;
    if (git_tree_lookup(new_tree_obj.ptr(), repo, &new_tree_oid) < 0) {
        std::println(std::cerr, "git-wip: cannot look up new tree: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }

    CommitGuard parent_commit_obj;
    if (git_commit_lookup(parent_commit_obj.ptr(), repo, &wip_parent_oid) < 0) {
        std::println(std::cerr, "git-wip: cannot look up parent commit: {}", git_error_str());
        git_libgit2_shutdown();
        return 1;
    }

    // Get author/committer signature from config (or default)
    SignatureGuard sig;
    if (git_signature_default(sig.ptr(), repo) < 0) {
        // Fall back to a generic signature
        git_signature_now(sig.ptr(), "git-wip", "git-wip@localhost");
    }

    git_oid new_commit_oid{};
    {
        const git_commit *parents[] = {parent_commit_obj.get()};
        int rc = git_commit_create(
            &new_commit_oid,
            repo,
            nullptr, // don't update any ref yet (we do it with create_matching below)
            sig.get(),
            sig.get(),
            nullptr, // use default encoding
            message.c_str(),
            new_tree_obj.get(),
            1,
            parents);
        if (rc < 0) {
            std::println(std::cerr, "git-wip: cannot create commit: {}", git_error_str());
            git_libgit2_shutdown();
            return 1;
        }
    }

    spdlog::debug("save: new_wip={}", git_oid_tostr_s(&new_commit_oid));

    // -----------------------------------------------------------------------
    // 10. Update the wip ref (with reflog message, using old-value guard)
    // -----------------------------------------------------------------------
    {
        // Build reflog message: "git-wip: <first line of message>"
        std::string first_line = message.substr(0, message.find('\n'));
        std::string reflog_msg = "git-wip: " + first_line;

        ReferenceGuard new_ref;
        const git_oid *current_id = wip_last_valid ? &wip_last_oid : nullptr;
        int rc = git_reference_create_matching(
            new_ref.ptr(),
            repo,
            wip_branch.c_str(),
            &new_commit_oid,
            /*force=*/1,
            current_id,
            reflog_msg.c_str());
        if (rc < 0) {
            std::println(std::cerr, "git-wip: cannot update ref '{}': {}", wip_branch, git_error_str());
            git_libgit2_shutdown();
            return 1;
        }
    }

    spdlog::debug("save: SUCCESS");

    git_libgit2_shutdown();
    return 0;
}
