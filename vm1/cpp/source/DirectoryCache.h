/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <shared_mutex>
#include <chrono>
#include <future>
#include <atomic>
#include <optional>

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

struct DirectoryEntry {
    std::string name;
    bool isDir = false;
    uint64_t size = 0;
    std::uint64_t mtime = 0;
};

struct DirectoryNode {
    bool isValid = false;
    std::vector<DirectoryEntry> entries;
    Clock::time_point loadedAt{};
    std::atomic<bool> loading{false};
    uint64_t version = 0;
};

class DirectoryCache {
public:
    DirectoryCache(size_t maxEntries = 256, std::chrono::seconds ttl = std::chrono::seconds(5));

    std::vector<DirectoryEntry> getEntries(const std::string& path);
    void ensureLoaded(const std::string& path);
    void invalidate(const std::string& path);
    bool isStale(const std::shared_ptr<DirectoryNode>& node) const;

private:
    void touchLRU(std::list<std::string>::iterator it);
    void enforceCapacity();
    void startLoadIfNeeded(const std::string& path, std::shared_ptr<DirectoryNode> node);
    void startAsyncLoad(const std::string& path, std::shared_ptr<DirectoryNode> node);

private:
    struct MapEntry {
        std::shared_ptr<DirectoryNode> node;
        std::list<std::string>::iterator lruIt;
    };

    std::unordered_map<std::string, MapEntry> m_map;
    std::list<std::string> m_lru;
    size_t m_maxEntries;
    std::chrono::seconds m_ttl;
    mutable std::shared_mutex m_mutex;
    std::future<void> m_future;
};