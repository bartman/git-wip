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

struct GitLibGuard {
    GitLibGuard() { git_libgit2_init(); }
    ~GitLibGuard() { git_libgit2_shutdown(); }

    GitLibGuard(const GitLibGuard &) = delete;
    GitLibGuard &operator=(const GitLibGuard &) = delete;
};

struct GitRepoGuard {
    git_repository *m_repo = nullptr;
    ~GitRepoGuard() { if (m_repo) git_repository_free(m_repo); }
    git_repository       *get()       { return m_repo; }
    git_repository const *get() const { return m_repo; }
    git_repository      **ptr()       { return &m_repo; }
};

struct GitIndexGuard {
    git_index *m_idx = nullptr;
    ~GitIndexGuard() { if (m_idx) git_index_free(m_idx); }
    git_index       *get()       { return m_idx; }
    git_index const *get() const { return m_idx; }
    git_index      **ptr()       { return &m_idx; }
};

struct GitTreeGuard {
    git_tree *m_tree = nullptr;
    ~GitTreeGuard() { if (m_tree) git_tree_free(m_tree); }
    git_tree       *get()       { return m_tree; }
    git_tree const *get() const { return m_tree; }
    git_tree      **ptr()       { return &m_tree; }
};

struct GitCommitGuard {
    git_commit *m_commit = nullptr;
    ~GitCommitGuard() { if (m_commit) git_commit_free(m_commit); }
    git_commit       *get()       { return m_commit; }
    git_commit const *get() const { return m_commit; }
    git_commit      **ptr()       { return &m_commit; }
};

struct GitReferenceGuard {
    git_reference *m_ref = nullptr;
    ~GitReferenceGuard() { if (m_ref) git_reference_free(m_ref); }
    git_reference       *get()       { return m_ref; }
    git_reference const *get() const { return m_ref; }
    git_reference      **ptr()       { return &m_ref; }
};

struct GitSignatureGuard {
    git_signature *m_sig = nullptr;
    ~GitSignatureGuard() { if (m_sig) git_signature_free(m_sig); }
    git_signature       *get()       { return m_sig; }
    git_signature const *get() const { return m_sig; }
    git_signature      **ptr()       { return &m_sig; }
};

struct GitRevwalkGuard {
    git_revwalk *m_walk = nullptr;
    ~GitRevwalkGuard() { if (m_walk) git_revwalk_free(m_walk); }
    git_revwalk       *get()       { return m_walk; }
    git_revwalk const *get() const { return m_walk; }
    git_revwalk      **ptr()       { return &m_walk; }
};

// ---------------------------------------------------------------------------
// Convenience helper: return the last libgit2 error message as a std::string.
// ---------------------------------------------------------------------------
inline std::string git_error_str() {
    const git_error *e = git_error_last();
    return e ? e->message : "(unknown error)";
}
