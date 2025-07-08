#pragma once
#include "GuiStateBase.h"
#include "Gui.h"
#include "GuiStates.h"
#include "GuiStateMachine.h"

namespace PlaybackStateMenu
{
    enum struct MainMenu
    {
        Looping,
        Speed,
        Direction,
        Trim,
        Count
    };
};

class PlaybackState : public GuiState,
                      public SelectableMenu<PlaybackStateMenu::MainMenu>
{
private:
    static constexpr std::array<const char *, static_cast<size_t>(PlaybackStateMenu::MainMenu::Count)> menuLabels = {
        "Looping",
        "Speed",
        "Direction",
        "Trim"};

public:
    PlaybackState(GuiStateMachine &machine,
                  Settings &settings,
                  Gui &gui,
                  AppEventBus &appEventBus)
        : GuiState(machine, settings, gui, appEventBus),
          SelectableMenu(std::span{menuLabels})
    {
    }

    void handleInput(const char &key) override
    {
        switch (key)
        {
        case '-':
            appEventBus.publish(AppEvent::FocusNext);
            break;
        case '+':
            appEventBus.publish(AppEvent::FocusPrevious);
            break;
        default:
            break;
        }
    }

    void describeUI() override
    {
        gui.addLabel("PLAYBACK");
        gui.addMenu(menuLabels, getFocusedIndex());
    }

    void handleSelectedMenuItem() override
    {
        switch (focused)
        {
        case PlaybackStateMenu::MainMenu::Looping:
            appEventBus.publish(AppEvent::ToggleLooping);
            break;
        default:
            break;
        }
    }
};