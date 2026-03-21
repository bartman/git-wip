#pragma once

#include <git2.h>
#include <string>

// ---------------------------------------------------------------------------
// RAII wrappers for libgit2 objects
//
// Each guard owns the pointed-to object and frees it on destruction.
// Use ptr() to obtain the address to pass to libgit2 "out" parameters,
// and get() to retrieve the wrapped pointer for subsequent API calls.
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

struct RevwalkGuard {
    git_revwalk *m_walk = nullptr;
    ~RevwalkGuard() {
        if (m_walk)
            git_revwalk_free(m_walk);
    }
    git_revwalk *get() { return m_walk; }
    git_revwalk **ptr() { return &m_walk; }
};

// ---------------------------------------------------------------------------
// Convenience helper: return the last libgit2 error message as a std::string.
// ---------------------------------------------------------------------------
inline std::string git_error_str() {
    const git_error *e = git_error_last();
    return e ? e->message : "(unknown error)";
}
