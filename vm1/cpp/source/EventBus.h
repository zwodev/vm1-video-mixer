#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

enum class Event
{
    GoToIntroState,
    GoToInfoState,
    GoToSourceState,
    GoToPlaybackState,
    GoToValueState,
 
    FocusNext,
    FocusPrevious,
    SelectItem,
    IncreaseValue,
    DecreaseValue,
    GoToParentMenu,

    ActivateMediaSlot,
};

class EventBus
{
public:
    using Handler = std::function<void()>;

    void subscribe(Event e, Handler h)
    {
        handlers[e].push_back(h);
    }

    void publish(Event e)
    {
        for (Handler &h : handlers[e])
            h();
    }

private:
    std::unordered_map<Event, std::vector<Handler>> handlers;
};