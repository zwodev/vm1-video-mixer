#pragma once

#include <functional>
#include <unordered_map>
#include <vector> 
#include <typeindex>
#include <memory>
#include <queue>
#include <mutex>

// Each event type is defined as struct.
// This way it can have several properties.
struct SystemEvent {
    enum Type {
        Restart,
        Exit,
        KeyDown
    };

    SystemEvent() = delete;
    SystemEvent(Type type) { this->type = type; }
    Type type;
};

struct HdmiCaptureInitEvent {
    HdmiCaptureInitEvent() = delete;
    HdmiCaptureInitEvent(std::string configString) { this->configString = configString; }
    std::string configString;
};

struct PlaybackEvent {
    enum Type {
        NoDisplay,
        NoMedia,
        FileNotSupported,
        InputNotReady
    };

    PlaybackEvent() = delete;
    PlaybackEvent(Type type, std::string message = std::string()) { this->type = type; this->message = message; }
    Type type;
    std::string message;
};

struct MediaSlotEvent 
{
    MediaSlotEvent() = delete;
    MediaSlotEvent(int slotId, bool triggerPlayback = true){
            this->slotId = slotId; 
            this->triggerPlayback = triggerPlayback;         
    }
    int slotId;
    bool triggerPlayback;
};

struct EditModeEvent
{
    EditModeEvent() = delete;
    EditModeEvent(int modeId) { this->modeId = modeId; }
    int modeId;
};

struct NavigationEvent {
    enum Type {
        FocusPrevious,
        FocusNext,
        SelectItem,
        DecreaseValue,
        IncreaseValue,
        HierarchyUp,
        HierarchyDown,
        BankUp,
        BankDown
    };

    NavigationEvent() = delete;
    NavigationEvent(Type type) { this->type = type; }
    Type type;
};

// Base struct for a subcriber list
struct ISubscriberList {
    virtual ~ISubscriberList() = default;
};

// Using a template here for all the different event types (defined as structs).
// This derives from ISubscriberList to hide the actual type.
template<typename T>
struct SubscriberList : ISubscriberList {
    std::vector<std::function<void(const T&)>> subscribers;
};

// Base class for queued events (type-erased)
struct IQueuedEvent {
    virtual ~IQueuedEvent() = default;
    virtual void dispatch(class EventBus& bus) = 0;
};

template<typename T>
struct QueuedEvent : IQueuedEvent {
    T event;
    explicit QueuedEvent(const T& e) : event(e) {}
    void dispatch(class EventBus& bus) override {
        bus.publish(event);
    }
};

class EventBus
{
public:
    // Subscribing to an event with std::function. Could be a lambda function for example.
    // Using std::move prevents the duplication of the whole function object.
    // Since the original one is not needed anymore, this is a more efficient way.
    template<typename T>
    void subscribe(std::function<void(const T&)> callback)
    {
        auto& list = getOrCreateSubscriberList<T>();
        list.subscribers.push_back(std::move(callback));
    } 

    // Publishing an event will check if there is a list available for the type (T).
    // If found, this list will be iterated and all subscribers "notified" by calling the function object.
    template<typename T>
    void publish(const T& event) const
    {
        auto index = std::type_index(typeid(T));
        auto iter = m_subscriberLists.find(index);
        if (iter != m_subscriberLists.end()) {
            auto* subscriberList = static_cast<SubscriberList<T>*>(iter->second.get());
            for (auto& subscriber : subscriberList->subscribers) {
                subscriber(event);
            }
        }
    }

    // Thread-safe queueing of an event
    template<typename T>
    void enqueue(const T& event) {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_eventQueue.push(std::make_unique<QueuedEvent<T>>(event));
    }

    // Called from main thread to process and dispatch queued events
    void processEvents() {
        std::queue<std::unique_ptr<IQueuedEvent>> localQueue;

        {   // Swap under lock
            std::lock_guard<std::mutex> lock(m_queueMutex);
            std::swap(localQueue, m_eventQueue);
        }

        // Dispatch all events
        while (!localQueue.empty()) {
            localQueue.front()->dispatch(*this);
            localQueue.pop();
        }
    }

private:
    template<typename T>
    SubscriberList<T>& getOrCreateSubscriberList()
    {
        auto index = std::type_index(typeid(T));
        if (m_subscriberLists.count(index) == 0) {
            m_subscriberLists[index] = std::make_unique<SubscriberList<T>>();
        }

        return *(static_cast<SubscriberList<T>*>(m_subscriberLists[index].get()));
    }  

private:
    std::unordered_map<std::type_index, std::unique_ptr<ISubscriberList>> m_subscriberLists;
    std::mutex m_queueMutex;
    std::queue<std::unique_ptr<IQueuedEvent>> m_eventQueue;
};