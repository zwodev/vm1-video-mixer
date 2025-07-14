#include "UIHelper.h"

StbRenderer* UI::m_stbRenderer = 0;
int UI::m_x = 0;
int UI::m_y = 0;
int UI::m_listSize = 0;
int UI::m_lineHeight = 0;
int UI::m_menuHeight = 0;
int UI::m_visibleListElements = 0;
int* UI::m_focusedIdx = nullptr;
int UI::m_firstLine = 0;
int UI::m_currentElementHeight = 0;

void UI::SetRenderer(StbRenderer* stbRenderer)
{
    m_stbRenderer = stbRenderer;
}

void UI::NewFrame()
{
    m_stbRenderer->clear();
    m_focusedIdx = nullptr;
    m_x = 0;
    m_y = 0;
}

void UI::FocusNextElement()
{
    if (!m_focusedIdx) return;
    if ((*m_focusedIdx) + 1 < m_listSize) {
        (*m_focusedIdx)++;
    }
}

void UI::FocusPreviousElement()
{
    if (!m_focusedIdx) return;
    if ((*m_focusedIdx) > 0) {
        (*m_focusedIdx)--;
    }
}

void UI::BeginList(int* focusedIdx) 
{   
    if (!focusedIdx) return;
    //int maxWidth = m_stbRenderer->width();
    int maxHeight = m_stbRenderer->height();
    float fontSize = 16.0f;
    m_listSize = 0;
    m_focusedIdx = focusedIdx;
    m_lineHeight = m_stbRenderer->getFontLineHeight(fontSize);
    m_menuHeight = maxHeight - m_y;
    m_visibleListElements = m_menuHeight / m_lineHeight;
    m_firstLine = std::clamp(*m_focusedIdx - m_visibleListElements / 2,
                            0, std::max(0, m_listSize - m_visibleListElements));
    //m_y += m_firstLine * m_lineHeight;
}

void UI::EndList()
{
    m_stbRenderer->setEnabled(true);
}

void UI::BeginListElement()
{
    bool enableRendering = (m_listSize < (m_visibleListElements + m_firstLine)); 
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

void UI::MediaButtonID(int id)
{
    // ImFont *font_big = FontManager::GetInstance().font_big;
    // int width = (int)ImGui::GetWindowSize().x;
    // int height = (int)ImGui::GetWindowSize().y;

    // ImGui::PushFont(FontManager::GetInstance().font_big);

    // ImDrawList *drawList = ImGui::GetForegroundDrawList();

    // int id16 = id % 16;
    // char bank = id / 16 + 65;
    // std::string text = std::string(1, bank) + std::to_string(id16);

    // ImVec2 textExtent = ImGui::CalcTextSize(text.c_str());

    // // ImGui::PushClipRect({}, {1000, 1000}, false); // Disable clipping to prevent cutting corners

    // int x = width - (int)textExtent.x;
    // int y = 0;

    // ImVec2 rectStart = ImVec2(x, y);
    // ImVec2 rectEnd = ImVec2(
    //     x + static_cast<float>(textExtent.x),
    //     y + static_cast<float>(textExtent.y) - 4); // hard-coded 4 pixels less in height

    // // printf("textExtend x: %f, y: %f // start-x: %d, start-y: %d, width: %d, height: %d\n", textExtent.x, textExtent.y, x, y, width, height);

    // ImU32 rectColor = IM_COL32(255, 255, 255, 255);
    // ImU32 textColor = IM_COL32(0, 0, 0, 255);
    // drawList->AddRectFilled(rectStart, rectEnd, rectColor);
    // // drawList->AddRectFilled(ImVec2(0, 0), ImVec2(width, textExtent.y - 4), rectColor);
    // drawList->AddText(ImVec2(x, y), textColor, text.c_str());
    // // drawList->AddRectFilled(ImVec2(0, rectEnd.y), ImVec2(width, rectEnd.y + 1), rectColor);
    // drawList->AddRectFilled(ImVec2(rectStart.x - 3, 0), ImVec2(rectStart.x - 1, rectEnd.y), textColor);

    // // ImGui::PopClipRect();
    // ImGui::PopFont();
}

void UI::Text(const std::string &label)
{
    UI::BeginListElement();
    float fontSize = 16.0f;
    Color color = COLOR::WHITE;
    if (m_focusedIdx) {
        if ((*m_focusedIdx) == m_listSize) color = COLOR::RED;
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
    if (!m_focusedIdx) return false;
    bool oldChecked = checked;
    bool keyPressed = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)));
    bool focused = ((*m_focusedIdx) == m_listSize);
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
    if (!m_focusedIdx) return false;
    bool keyPressed = (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_DownArrow)));
    bool focused = ((*m_focusedIdx) == m_listSize);
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
    if (!m_focusedIdx) return;
    int diff = 0;
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) diff = 1;
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) diff = -1;
    }

    bool focused = ((*m_focusedIdx) == m_listSize);
    if (focused && diff != 0) {
        value += diff;
        if (value < minValue) value = minValue;
        else if (value > maxValue) value = maxValue;
    }

    std::string newLabel = label + ": " + std::to_string(value);
    Text(newLabel);
}