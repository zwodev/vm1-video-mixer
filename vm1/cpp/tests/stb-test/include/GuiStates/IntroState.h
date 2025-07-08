#pragma once
#include "GuiStateBase.h"
#include "Gui.h"
#include "GuiStates.h"
#include "GuiStateMachine.h"

class IntroState : public GuiState
{
private:
public:
    IntroState(GuiStateMachine &machine, Settings &settings, Gui &gui, AppEventBus &appEventBus)
        : GuiState(machine, settings, gui, appEventBus)
    {
    }

    void handleInput(const char &key) override
    {
        switch (key)
        {
        case '-':
            appEventBus.publish(AppEvent::DecreaseFoo);
            break;
        case '+':
            appEventBus.publish(AppEvent::IncreaseFoo);
            break;
        default:
            break;
        }
    }
    void describeUI() override
    {
        gui.addLabel("INTRO(FOO)");
        gui.addSlider("Foo", settings.foo);
    }
};
