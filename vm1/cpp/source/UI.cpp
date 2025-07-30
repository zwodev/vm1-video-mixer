#include "UI.h"


UI::UI(StbRenderer &stbRenderer, EventBus &eventBus) : 
    m_stbRenderer(stbRenderer), 
    m_eventBus(eventBus) 
{
    subscribeToEvents();
}

void UI::subscribeToEvents()
{
    // Examples:

    // Media Slot Event
    m_eventBus.subscribe<MediaSlotEvent>([this](const MediaSlotEvent& event) {
        printf("Media Slot Event - (Slot Idx: %d)\n", event.slotId);
        mediaSlotEvents.push_back(event);
    });

    // Edit Mode Event
    m_eventBus.subscribe<EditModeEvent>([this](const EditModeEvent& event) {
        printf("Edit Mode Event - (Mode Idx: %d)\n", event.modeId);
        editModeEvents.push_back(event);

    });

    // Navigation Event
    m_eventBus.subscribe<NavigationEvent>([this](const NavigationEvent& event) {
        printf("Navigation Event - (Type: %d)\n", (int)event.type);
        navigationEvents.push_back(event);
    });
}

bool UI::isNavigationEventTriggered(NavigationEvent::Type eventType) 
{
    for (auto e : navigationEvents) 
    {
        if (e.type == eventType) {
            return true;
        }
    }
    return false;
}

bool UI::isMediaSlotEventTriggered(int mediaSlotId) 
{
    for (auto e : mediaSlotEvents)
    {
        if (e.slotId == mediaSlotId) {
            return true;
        }
    }
    return false;
}

bool UI::isEditModeEventTriggered(int modeId) 
{
    for (auto e : editModeEvents) 
    {
        if (e.modeId == modeId) {
            return true;
        }
    }
    return false;
}

std::vector<int> UI::getTriggeredMediaSlotIds() 
{
    std::vector<int> ids;
    for(auto e : mediaSlotEvents) 
    {
        ids.push_back(e.slotId);
    }
    return ids;
}

std::vector<int> UI::getTriggeredEditButtons() 
{
    std::vector<int> ids;
    for(auto e : editModeEvents) 
    {
        ids.push_back(e.modeId);
    }
    return ids;
}


void UI::NewFrame()
{
    m_stbRenderer.clear();
    m_focusedIdxPtr = nullptr;
    m_x = 0;
    m_y = 0;
}

void UI::EndFrame()
{
    navigationEvents.clear();
    mediaSlotEvents.clear();
    editModeEvents.clear();
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
    //int maxWidth = m_stbRenderer.width();
    int maxHeight = m_stbRenderer.height();
    float fontSize = 16.0f;
    m_listSize = 0;
    m_focusedIdxPtr = focusedIdxPtr;
    m_lineHeight = m_stbRenderer.getFontLineHeight(fontSize) + m_textPaddingBottom;
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
    m_stbRenderer.setEnabled(true);
}

void UI::BeginListElement()
{
    int halfVisibleElements = m_visibleListElements / 2;
    int firstElementId = std::max(0, *m_focusedIdxPtr - halfVisibleElements);
    int lastElementId = firstElementId + m_visibleListElements;

    bool enableRendering = (firstElementId <= m_listSize && m_listSize <= lastElementId); 
    
    m_stbRenderer.setEnabled(enableRendering);
    m_currentElementHeight = m_lineHeight;
}

void UI::EndListElement()
{
    m_listSize++;
    if (m_stbRenderer.isEnabled()) {
        m_y += m_currentElementHeight;
        m_y += m_textPaddingBottom;
    }
}

void UI::startUpLogo() {
    int centerX = m_stbRenderer.width() / 2;
    int centerY = m_stbRenderer.height() / 2;
    int quadSize = 80;
    m_stbRenderer.drawEmptyRect(centerX - quadSize / 2, centerY - quadSize /2, quadSize, quadSize, COLOR::WHITE);

    CenteredText("VM-1");
}

