#include "AppEventHandler.h"
#include "GuiStates.h"

AppEventHandler::AppEventHandler(Settings &settings,
                                 Gui &gui,
                                 GuiStateMachine &stateMachine) : settings(settings),
                                                                  gui(gui),
                                                                  stateMachine(stateMachine)
{
}

void AppEventHandler::handle(AppEvent event)
{
    switch (event)
    {
    case AppEvent::IncreaseBar:
    {
        settings.bar += 1;
        gui.requestRedraw();
        break;
    }
    case AppEvent::DecreaseBar:
    {
        settings.bar -= 1;
        gui.requestRedraw();
        break;
    }
    case AppEvent::IncreaseFoo:
    {
        settings.foo += 0.1f;
        gui.requestRedraw();
        break;
    }
    case AppEvent::DecreaseFoo:
    {
        settings.foo -= 0.1f;
        gui.requestRedraw();
        break;
    }
    case AppEvent::FocusNext:
    {
        auto *sel = dynamic_cast<ISelectableMenu *>(stateMachine.getCurrentState());
        if (sel)
        {
            sel->focusNext();
            gui.requestRedraw();
        }
        break;
    }
    case AppEvent::FocusPrevious:
    {
        auto *sel = dynamic_cast<ISelectableMenu *>(stateMachine.getCurrentState());
        if (sel)
        {
            sel->focusPrevious();
            gui.requestRedraw();
        }
        break;
    }
    case AppEvent::SelectMenuItem:
    {
        auto *sel = dynamic_cast<ISelectableMenu *>(stateMachine.getCurrentState());
        if (sel)
        {
            sel->handleSelectedMenuItem();
            gui.requestRedraw();
        }
        break;
    }

    case AppEvent::ToggleLooping:
    {
        std::cout << "Toggling Looping" << std::endl;
    }
    }
}
