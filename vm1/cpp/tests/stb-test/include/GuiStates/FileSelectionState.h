#pragma once
#include "GuiStateBase.h"
#include "Gui.h"
#include "GuiStates.h"
#include "GuiStateMachine.h"

class FileSelectionState : public GuiState,
                           public SelectableMenuDynamic
{
private:
    std::vector<std::string> loadedFiles;

public:
    FileSelectionState(GuiStateMachine &machine,
                       Settings &settings,
                       Gui &gui,
                       AppEventBus &appEventBus)
        : GuiState(machine, settings, gui, appEventBus),
          SelectableMenuDynamic(loadedFiles)
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
        gui.addLabel("FILES");
        gui.addMenu(loadedFiles, getFocusedIndex(), -1, true);
    }

    void onEnter()
    {
        // ToDo: Load the files from the directory
        loadedFiles.clear();
        loadedFiles.push_back("file-q-01.mov");
        loadedFiles.push_back("file-02.mov");
        loadedFiles.push_back("file-q-03.mov");
        loadedFiles.push_back("file-04.mov");
        loadedFiles.push_back("file-q-05.mov");
        loadedFiles.push_back("file-06.mov");
        loadedFiles.push_back("file-q-07.mov");
        loadedFiles.push_back("file-08.mov");
        loadedFiles.push_back("file-q-09.mov");
        loadedFiles.push_back("file-10.mov");
        loadedFiles.push_back("file-q-11.mov");
        loadedFiles.push_back("file-12.mov");
        loadedFiles.push_back("file-q-13.mov");
        loadedFiles.push_back("file-14.mov");
        loadedFiles.push_back("file-q-15.mov");
        labels = loadedFiles;
        focused = 0;
    }

    void handleSelectedMenuItem() override
    {
        std::cout << "File " << getFocusedLabel() << " selected" << std::endl;
        // todo:
        // get the current Bank and Mediaslot-ID
        // publish an event that connectes the Mediaslot-ID with the current focused file
        // e.g. appEventBus.publish(AppEvent::AddFileToMediaSlot(/* add Filename, ID etc...*/));
    }
};
