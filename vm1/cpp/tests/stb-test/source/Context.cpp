#include "Context.h"
#include "Settings.h"
#include "StbRenderer.h"

#include <chrono>
#include <thread>

Context::Context(Settings &settings)
    : settings(settings),
      introState(stateMachine, settings, gui, appEventBus),
      infoState(stateMachine, settings, gui, appEventBus),
      sourceState(stateMachine, settings, gui, appEventBus),
      fileSelectionState(stateMachine, settings, gui, appEventBus),
      valueState(stateMachine, settings, gui, appEventBus),
      playbackState(stateMachine, settings, gui, appEventBus),
      appEventHandler(settings, gui, stateMachine),
      guiRenderer(128, 128)
{
    // subscribe to STATE CHANGES
    appEventBus.subscribe(AppEvent::GoToParentMenu, [this]()
                          { 
                            std::cout << "going to parent menu" << std::endl;
                            if(stateMachine.getCurrentState()->getParentMenu()) {
                                stateMachine.setState(stateMachine.getCurrentState()->getParentMenu()); 
                                stateMachine.getCurrentState()->setSubmenu(nullptr);
                                gui.requestRedraw(); 
                            } });

    appEventBus.subscribe(AppEvent::GoToIntroState, [this]()
                          { 
                            stateMachine.setState(&introState); 
                            gui.requestRedraw(); });

    appEventBus.subscribe(AppEvent::GoToInfoState, [this]()
                          { 
                            stateMachine.setState(&infoState); 
                            gui.requestRedraw(); });

    appEventBus.subscribe(AppEvent::GoToPlaybackState, [this]()
                          { 
                            stateMachine.setState(&playbackState); 
                            gui.requestRedraw(); });

    appEventBus.subscribe(AppEvent::GoToValueState, [this]()
                          { 
                            stateMachine.setState(&valueState);
                            gui.requestRedraw(); });

    // handlers for MENUS
    appEventBus.subscribe(AppEvent::FocusNext, [this]()
                          { appEventHandler.handle(AppEvent::FocusNext); });

    appEventBus.subscribe(AppEvent::FocusPrevious, [this]()
                          { appEventHandler.handle(AppEvent::FocusPrevious); });

    appEventBus.subscribe(AppEvent::SelectMenuItem, [this]()
                          { appEventHandler.handle(AppEvent::SelectMenuItem); });

    // handlers for state SOURCE
    appEventBus.subscribe(AppEvent::GoToSourceState, [this]()
                          { 
                            if (sourceState.getSubmenu() == nullptr)
                            {
                                stateMachine.setState(&sourceState); 
                            }
                            else
                            {
                                stateMachine.setState(sourceState.getSubmenu());
                            }
                            gui.requestRedraw(); });

    appEventBus.subscribe(AppEvent::GoToFileSelectionState, [this]()
                          { sourceState.setSubmenu(&fileSelectionState);
                            fileSelectionState.setParentMenu(&sourceState);
                            fileSelectionState.onEnter();
                            stateMachine.setState(&fileSelectionState);
                        gui.requestRedraw(); });

    // handlers for state PLAYBACK
    appEventBus.subscribe(AppEvent::ToggleLooping, [this]()
                          { appEventHandler.handle(AppEvent::ToggleLooping); });

    // subscribe to ALL OTHER EVENTS that change variables etc
    appEventBus.subscribe(AppEvent::IncreaseBar, [this]()
                          { appEventHandler.handle(AppEvent::IncreaseBar); });

    appEventBus.subscribe(AppEvent::DecreaseBar, [this]()
                          { appEventHandler.handle(AppEvent::DecreaseBar); });

    appEventBus.subscribe(AppEvent::IncreaseFoo, [this]()
                          { appEventHandler.handle(AppEvent::IncreaseFoo); });

    appEventBus.subscribe(AppEvent::DecreaseFoo, [this]()
                          { appEventHandler.handle(AppEvent::DecreaseFoo); });
}

Context::~Context()
{
    std::cout << "Closing Context" << std::endl;
}

void Context::run()
{
    isRunning = true;

    while (isRunning)
    {
        if (!stateMachine.getCurrentState())
            stateMachine.setState(&introState);

        // check input
        char key = keyboardController.getKeyPress();
        if (key)
            std::cout << "\nKey pressed: " << key << "\n";

        // We switch the keys that are same for each state.
        // Individual keys are managed by each state separateley.
        switch (key)
        {
        case 'x': // quit on 'x'
            isRunning = false;
            continue;
        case '0': // not possible on VM-1, switching to Intro/Startup-Screen
            appEventBus.publish(AppEvent::GoToIntroState);
            break;
        case '1':
            appEventBus.publish(AppEvent::GoToInfoState);
            break;
        case '2':
            appEventBus.publish(AppEvent::GoToSourceState);
            break;
        case '3':
            appEventBus.publish(AppEvent::GoToPlaybackState);
            break;
        case '4':
            appEventBus.publish(AppEvent::GoToValueState);
            break;
        case '\n': // enter a menu with 'return'
            appEventBus.publish(AppEvent::SelectMenuItem);
            break;
        case 'b': // go back one menu with 'b'
            appEventBus.publish(AppEvent::GoToParentMenu);
            break;
        default:
            stateMachine.handleInput(key);
            break;
        }

        // here, some other things like a progress bar or an animation could
        // also call gui.requestRedraw and force a redraw

        if (gui.shouldRedraw())
        {
            std::cout << "Rendering" << std::endl;

            // Debug Output BEGIN
            // ------------------
            std::cout << "Current State: ";
            if (stateMachine.getCurrentState() == &introState)
            {
                std::cout << "Intro State" << std::endl;
            }
            else if (stateMachine.getCurrentState() == &valueState)
            {
                std::cout << "Value State" << std::endl;
            }

            std::cout << "Settings: "
                      << "foo: " << settings.foo
                      << " bar: " << settings.bar << std::endl;
            // ----------------
            // Debug Output END

            // update GUI from settings and let the gui create a scenegraph
            gui.clearSceneGraph();
            stateMachine.getCurrentState()->describeUI();

            // let the GUI RENDERER take the scenegraph and render it
            guiRenderer.clear();
            for (std::unique_ptr<GuiItem> &item : gui.sceneGraph)
            {
                if (item)
                    item->render(guiRenderer);
            }
            guiRenderer.getStbRenderer()->savePNG("output.png");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}
