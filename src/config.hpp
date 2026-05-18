#pragma once

#include <git2.h>
#include <optional>
#include <string>

// ---------------------------------------------------------------------------
// Config — generic wrapper around libgit2's git config API
//
// Provides typed accessors for reading configuration values from .gitconfig.
// Commands use this to read their specific settings (e.g., git-wip.save).
//
// Example usage:
//
//   Config cfg(repo);
//   auto save_spec = cfg.get_string("git-wip.save");
//   auto gpg_sign = cfg.get_bool("git-wip.gpg-sign");
//
// ---------------------------------------------------------------------------

class Config {
public:
    // Construct from an open repository; opens the merged config view.
    explicit Config(git_repository *repo);

    // Non-copyable, movable
    Config(const Config &) = delete;
    Config &operator=(const Config &) = delete;
    Config(Config &&other) noexcept;
    Config &operator=(Config &&other) noexcept;

    ~Config();

    // Returns true if the config was successfully opened
    bool valid() const { return m_cfg != nullptr; }

    // Get a string value; returns nullopt if not found or on error
    std::optional<std::string> get_string(const char *key) const;

    // Get a boolean value; returns nullopt if not found or on error
    std::optional<bool> get_bool(const char *key) const;

    // Get an integer value; returns nullopt if not found or on error
    std::optional<int> get_int(const char *key) const;

    // Get an int64 value; returns nullopt if not found or on error
    std::optional<int64_t> get_int64(const char *key) const;

private:
    git_config *m_cfg = nullptr;
};
