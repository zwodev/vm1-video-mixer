#include "UIHelper.h"

StbRenderer* UI::m_stbRenderer = 0;
int UI::m_x = 0;
int UI::m_y = 0;
int UI::m_listSize = 0;
int UI::m_lineHeight = 0;
int UI::m_menuHeight = 0;
int UI::m_visibleListElements = 0;
int* UI::m_focusedIdxPtr = nullptr;
int UI::m_firstLine = 0;
int UI::m_currentElementHeight = 0;

void UI::SetRenderer(StbRenderer* stbRenderer)
{
    m_stbRenderer = stbRenderer;
}

void UI::NewFrame()
{
    m_stbRenderer->clear();
    m_focusedIdxPtr = nullptr;
    m_x = 0;
    m_y = 0;
}

void UI::FocusNextElement()
{
    if (!m_focusedIdxPtr) return;
    if ((*m_focusedIdxPtr) + 1 < m_listSize) {
        (*m_focusedIdxPtr)++;
    }
}

void UI::FocusPreviousElement()
{
    if (!m_focusedIdxPtr) return;
    if ((*m_focusedIdxPtr) > 0) {
        (*m_focusedIdxPtr)--;
    }
}

void UI::BeginList(int* focusedIdxPtr) 
{   
    if (!focusedIdxPtr) return;
    //int maxWidth = m_stbRenderer->width();
    int maxHeight = m_stbRenderer->height();
    float fontSize = 16.0f;
    m_listSize = 0;
    m_focusedIdxPtr = focusedIdxPtr;
    m_lineHeight = m_stbRenderer->getFontLineHeight(fontSize);
    m_menuHeight = maxHeight - m_y;
    m_visibleListElements = m_menuHeight / m_lineHeight;

    // int halfVisibleElements = m_visibleListElements / 2;
    // int firstElementId = max(0, *m_focusIndexPtr - halfVisibleElements);
    // int lastElementId = firstElementId + m_visibleListElements;
    // *m_focusIndexPtr = 3
    // m_visibleListElements = 4 ({1,2,3,4})
    // show(, *m_focusedIdxPtr + m_visibleListElements / 2);

    // 0.)
    // ---------------
    // 1.)
    // 2.)
    // 3.)
    // 4.)
    // ---------------
    // 5.)

    // 0.)
    // 1.)
    // ---------------
    // 2.)
    // 3.)
    // 4.)
    // 5.)
    // ---------------

}

void UI::EndList()
{
    m_stbRenderer->setEnabled(true);
}

void UI::BeginListElement()
{
    int halfVisibleElements = m_visibleListElements / 2;
    int firstElementId = std::max(0, *m_focusedIdxPtr - halfVisibleElements);
    int lastElementId = firstElementId + m_visibleListElements;

    bool enableRendering = (firstElementId <= m_listSize && m_listSize <= lastElementId); 
    
    m_stbRenderer->setEnabled(enableRendering);
    m_currentElementHeight = m_lineHeight;
}

void UI::EndListElement()
{
    m_listSize++;
    if (m_stbRenderer->isEnabled()) m_y += m_currentElementHeight;
}

void UI::CenteredText(const std::string &label)
{
    float fontSize = 16.0f;
    Color color = COLOR::WHITE;
    m_stbRenderer->drawText(label, 50, 50, fontSize, COLOR::WHITE);
}

void UI::Text(const std::string &label)
{
    UI::BeginListElement();
    float fontSize = 16.0f;
    Color color = COLOR::WHITE;
    if (m_focusedIdxPtr) {
        if ((*m_focusedIdxPtr) == m_listSize) color = COLOR::RED;
    }
    m_stbRenderer->drawText(label, m_x, m_y, fontSize, color);
    UI::EndListElement();
}

void UI::MenuTitle(std::string menuTitle)
{
    float fontSize = 32.0f;
    m_stbRenderer->drawText(menuTitle, m_x, m_y, fontSize, COLOR::WHITE);
    m_y += m_stbRenderer->getFontLineHeight(fontSize);
}

void UI::MenuInfo(std::string menuInfo)
{
    float fontSize = 32.0f;
    int width = m_stbRenderer->width();
    m_stbRenderer->drawText(menuInfo, width - 28, 0, fontSize, COLOR::WHITE);
}

void UI::InfoScreen(int bank, int id, std::string filename)
{
    // ImGui::SetCursorPosY(25);
    // ImGui::Text("Information:");
    // ImGui::Text("%s", filename.c_str());
    // ImGui::Text("Currrent Pos/Duration");
    // ImGui::Text("Loop yes or no");
    // ImGui::Text("%d/%d", bank, id);
}

bool UI::CheckBox(const std::string& label, bool checked)
{
    if (!m_focusedIdxPtr) return false;
    bool oldChecked = checked;
    bool keyPressed = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)));
    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && keyPressed) {
        checked = !checked;
    }

    std::string newLabel = "[ ] " + label;
    if (checked) newLabel = "[x] " + label;
    Text(newLabel);

    return checked != oldChecked;
}

bool UI::RadioButton(const std::string& label, bool active)
{
    if (!m_focusedIdxPtr) return false;
    bool keyPressed = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)));
    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && !active && keyPressed) {
        active = true;
    }

    std::string newLabel = "[ ] " + label;
    if (active) newLabel = "[*] " + label;
    Text(newLabel);

    return focused && active;
}

void UI::SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue)
{
    if (!m_focusedIdxPtr) return;
    int diff = 0;
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) diff = 1;
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) diff = -1;
    }

    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && diff != 0) {
        value += diff;
        if (value < minValue) value = minValue;
        else if (value > maxValue) value = maxValue;
    }

    std::string newLabel = label + ": " + std::to_string(value);
    Text(newLabel);
}