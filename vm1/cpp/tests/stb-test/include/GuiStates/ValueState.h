#pragma once
#include "GuiStateBase.h"
#include "Gui.h"
#include "GuiStates.h"
#include "GuiStateMachine.h"

class ValueState : public GuiState
{
private:
public:
    ValueState(GuiStateMachine &machine, Settings &settings, Gui &gui, AppEventBus &appEventBus)
        : GuiState(machine, settings, gui, appEventBus)
    {
    }
    void handleInput(const char &key) override
    {
        switch (key)
        {
        case '-':
            appEventBus.publish(AppEvent::DecreaseBar);
            break;
        case '+':
            appEventBus.publish(AppEvent::IncreaseBar);
            break;
        default:
            break;
        }
    }

    void describeUI() override
    {
        gui.addLabel("VALUE(BAR)");
        gui.addSlider("Bar", settings.bar);
    }
};
