#pragma once

#include "imgui.h"
#include "FontManager.h"
#include "Registry.h"
#include "StbRenderer.h"

#include <string>
#include <algorithm>

class UI
{
private:
    UI() {}
    ~UI() {}

public:
    enum TextState
    {
        DEFAULT,
        HIGHLIGHT,
        SELECTED,
        ERROR,
        WARNING
    };

    static void SetRenderer(StbRenderer* stbRenderer);
    static void NewFrame();
    static void FocusNextElement();
    static void FocusPreviousElement();
    static void DrawTitle(const std::string& label);
    static void BeginList(int* focusedIdxPtr);
    static void EndList();
    static void BeginListElement();
    static void EndListElement();
    static void CenteredText(const std::string &label);
    static void Text(const std::string &label);
    static void MenuTitle(std::string menuTitle);
    static void MenuInfo(std::string menuInfo);
    static void InfoScreen(int bank, int id, std::string filename);
    static bool CheckBox(const std::string& label, bool checked);
    static bool RadioButton(const std::string& label, bool active);
    static void SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue);

private:
    static StbRenderer* m_stbRenderer;
    static int m_x;
    static int m_y;
    static int m_listSize;
    static int m_lineHeight;
    static int m_menuHeight;
    static int m_visibleListElements;
    static int* m_focusedIdxPtr;
    static int m_firstLine;
    static int m_currentElementHeight;
};