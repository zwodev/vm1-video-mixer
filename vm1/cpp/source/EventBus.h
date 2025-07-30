#pragma once

#include <functional>
#include <unordered_map>
#include <vector> 
#include <typeindex>
#include <memory>

// Each event type is defined as struct.
// This way it can have several properties.
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
};