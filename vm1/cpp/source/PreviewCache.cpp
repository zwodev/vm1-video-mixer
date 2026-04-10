/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "PreviewCache.h"

PreviewCache::PreviewCache(size_t maxEntries, std::chrono::seconds ttl)
    : m_maxEntries(maxEntries), m_ttl(ttl) 
{
}

std::shared_ptr<PreviewNode> PreviewCache::getEntry(const std::string& path) {
    std::unique_lock lock(m_mutex);
    MapEntry& entry = m_map[path];
    if (!entry.node) {
        entry.node = std::make_shared<PreviewNode>();
        entry.lruIt = m_lru.insert(m_lru.begin(), path);
        enforceCapacity();
    } else {
        touchLRU(entry.lruIt);
    }

    if (!entry.node->loading.load() && isStale(entry.node)) startAsyncLoad(path, entry.node);
    
    return entry.node;
}

// ImageBuffer* PreviewCache::lock(const std::string& path) {
//     std::unique_lock lock(m_mutex);
//     MapEntry& entry = m_map[path];
//     if (!entry.node) {
//         entry.node = std::make_shared<PreviewNode>();
//         entry.lruIt = m_lru.insert(m_lru.begin(), path);
//         enforceCapacity();
//     } else {
//         touchLRU(entry.lruIt);
//     }

//     if (!entry.node->loading.load() || isStale(entry.node)) {
//         startAsyncLoad(path, entry.node);
//         return nullptr;
//     }
    
//     entry.node->locked.store(true);
//     return &(entry.node->image);
// }

// ImageBuffer* PreviewCache::unlock(const std::string& path) {
//     std::unique_lock lock(m_mutex);
//     MapEntry& entry = m_map[path];
//     if (entry.node) {
//         if
//     }
// }

void PreviewCache::invalidate(const std::string& path) {
    std::unique_lock lock(m_mutex);
    auto it = m_map.find(path);
    if (it != m_map.end()) {
        it->second.node->loadedAt = Clock::time_point{};
        it->second.node->version++;
    }
}

bool PreviewCache::isStale(const std::shared_ptr<PreviewNode>& node) const {
    if (!node->loadedAt.time_since_epoch().count()) return true;
    return Clock::now() - node->loadedAt > m_ttl;
}

void PreviewCache::touchLRU(std::list<std::string>::iterator it) {
    m_lru.splice(m_lru.begin(), m_lru, it);
}

void PreviewCache::enforceCapacity() {
    while (m_map.size() > m_maxEntries) {
        auto last = m_lru.end(); --last;
        m_map.erase(*last);
        m_lru.erase(last);
    }
}

void PreviewCache::startLoadIfNeeded(const std::string& path, std::shared_ptr<PreviewNode> node) {
    if (!node->loading.load()) {
        startAsyncLoad(path, node);
    }
}

void PreviewCache::startAsyncLoad(const std::string& path, std::shared_ptr<PreviewNode> node) {
    if (m_loading.load()) return;
    m_loading.store(true);
    node->loading.store(true);

    // Load directory content in separate thread
    std::weak_ptr<PreviewNode> weakNode = node;
    std::string p = path;
    
    m_future = std::async(std::launch::async, [this, p, weakNode]() {
        ImageBuffer imageBuffer(p);
        if (!imageBuffer.isValid) return;

        if (auto node = weakNode.lock()) {
            node->image = std::move(imageBuffer);
            node->loadedAt = Clock::now();
            node->loading.store(false);
            node->version++;
        }
        m_loading.store(false);
    });
}