void UI::CenteredText(const std::string &label)
{
    float fontSize = 16.0f;
    float fontWidth = m_stbRenderer.getTextWidth(label, fontSize);
    float fontHeight = m_stbRenderer.getFontLineHeight(fontSize);

    int centerX = m_stbRenderer.width() / 2;
    int centerY = m_stbRenderer.height() / 2;

    m_stbRenderer.drawText(label, 
                           centerX - fontWidth / 2, 
                           centerY - fontHeight / 2, 
                           fontSize, 
                           COLOR::WHITE);
}

void UI::Text(const std::string &label)
{
    UI::BeginListElement();
    float fontSize = 16.0f;
    Color color = COLOR::WHITE;
    if (m_focusedIdxPtr) {
        if ((*m_focusedIdxPtr) == m_listSize){            
            m_stbRenderer.drawRect(m_x, m_y - 1, m_stbRenderer.width() - m_x, fontSize - 2, COLOR::WHITE);
            color = COLOR::BLACK;
        } 
    }
    m_stbRenderer.drawText(label, m_x, m_y, fontSize, color);
    UI::EndListElement();
}

void UI::MenuTitle(std::string menuTitle)
{
    float fontSize = 32.0f;
    m_stbRenderer.drawText(menuTitle, m_x, m_y, fontSize, COLOR::WHITE);
    m_y += m_stbRenderer.getFontLineHeight(fontSize);
    m_y += m_titlePaddingBottom;
}

void UI::MenuInfo(std::string menuInfo)
{
    float fontSize = 32.0f;
    int width = m_stbRenderer.width();
    int textWidth = m_stbRenderer.getTextWidth(menuInfo, fontSize);
    // std::cout << "text width for '" << menuInfo << "': " << textWidth << std::endl;
    m_stbRenderer.drawEmptyRect(width - textWidth - 2, 
                                0, 
                                textWidth + 1, 
                                m_stbRenderer.getFontLineHeight(fontSize) - 1, 
                                COLOR::WHITE);
    m_stbRenderer.drawText(menuInfo, 
                           width - textWidth - 1, 
                           0, 
                           fontSize, 
                           COLOR::WHITE);
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
    bool keyPressed = (isNavigationEventTriggered(NavigationEvent::Type::SelectItem));
    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && keyPressed) {
        checked = !checked;
    }

    // std::string newLabel = "[ ] " + label;
    // if (checked) newLabel = "[x] " + label;
    // Text(newLabel);
    if(checked) {
        m_stbRenderer.drawRect(m_x, m_y + 2, 7, 7, COLOR::WHITE);
    }
    else {
        m_stbRenderer.drawEmptyRect(m_x, m_y + 2, 7, 7, COLOR::WHITE);
    }
    m_x = 10;
    Text(label);
    m_x = 0;
    return checked != oldChecked;
}

bool UI::RadioButton(const std::string& label, bool active)
{
    if (!m_focusedIdxPtr) return false;
    bool keyPressed = (isNavigationEventTriggered(NavigationEvent::Type::SelectItem));
    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && !active && keyPressed) {
        active = true;
    }

    // std::string newLabel = "[ ] " + label;
    // if (active) newLabel = "[*] " + label;
    // Text(newLabel);
    if(active) m_stbRenderer.drawRect(m_x, m_y + 2, 7, 7, COLOR::WHITE);
    m_x = m_listPaddingLeft;
    Text(label);
    m_x = 0;

    return focused && active;
}

void UI::SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue)
{
    if (!m_focusedIdxPtr) return;
    int diff = 0;
    if(isNavigationEventTriggered(NavigationEvent::Type::IncreaseValue))
    {
        diff = 1;    
    }
    else if(isNavigationEventTriggered(NavigationEvent::Type::DecreaseValue))
    {
        diff = -1;
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