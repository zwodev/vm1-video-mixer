#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

template<typename T>
class ThreadableQueue
{
public:
    ThreadableQueue() = default;
    ~ThreadableQueue() = default;

public:
    void setActive(bool active) {
        m_isActive = active;
        m_frameCV.notify_all();
    }

    void clearFrames() {
        std::unique_lock<std::mutex> lock(m_frameMutex);
        while (!m_frameQueue.empty()) {
            m_frameQueue.pop();
        }
    }

    void pushFrame(T& frame) {
        std::unique_lock<std::mutex> lock(m_frameMutex);
        m_frameCV.wait(lock, [this]() { 
            return (m_frameQueue.size() < MAX_QUEUE_SIZE || !m_isActive); 
        });
        
        if (m_isActive) m_frameQueue.push(frame);
        m_frameCV.notify_one();
    }

    bool popFrame(T& frame) {
        std::unique_lock<std::mutex> lock(m_frameMutex);
        if (m_frameQueue.empty()) {
            return false;
        }
        
        frame = m_frameQueue.front();
        m_frameQueue.pop();
        m_frameCV.notify_one();
        return true;
    }

    bool peekFrame(T& frame) {
        std::unique_lock<std::mutex> lock(m_frameMutex);
        if (m_frameQueue.empty()) {
            return false;
        }
        
        frame = m_frameQueue.front();
        return true;
    }

private:
    static constexpr size_t MAX_QUEUE_SIZE = 3;
    std::atomic<bool> m_isActive = true;
    std::mutex m_frameMutex;
    std::condition_variable m_frameCV;
    std::queue<T> m_frameQueue;
};