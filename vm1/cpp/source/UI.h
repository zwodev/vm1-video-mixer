#pragma once

#include "imgui.h"
#include "FontManager.h"
#include "Registry.h"
#include "StbRenderer.h"
#include "EventBus.h"

#include <string>
#include <algorithm>

class UI
{
public:
    enum TextState
    {
        DEFAULT,
        HIGHLIGHT,
        SELECTED,
        ERROR,
        WARNING
    };
    UI() = delete;
    ~UI() = default;
    UI(StbRenderer &stbRenderer, EventBus &eventBus);
    void NewFrame();
    void EndFrame();
    void FocusNextElement();
    void FocusPreviousElement();
    void DrawTitle(const std::string& label);
    void BeginList(int* focusedIdxPtr);
    void EndList();
    void BeginListElement();
    void EndListElement();
    void CenteredText(const std::string &label);
    void Text(const std::string &label);
    void MenuTitle(std::string menuTitle);
    void MenuInfo(std::string menuInfo);
    void InfoScreen(int bank, int id, std::string filename);
    bool CheckBox(const std::string& label, bool checked);
    bool RadioButton(const std::string& label, bool active);
    void SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue);
    bool isNavigationEventTriggered(NavigationEvent::Type eventType);
    bool isMediaSlotEventTriggered(int mediaSlotId);
    bool isEditModeEventTriggered(int modeId);
    std::vector<int> getTriggeredMediaSlotIds();
    std::vector<int> getTriggeredEditButtons();
    
private:
    void subscribeToEvents();

    std::vector<MediaSlotEvent> mediaSlotEvents;
    std::vector<EditModeEvent> editModeEvents;
    std::vector<NavigationEvent> navigationEvents;

    StbRenderer& m_stbRenderer;
    EventBus& m_eventBus;
    int m_x = 0;
    int m_y = 0;
    int m_listSize = 0;
    int m_lineHeight = 0;
    int m_menuHeight = 0;
    int m_visibleListElements = 0;
    int* m_focusedIdxPtr = 0;
    int m_firstLine = 0;
    int m_currentElementHeight = 0;
};