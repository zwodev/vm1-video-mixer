#pragma once
#include "GuiStateBase.h"
#include "Gui.h"
#include "GuiStates.h"
#include "GuiStateMachine.h"

class InfoState : public GuiState
{
private:
public:
    InfoState(GuiStateMachine &machine, Settings &settings, Gui &gui, AppEventBus &appEventBus)
        : GuiState(machine, settings, gui, appEventBus)
    {
    }

    void handleInput(const char &key) override
    {
        switch (key)
        {
        case '-':
            break;
        case '+':
            break;
        default:
            break;
        }
    }
    void describeUI() override
    {
        gui.addLabel("INFO");
    }
};
