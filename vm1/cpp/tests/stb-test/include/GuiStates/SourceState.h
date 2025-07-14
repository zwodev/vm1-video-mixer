#pragma once
#include <span>
#include "GuiStateBase.h"
#include "Gui.h"
#include "GuiStates.h"
#include "GuiStateMachine.h"

namespace SourceStateMenu
{
    enum struct MainMenu
    {
        HDMI,
        File,
        Shader,
        Count
    };
};

class SourceState : public GuiState,
                    public SelectableMenu<SourceStateMenu::MainMenu>
{
private:
    static constexpr std::array<const char *, static_cast<size_t>(SourceStateMenu::MainMenu::Count)> menuLabels = {
        "HDMI",
        "File",
        "Shader"};

    static constexpr std::array<const char *, static_cast<size_t>(SourceStateMenu::MainMenu::Count)> menuEntriesPngFilenames = {
        "resources/source-hdmi.png",
        "resources/source-file.png",
        "resources/source-shader.png"};

public:
    SourceState(GuiStateMachine &machine,
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
        // gui.addLabel("Source q 1");
        // gui.addMenu(menuEntriesPngFilenames, getFocusedIndex());
        std::string filename = menuEntriesPngFilenames[getFocusedIndex()];
        gui.addPng(filename);
    }

    void handleSelectedMenuItem() override
    {
        switch (focused)
        {
        case SourceStateMenu::MainMenu::File:
            appEventBus.publish(AppEvent::GoToFileSelectionState);
            break;
        default:
            break;
        }
    }
};
