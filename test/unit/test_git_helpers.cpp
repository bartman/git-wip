#include "git_helpers.hpp"
#include "test_repo_fixture.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <set>
#include <string>

// ---------------------------------------------------------------------------
// oid_to_hex / oid_to_short_hex
// ---------------------------------------------------------------------------

TEST(OidToHex, FullLengthIs40) {
    TestRepo repo;
    repo.write_file("f", "hello");
    git_oid oid = repo.commit("init");

    std::string hex = oid_to_hex(&oid);
    EXPECT_EQ(hex.size(), 40u);
    // only hex digits
    EXPECT_TRUE(hex.find_first_not_of("0123456789abcdef") == std::string::npos);
}

TEST(OidToHex, ShortLengthIs7) {
    TestRepo repo;
    repo.write_file("f", "hi");
    git_oid oid = repo.commit("init");

    std::string s = oid_to_short_hex(&oid);
    EXPECT_EQ(s.size(), 7u);
    EXPECT_TRUE(s.find_first_not_of("0123456789abcdef") == std::string::npos);
}

TEST(OidToHex, ShortIsPrefixOfFull) {
    TestRepo repo;
    repo.write_file("f", "data");
    git_oid oid = repo.commit("init");

    std::string full  = oid_to_hex(&oid);
    std::string short_ = oid_to_short_hex(&oid);
    EXPECT_EQ(full.substr(0, 7), short_);
}

// ---------------------------------------------------------------------------
// resolve_branch_names
// ---------------------------------------------------------------------------

TEST(ResolveBranchNames, FreshRepoUnbornHead) {
    // No commits yet → HEAD is unborn → should return nullopt
    TestRepo repo;
    // Don't commit anything — HEAD is unborn
    auto bn = resolve_branch_names(repo.repo());
    EXPECT_FALSE(bn.has_value());
}

TEST(ResolveBranchNames, AfterFirstCommit) {
    TestRepo repo;
    repo.write_file("README", "hi");
    repo.commit("initial");

    auto bn = resolve_branch_names(repo.repo());
    ASSERT_TRUE(bn.has_value());
    EXPECT_EQ(bn->work_branch, "master");
    EXPECT_EQ(bn->work_ref,    "refs/heads/master");
    EXPECT_EQ(bn->wip_ref,     "refs/wip/master");
}

TEST(ResolveBranchNames, DetachedHead) {
    TestRepo repo;
    repo.write_file("f", "x");
    git_oid oid = repo.commit("initial");

    // Detach HEAD by pointing it directly at the commit OID
    git_repository_set_head_detached(repo.repo(), &oid);

    auto bn = resolve_branch_names(repo.repo());
    EXPECT_FALSE(bn.has_value());
}

TEST(ResolveBranchNames, ExplicitBranchName) {
    TestRepo repo;
    repo.write_file("f", "x");
    repo.commit("initial");

    auto bn = resolve_branch_names(repo.repo(), std::string{"feature/foo"});
    ASSERT_TRUE(bn.has_value());
    EXPECT_EQ(bn->work_branch, "feature/foo");
    EXPECT_EQ(bn->work_ref, "refs/heads/feature/foo");
    EXPECT_EQ(bn->wip_ref, "refs/wip/feature/foo");
}

// ---------------------------------------------------------------------------
// resolve_oid
// ---------------------------------------------------------------------------

TEST(ResolveOid, ExistingRef) {
    TestRepo repo;
    repo.write_file("f", "data");
    git_oid expected = repo.commit("initial");

    auto got = resolve_oid(repo.repo(), "refs/heads/master");
    ASSERT_TRUE(got.has_value());
    EXPECT_TRUE(git_oid_equal(&*got, &expected));
}

TEST(ResolveOid, MissingRefReturnsNullopt) {
    TestRepo repo;
    repo.write_file("f", "data");
    repo.commit("initial");

    auto got = resolve_oid(repo.repo(), "refs/wip/master");
    EXPECT_FALSE(got.has_value());
}

TEST(ResolveOid, WipRefAfterCreation) {
    TestRepo repo;
    repo.write_file("f", "data");
    git_oid commit_oid = repo.commit("initial");

    repo.create_ref("refs/wip/master", commit_oid);

    auto got = resolve_oid(repo.repo(), "refs/wip/master");
    ASSERT_TRUE(got.has_value());
    EXPECT_TRUE(git_oid_equal(&*got, &commit_oid));
}

// ---------------------------------------------------------------------------
// ensure_reflog_dir
// ---------------------------------------------------------------------------

TEST(EnsureReflogDir, CreatesDirectoryAndFile) {
    TestRepo repo;
    repo.write_file("f", "x");
    repo.commit("initial");

    ensure_reflog_dir(repo.repo(), "refs/wip/master");

    const char *git_dir = git_repository_path(repo.repo());
    std::filesystem::path reflog_file =
        std::filesystem::path(git_dir) / "logs" / "refs" / "wip" / "master";

    EXPECT_TRUE(std::filesystem::exists(reflog_file))
        << "expected reflog file: " << reflog_file;
}

TEST(EnsureReflogDir, IdempotentSecondCall) {
    TestRepo repo;
    repo.write_file("f", "x");
    repo.commit("initial");

    // Calling twice should not throw or fail
    EXPECT_NO_THROW(ensure_reflog_dir(repo.repo(), "refs/wip/master"));
    EXPECT_NO_THROW(ensure_reflog_dir(repo.repo(), "refs/wip/master"));

    const char *git_dir = git_repository_path(repo.repo());
    std::filesystem::path reflog_file =
        std::filesystem::path(git_dir) / "logs" / "refs" / "wip" / "master";
    EXPECT_TRUE(std::filesystem::exists(reflog_file));
}

TEST(EnsureReflogDir, NestedBranchName) {
    TestRepo repo;
    repo.write_file("f", "x");
    repo.commit("initial");

    ensure_reflog_dir(repo.repo(), "refs/wip/team/feature");

    const char *git_dir = git_repository_path(repo.repo());
    std::filesystem::path reflog_file =
        std::filesystem::path(git_dir) / "logs" / "refs" / "wip" / "team" / "feature";
    EXPECT_TRUE(std::filesystem::exists(reflog_file));
}

// ---------------------------------------------------------------------------
// find_refs
// ---------------------------------------------------------------------------

TEST(FindRefs, FindsWipRefsByPrefix) {
    TestRepo repo;
    repo.write_file("f", "x");
    git_oid oid = repo.commit("initial");

    repo.create_ref("refs/wip/master", oid);
    repo.create_ref("refs/wip/foo", oid);
    repo.create_ref("refs/wip/team/feature", oid);

    auto refs = find_refs(repo.repo(), "refs/wip/");

    EXPECT_EQ(refs.size(), 3u);
    EXPECT_EQ(refs[0], "refs/wip/foo");
    EXPECT_EQ(refs[1], "refs/wip/master");
    EXPECT_EQ(refs[2], "refs/wip/team/feature");
}

TEST(FindRefs, FindsHeadsRefsByPrefix) {
    TestRepo repo;
    repo.write_file("f", "x");
    git_oid oid = repo.commit("initial");

    repo.create_ref("refs/heads/foo", oid);
    repo.create_ref("refs/heads/bar", oid);

    auto refs = find_refs(repo.repo(), "refs/heads");
    std::set<std::string> got(refs.begin(), refs.end());

    EXPECT_TRUE(got.contains("refs/heads/master"));
    EXPECT_TRUE(got.contains("refs/heads/foo"));
    EXPECT_TRUE(got.contains("refs/heads/bar"));
}
