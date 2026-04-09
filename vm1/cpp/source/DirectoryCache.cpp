/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "DirectoryCache.h"

#include <algorithm>

DirectoryCache::DirectoryCache(size_t maxEntries, std::chrono::seconds ttl)
    : m_maxEntries(maxEntries), m_ttl(ttl) 
{
}

std::vector<DirectoryEntry> DirectoryCache::getEntries(const std::string& path) {
    std::unique_lock lock(m_mutex);
    MapEntry& entry = m_map[path];
    if (!entry.node) {
        entry.node = std::make_shared<DirectoryNode>();
        entry.lruIt = m_lru.insert(m_lru.begin(), path);
        enforceCapacity();
    } else {
        touchLRU(entry.lruIt);
    }
    auto node = entry.node;
    if (!node->loading.load() || isStale(node)) startAsyncLoad(path, node);
    return node->entries;
}

void DirectoryCache::ensureLoaded(const std::string& path) {
    std::unique_lock lock(m_mutex);
    MapEntry& entry = m_map[path];
    if (!entry.node) {
        entry.node = std::make_shared<DirectoryNode>();
        entry.lruIt = m_lru.insert(m_lru.begin(), path);
        enforceCapacity();
    } else {
        touchLRU(entry.lruIt);
    }
    auto node = entry.node;
    if (!node->loading.load()) startAsyncLoad(path, node);
}

void DirectoryCache::invalidate(const std::string& path) {
    std::unique_lock lock(m_mutex);
    auto it = m_map.find(path);
    if (it != m_map.end()) {
        it->second.node->loadedAt = Clock::time_point{};
        it->second.node->version++;
    }
}

bool DirectoryCache::isStale(const std::shared_ptr<DirectoryNode>& node) const {
    if (!node->loadedAt.time_since_epoch().count()) return true;
    return Clock::now() - node->loadedAt > m_ttl;
}

void DirectoryCache::touchLRU(std::list<std::string>::iterator it) {
    m_lru.splice(m_lru.begin(), m_lru, it);
}

void DirectoryCache::enforceCapacity() {
    while (m_map.size() > m_maxEntries) {
        auto last = m_lru.end(); --last;
        m_map.erase(*last);
        m_lru.erase(last);
    }
}

void DirectoryCache::startLoadIfNeeded(const std::string& path, std::shared_ptr<DirectoryNode> node) {
    if (!node->loading.load()) {
        startAsyncLoad(path, node);
    }
}

void DirectoryCache::startAsyncLoad(const std::string& path, std::shared_ptr<DirectoryNode> node) {
    node->loading.store(true);

    // Load directory content in separate thread
    std::weak_ptr<DirectoryNode> weakNode = node;
    std::string p = path;
    m_future = std::async(std::launch::async, [this, p, weakNode]() {
        std::vector<DirectoryEntry> entries;
        std::vector<DirectoryEntry> fileEntries;
        
        try {
            for (auto &it : fs::directory_iterator(p)) {
                std::string fileName = it.path().filename().string();
                if (fileName.ends_with(".preview")) continue;
                DirectoryEntry entry;
                entry.name = fileName;
                entry.absolutePath = it.path().string();
                entry.isDir = it.is_directory();
                if (!entry.isDir) {
                    std::error_code errorCode;
                    entry.size = fs::file_size(it.path(), errorCode);
                    auto ftime = fs::last_write_time(it.path(), errorCode);
                    if (!errorCode) {
                        entry.mtime = std::chrono::duration_cast<std::chrono::seconds>(ftime.time_since_epoch()).count();
                    }
                    fileEntries.push_back(std::move(entry));
                }
                else {
                    entries.push_back(std::move(entry));
                }
                
            }
        } catch (...) {
            // No entry if error occurs.
        }
        if (auto node = weakNode.lock()) {
            std::sort(entries.begin(), entries.end(), [](const DirectoryEntry& a, const DirectoryEntry b){ return a.name > b.name; });
            std::sort(fileEntries.begin(), fileEntries.end(), [](const DirectoryEntry& a, const DirectoryEntry b){ return a.name > b.name; });
            std::move(fileEntries.begin(), fileEntries.end(), back_inserter(entries));
            node->entries = std::move(entries);
            node->loadedAt = Clock::now();
            node->loading.store(false);
            node->version++;
        }
    });
}