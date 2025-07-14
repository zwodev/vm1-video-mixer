#pragma once

#include <memory>
#include "Gui.h"
#include "KeyboardController.h"
#include "GuiStateMachine.h"
#include "GuiStates.h"
#include "GuiRenderer.h"
#include "AppEventBus.h"
#include "AppEventHandler.h"

// class StbRenderer;
class Settings;

/**
 * @class Context
 * @brief Manages the main application context, including GUI, input handling, rendering, and state management.
 *
 * The Context class encapsulates the core components and logic required to run the application.
 * It manages the GUI, keyboard input, rendering, application settings, and state transitions.
 * The main application loop is controlled via the run() method.
 *
 * @note This class owns the renderer and manages the lifetime of its components.
 *
 * @todo
 * - event system for keyboardController
 */
class Context
{
private:
    bool isRunning;
    Settings &settings;

    Gui gui;
    AppEventBus appEventBus;
    AppEventHandler appEventHandler;
    KeyboardController keyboardController;
    GuiRenderer guiRenderer;

    GuiStateMachine stateMachine;
    IntroState introState;
    InfoState infoState;
    SourceState sourceState;
    FileSelectionState fileSelectionState;
    PlaybackState playbackState;
    ValueState valueState;

public:
    Context(Settings &settings);
    ~Context();

    void run();
};