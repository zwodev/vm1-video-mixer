#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

enum class AppEvent
{
    Redraw,

    // switch state events:
    GoToIntroState,
    GoToInfoState,
    GoToSourceState,
    GoToPlaybackState,
    GoToValueState,

    // events shared by all menus
    // or, in other words:
    // the four arrow key functions
    FocusNext,
    FocusPrevious,
    SelectMenuItem,
    GoToParentMenu,

    // source menu
    GoToFileSelectionState,

    // playback menu
    ToggleLooping,
    SetSpeed,
    SetDirection,
    SetTrim,

    // state specific events:
    IncreaseFoo,
    DecreaseFoo,

    IncreaseBar,
    DecreaseBar,
};

// Maybe I have to sort things out later, e.g. like:
// -------------------------------------------------
// namespace AppEvents
// {
//     enum class GoToState
//     {
//         Intro,
//         Value
//     };
//     enum class StateIntro
//     {
//         IncreaseFoo,
//         DecreaseFoo
//     };
//     enum class StateValue
//     {
//         IncreaseBar,
//         DecreaseBar
//     };
// }

class AppEventBus
{
public:
    using Handler = std::function<void()>;

    void subscribe(AppEvent e, Handler h)
    {
        handlers[e].push_back(h);
    }
    void publish(AppEvent e)
    {
        for (Handler &h : handlers[e])
            h();
    }

private:
    std::unordered_map<AppEvent, std::vector<Handler>> handlers;
};