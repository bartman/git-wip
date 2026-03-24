#include "wip_helpers.hpp"
#include "test_repo_fixture.hpp"

#include <gtest/gtest.h>

// ---------------------------------------------------------------------------
// wip_parent_oid
// ---------------------------------------------------------------------------

// No wip branch at all → parent should be work_last
TEST(WipParentOid, NoWipBranch) {
    TestRepo repo;
    repo.write_file("f", "v1");
    git_oid work_last = repo.commit("initial");

    auto parent = wip_parent_oid(repo.repo(), work_last, std::nullopt);
    ASSERT_TRUE(parent.has_value());
    EXPECT_TRUE(git_oid_equal(&*parent, &work_last));
}

// wip exists and work branch hasn't moved → parent = wip_last (stack)
TEST(WipParentOid, WorkBranchUnchanged) {
    TestRepo repo;
    repo.write_file("f", "v1");
    git_oid work_last = repo.commit("initial");

    // Simulate a wip commit on top of work_last
    repo.write_file("f", "v2");
    git_oid wip_last = repo.commit("WIP");
    // Reset the work branch back to work_last (wip is ahead)
    repo.create_ref("refs/wip/master", wip_last);
    repo.advance_head(work_last);

    auto parent = wip_parent_oid(repo.repo(), work_last, wip_last);
    ASSERT_TRUE(parent.has_value());
    EXPECT_TRUE(git_oid_equal(&*parent, &wip_last));
}

// wip exists but work branch has advanced → parent = new work_last (reset)
TEST(WipParentOid, WorkBranchAdvanced) {
    TestRepo repo;

    // Build:  A (work) → W (wip)
    //              ↘ B (new work commit)
    repo.write_file("f", "v1");
    git_oid commit_a = repo.commit("commit A");

    repo.write_file("f", "wip");
    git_oid wip_last = repo.commit("WIP");
    repo.create_ref("refs/wip/master", wip_last);

    // Advance work branch: go back to A, make a new commit B
    repo.advance_head(commit_a);
    repo.write_file("f", "v2");
    git_oid commit_b = repo.commit("commit B");

    auto parent = wip_parent_oid(repo.repo(), commit_b, wip_last);
    ASSERT_TRUE(parent.has_value());
    // Should reset to new work_last (commit_b), not stack on wip_last
    EXPECT_TRUE(git_oid_equal(&*parent, &commit_b));
}

// wip and work have no common ancestor → nullopt
TEST(WipParentOid, UnrelatedHistories) {
    // Create two independent repos and steal a commit OID from one to use
    // as a fake wip_last in the other — they'll have no common ancestor.
    TestRepo repo_a("wip_parent_unrelated_a");
    repo_a.write_file("f", "a");
    git_oid oid_a = repo_a.commit("repo a root");

    TestRepo repo_b("wip_parent_unrelated_b");
    repo_b.write_file("f", "b");
    git_oid oid_b = repo_b.commit("repo b root");

    // Use repo_a's repo but present oid_b as the wip_last.
    // git_merge_base will fail because oid_b doesn't exist in repo_a.
    auto parent = wip_parent_oid(repo_a.repo(), oid_a, oid_b);
    EXPECT_FALSE(parent.has_value());
}

// ---------------------------------------------------------------------------
// collect_wip_commits
// ---------------------------------------------------------------------------

// No wip commits on top of work → empty vector
TEST(CollectWipCommits, SingleCommitIsWorkHead) {
    TestRepo repo;
    repo.write_file("f", "v1");
    git_oid work_last = repo.commit("initial");

    // wip_last == work_last: nothing stacked on top
    auto commits = collect_wip_commits(repo.repo(), work_last, work_last);
    ASSERT_TRUE(commits.has_value());
    EXPECT_EQ(commits->size(), 0u);
}

// One wip commit stacked on work_last
TEST(CollectWipCommits, OneWipCommit) {
    TestRepo repo;
    repo.write_file("f", "v1");
    git_oid work_last = repo.commit("initial");

    repo.write_file("f", "v2");
    git_oid wip1 = repo.commit("WIP 1");
    // Reset work branch to work_last
    repo.advance_head(work_last);

    auto commits = collect_wip_commits(repo.repo(), wip1, work_last);
    ASSERT_TRUE(commits.has_value());
    ASSERT_EQ(commits->size(), 1u);
    EXPECT_TRUE(git_oid_equal(&(*commits)[0], &wip1));
}

// Three wip commits stacked, newest first
TEST(CollectWipCommits, ThreeWipCommits) {
    TestRepo repo;
    repo.write_file("f", "v1");
    git_oid work_last = repo.commit("initial");

    repo.write_file("f", "v2"); git_oid w1 = repo.commit("WIP 1");
    repo.write_file("f", "v3"); git_oid w2 = repo.commit("WIP 2");
    repo.write_file("f", "v4"); git_oid w3 = repo.commit("WIP 3");
    repo.advance_head(work_last);

    auto commits = collect_wip_commits(repo.repo(), w3, work_last);
    ASSERT_TRUE(commits.has_value());
    ASSERT_EQ(commits->size(), 3u);
    // Topological order: newest (w3) first
    EXPECT_TRUE(git_oid_equal(&(*commits)[0], &w3));
    EXPECT_TRUE(git_oid_equal(&(*commits)[1], &w2));
    EXPECT_TRUE(git_oid_equal(&(*commits)[2], &w1));
}

// Work branch has advanced past wip → 0 visible commits
TEST(CollectWipCommits, WorkBranchAdvanced) {
    TestRepo repo;

    repo.write_file("f", "v1");
    git_oid commit_a = repo.commit("commit A");

    repo.write_file("f", "wip");
    git_oid wip_last = repo.commit("WIP");

    // Now advance work branch past A (new commit B)
    repo.advance_head(commit_a);
    repo.write_file("f", "v2");
    git_oid commit_b = repo.commit("commit B");

    // wip_last's parent is A; work_last is now B (child of A)
    // merge_base(wip_last, commit_b) = A ≠ commit_b → 0 commits
    auto commits = collect_wip_commits(repo.repo(), wip_last, commit_b);
    ASSERT_TRUE(commits.has_value());
    EXPECT_EQ(commits->size(), 0u);
}

// wip_last and work_last are the same commit (no wip yet) → 0 commits
TEST(CollectWipCommits, WipSameAsWork) {
    TestRepo repo;
    repo.write_file("f", "v1");
    git_oid oid = repo.commit("initial");

    auto commits = collect_wip_commits(repo.repo(), oid, oid);
    ASSERT_TRUE(commits.has_value());
    EXPECT_EQ(commits->size(), 0u);
}
