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

#include "ImageBuffer.h"

namespace fs = std::filesystem;
using Clock = std::chrono::steady_clock;

struct PreviewNode {
    bool isValid = false;
    ImageBuffer image;
    Clock::time_point loadedAt{};
    std::atomic<bool> loading{false};
    uint64_t version = 0;
};

class PreviewCache {
public:
    PreviewCache(size_t maxEntries = 30, std::chrono::seconds ttl = std::chrono::seconds(200));

    std::shared_ptr<PreviewNode> getEntry(const std::string& path);
    void invalidate(const std::string& path);
    bool isStale(const std::shared_ptr<PreviewNode>& node) const;

private:
    void touchLRU(std::list<std::string>::iterator it);
    void enforceCapacity();
    void startLoadIfNeeded(const std::string& path, std::shared_ptr<PreviewNode> node);
    void startAsyncLoad(const std::string& path, std::shared_ptr<PreviewNode> node);

private:
    struct MapEntry {
        std::shared_ptr<PreviewNode> node;
        std::list<std::string>::iterator lruIt;
    };

    std::unordered_map<std::string, MapEntry> m_map;
    std::list<std::string> m_lru;
    size_t m_maxEntries;
    std::chrono::seconds m_ttl;
    std::atomic<bool> m_loading{false};
    mutable std::shared_mutex m_mutex;
    std::future<void> m_future;
};