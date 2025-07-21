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

private:
    StbRenderer& m_stbRenderer;
    EventBus& m_eventBus;
    int m_x;
    int m_y;
    int m_listSize;
    int m_lineHeight;
    int m_menuHeight;
    int m_visibleListElements;
    int* m_focusedIdxPtr;
    int m_firstLine;
    int m_currentElementHeight;
};