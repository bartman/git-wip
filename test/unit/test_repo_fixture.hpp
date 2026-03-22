#pragma once

// test_repo_fixture.hpp — helpers for creating throwaway git repositories
// inside the cmake binary directory during unit tests.
//
// Usage:
//   TestRepo repo("my_test_name");   // creates build/test/unit/repos/my_test_name/
//   repo.write_file("README", "hello");
//   git_oid oid = repo.commit("initial");
//
// The TEST_TMPDIR environment variable (set by CMakeLists.txt via
// set_tests_properties ENVIRONMENT) points to CMAKE_CURRENT_BINARY_DIR.
// Each TestRepo creates a uniquely-named subdirectory under it.

#include "git_guards.hpp"

#include <git2.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

// ---------------------------------------------------------------------------
// TestRepo
//
// RAII wrapper that initialises a bare-minimum git repository in a temp dir,
// provides helpers for writing files and making commits, and cleans up on
// destruction.
//
// Usage (preferred — name derived automatically from gtest):
//   TestRepo repo;
//
// The repo directory is  $TEST_TMPDIR/<SuiteName>/<TestName>  which is
// guaranteed unique because gtest enforces unique suite+test name pairs.
// ---------------------------------------------------------------------------
class TestRepo {
public:
    // Default constructor: derive the directory name from the currently-running
    // gtest case ("SuiteName/TestName").  Must be called from inside a TEST()
    // body so that current_test_info() is non-null.
    TestRepo() {
        const auto *info = ::testing::UnitTest::GetInstance()->current_test_info();
        if (!info) {
            ADD_FAILURE() << "TestRepo() default constructor called outside a TEST body";
            return;
        }
        // suite_name/test_name — both are guaranteed unique by gtest
        std::string name = std::string(info->test_suite_name()) +
                           "/" + info->name();
        init(name);
    }

    // Explicit constructor: use a caller-supplied name (for special cases).
    explicit TestRepo(const std::string &name) { init(name); }

private:
    void init(const std::string &name) {
        const char *base = std::getenv("TEST_TMPDIR");
        if (!base) {
            ADD_FAILURE() << "TEST_TMPDIR env var not set — cannot create test repo";
            return;
        }

        m_path = std::filesystem::path(base) / name;
        std::filesystem::remove_all(m_path);
        std::filesystem::create_directories(m_path);

        // git init
        if (git_repository_init(m_repo.ptr(), m_path.string().c_str(), /*bare=*/0) < 0)
            throw std::runtime_error("git_repository_init failed: " + last_error());

        // Set identity so commits don't fail
        git_config *cfg = nullptr;
        git_repository_config(&cfg, m_repo.get());
        git_config_set_string(cfg, "user.name",  "Test User");
        git_config_set_string(cfg, "user.email", "test@example.com");
        git_config_free(cfg);

        // Always start on a branch called "master"
        // (override init.defaultBranch if needed)
        // For a fresh repo there are no commits yet, so we just set HEAD
        // symbolically.
        git_repository_set_head(m_repo.get(), "refs/heads/master");
    } // end init()

public:
    ~TestRepo() {
        // repo/libgit2 cleanup handled by RAII guards
    }

    git_repository *repo() { return m_repo.get(); }

    const std::filesystem::path &path() const { return m_path; }

    // Write (or overwrite) a file in the working tree.
    void write_file(const std::string &rel_path, const std::string &content) {
        auto full = m_path / rel_path;
        std::filesystem::create_directories(full.parent_path());
        std::ofstream f(full);
        f << content;
    }

    // Stage all files in the working tree and create a commit.
    // Returns the OID of the new commit.
    git_oid commit(const std::string &message) {
        // Stage everything
        GitIndexGuard idx;
        if (git_repository_index(idx.ptr(), m_repo.get()) < 0)
            throw std::runtime_error("index: " + last_error());
        git_strarray dot{nullptr, 0};
        git_index_add_all(idx.get(), &dot, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr);
        git_index_update_all(idx.get(), &dot, nullptr, nullptr);
        git_index_write(idx.get());

        // Write tree
        git_oid tree_oid{};
        if (git_index_write_tree(&tree_oid, idx.get()) < 0)
            throw std::runtime_error("write_tree: " + last_error());
        GitTreeGuard tree;
        git_tree_lookup(tree.ptr(), m_repo.get(), &tree_oid);

        // Signature
        GitSignatureGuard sig;
        git_signature_now(sig.ptr(), "Test User", "test@example.com");

        // Parent (nullptr if this is the first commit)
        git_oid commit_oid{};
        GitReferenceGuard head_ref;
        const git_commit *parents[1] = {nullptr};
        int n_parents = 0;
        GitCommitGuard parent_commit;
        if (git_repository_head(head_ref.ptr(), m_repo.get()) == 0) {
            git_oid parent_oid{};
            git_reference_name_to_id(&parent_oid, m_repo.get(),
                                     git_reference_name(head_ref.get()));
            git_commit_lookup(parent_commit.ptr(), m_repo.get(), &parent_oid);
            parents[0]  = parent_commit.get();
            n_parents   = 1;
        }

        if (git_commit_create(&commit_oid, m_repo.get(),
                              "HEAD",
                              sig.get(), sig.get(),
                              nullptr, message.c_str(),
                              tree.get(),
                              n_parents, parents) < 0)
            throw std::runtime_error("commit_create: " + last_error());

        return commit_oid;
    }

    // Create a direct reference (e.g. a wip branch).
    void create_ref(const std::string &ref_name, const git_oid &oid) {
        GitReferenceGuard ref;
        git_reference_create(ref.ptr(), m_repo.get(),
                             ref_name.c_str(), &oid,
                             /*force=*/1, "test");
    }

    // Advance HEAD to point to `oid` and update the branch ref.
    // Used to simulate making a real commit after WIP commits.
    void advance_head(const git_oid &oid) {
        git_repository_set_head(m_repo.get(), "refs/heads/master");
        GitReferenceGuard ref;
        git_reference_create(ref.ptr(), m_repo.get(),
                             "refs/heads/master", &oid,
                             /*force=*/1, "advance_head");
    }

private:
    static std::string last_error() {
        const git_error *e = git_error_last();
        return e ? e->message : "(unknown)";
    }

    GitLibGuard           m_git_lib_guard;
    GitRepoGuard          m_repo;
    std::filesystem::path m_path;
};
