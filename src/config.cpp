#include "config.hpp"
#include "spdlog/spdlog.h"

// ---------------------------------------------------------------------------
// Config implementation — generic libgit2 config wrapper
// ---------------------------------------------------------------------------

Config::Config(git_repository *repo) {
    if (git_repository_config(&m_cfg, repo) < 0) {
        spdlog::debug("config: cannot open git config");
        m_cfg = nullptr;
    }
}

Config::Config(Config &&other) noexcept : m_cfg(other.m_cfg) {
    other.m_cfg = nullptr;
}

Config &Config::operator=(Config &&other) noexcept {
    if (this != &other) {
        if (m_cfg) git_config_free(m_cfg);
        m_cfg = other.m_cfg;
        other.m_cfg = nullptr;
    }
    return *this;
}

Config::~Config() {
    if (m_cfg) git_config_free(m_cfg);
}

std::optional<std::string> Config::get_string(const char *key) const {
    if (!m_cfg) return std::nullopt;

    git_config_entry *entry = nullptr;
    int rc = git_config_get_entry(&entry, m_cfg, key);
    if (rc == 0 && entry && entry->value) {
        std::string value = entry->value;
        git_config_entry_free(entry);
        spdlog::debug("config: {} = '{}'", key, value);
        return value;
    }
    if (rc != GIT_ENOTFOUND) {
        spdlog::debug("config: error reading {}", key);
    }
    return std::nullopt;
}

std::optional<bool> Config::get_bool(const char *key) const {
    if (!m_cfg) return std::nullopt;

    int value = 0;
    int rc = git_config_get_bool(&value, m_cfg, key);
    if (rc == 0) {
        bool result = (value != 0);
        spdlog::debug("config: {} = {}", key, result);
        return result;
    }
    if (rc != GIT_ENOTFOUND) {
        spdlog::debug("config: error reading {}", key);
    }
    return std::nullopt;
}

std::optional<int> Config::get_int(const char *key) const {
    if (!m_cfg) return std::nullopt;

    int value = 0;
    int rc = git_config_get_int32(&value, m_cfg, key);
    if (rc == 0) {
        spdlog::debug("config: {} = {}", key, value);
        return value;
    }
    if (rc != GIT_ENOTFOUND) {
        spdlog::debug("config: error reading {}", key);
    }
    return std::nullopt;
}

std::optional<int64_t> Config::get_int64(const char *key) const {
    if (!m_cfg) return std::nullopt;

    int64_t value = 0;
    int rc = git_config_get_int64(&value, m_cfg, key);
    if (rc == 0) {
        spdlog::debug("config: {} = {}", key, value);
        return value;
    }
    if (rc != GIT_ENOTFOUND) {
        spdlog::debug("config: error reading {}", key);
    }
    return std::nullopt;
}
