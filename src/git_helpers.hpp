#pragma once

// git_helpers.hpp — thin wrappers around libgit2 operations that are repeated
// across multiple commands.  All functions return by value and report errors
// via std::expected-style: an empty optional/string signals failure, and the
// caller can retrieve the last libgit2 error with git_error_str().

#include "git_guards.hpp"
#include "string_helpers.hpp"

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// oid_to_hex  — convert a git_oid to its full lowercase hex string
// ---------------------------------------------------------------------------
inline std::string oid_to_hex(const git_oid *oid) {
    char buf[GIT_OID_MAX_HEXSIZE + 1];
    git_oid_tostr(buf, sizeof(buf), oid);
    return buf;
}

// ---------------------------------------------------------------------------
// oid_to_short_hex  — first 7 characters of the hex OID
// ---------------------------------------------------------------------------
inline std::string oid_to_short_hex(const git_oid *oid) {
    char buf[GIT_OID_MAX_HEXSIZE + 1];
    git_oid_tostr(buf, sizeof(buf), oid);
    buf[7] = '\0';
    return buf;
}

// ---------------------------------------------------------------------------
// BranchNames
//
// Holds the three names every command needs after resolving the current branch:
//   work_branch  — short name, e.g. "master"
//   work_ref     — full ref,   e.g. "refs/heads/master"
//   wip_ref      — full ref,   e.g. "refs/wip/master"
// ---------------------------------------------------------------------------
struct BranchNames {
    std::string work_branch; // short, e.g. "master"
    std::string work_ref;    // e.g. "refs/heads/master"
    std::string wip_ref;     // e.g. "refs/wip/master"
};

// ---------------------------------------------------------------------------
// resolve_branch_names
//
// Read HEAD from `repo` and derive BranchNames.
// Returns std::nullopt if HEAD is unborn or detached (not on a local branch).
// ---------------------------------------------------------------------------
inline std::optional<BranchNames> resolve_branch_names(git_repository *repo) {
    ReferenceGuard head_ref;
    if (git_repository_head(head_ref.ptr(), repo) < 0)
        return std::nullopt;
    if (!git_reference_is_branch(head_ref.get()))
        return std::nullopt;

    BranchNames bn;
    bn.work_ref    = git_reference_name(head_ref.get()); // e.g. "refs/heads/master"
    bn.work_branch = strip_prefix(bn.work_ref, "refs/heads/");
    bn.wip_ref     = "refs/wip/" + bn.work_branch;
    return bn;
}

// ---------------------------------------------------------------------------
// resolve_oid
//
// Look up the OID for a named ref.  Returns std::nullopt on failure.
// ---------------------------------------------------------------------------
inline std::optional<git_oid> resolve_oid(git_repository *repo,
                                          const std::string &ref_name) {
    git_oid oid{};
    if (git_reference_name_to_id(&oid, repo, ref_name.c_str()) < 0)
        return std::nullopt;
    return oid;
}

// ---------------------------------------------------------------------------
// ensure_reflog_dir
//
// Create the directory tree and empty file needed for libgit2 to maintain a
// reflog for `wip_ref` (e.g. "refs/wip/master").
// Silently ignores errors — a missing reflog is not fatal.
// ---------------------------------------------------------------------------
inline void ensure_reflog_dir(git_repository *repo, const std::string &wip_ref) {
    const char *git_dir = git_repository_path(repo); // ends with "/"
    std::filesystem::path reflog_path =
        std::filesystem::path(git_dir) / "logs" / wip_ref;
    std::error_code ec;
    std::filesystem::create_directories(reflog_path.parent_path(), ec);
    if (!ec && !std::filesystem::exists(reflog_path, ec))
        std::ofstream{reflog_path, std::ios::app}; // touch
}
