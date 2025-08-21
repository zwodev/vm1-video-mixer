#pragma once

#include "imgui.h"
#include "FontManager.h"
#include "Registry.h"
#include "StbRenderer.h"
#include "EventBus.h"

#include <string>
#include <algorithm>
#include <functional>

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
    void StartOverlay(std::function<void()> overlay);
    void ShowOverlay();
    void StopOverlay();
    void FocusNextElement();
    void FocusPreviousElement();
    void DrawTitle(const std::string& label);
    void BeginList(int* focusedIdxPtr);
    void EndList();
    void BeginListElement();
    void EndListElement();
    void CenteredText(const std::string &label);
    void startUpLogo();
    void Text(const std::string &label);
    void MenuTitle(std::string menuTitle);
    void MenuInfo(std::string menuInfo);
    void InfoScreen(int bank, int id, std::string filename);
    void ShowPopupMessage(std::string message);
    void ShowBankInfo(int bank);
    void ShowButtonMatrix(std::vector<std::pair<char, Color>> buttonTexts);
    bool Action(const std::string& label);
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

    std::vector<NavigationEvent> navigationEvents;
    std::vector<EditModeEvent> editModeEvents;
    std::vector<MediaSlotEvent> mediaSlotEvents;

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

    int m_titlePaddingBottom = 10;
    int m_textPaddingBottom = 2;
    int m_listPaddingLeft = 10;

    std::function<void()> m_overlay;
    int64_t m_overlayStartTimeMs = 0;
    int64_t m_overlayDurationMs = 600;
};