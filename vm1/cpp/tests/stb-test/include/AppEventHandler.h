#pragma once
#include "Settings.h"
#include "Gui.h"
#include "GuiStateMachine.h"
#include "AppEventBus.h"

class AppEventHandler
{
private:
    Settings &settings;
    Gui &gui;
    GuiStateMachine &stateMachine;

public:
    AppEventHandler(Settings &settings,
                    Gui &Gui,
                    GuiStateMachine &stateMachine);

    void handle(AppEvent event);
};