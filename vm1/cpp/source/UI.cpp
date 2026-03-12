#include "UI.h"
#include "VM1DeviceDefinitions.h"
#include "StringHelper.h"

UI::UI(StbRenderer &stbRenderer, EventBus &eventBus) : 
    m_stbRenderer(stbRenderer), 
    m_eventBus(eventBus) 
{
    subscribeToEvents();
}

void UI::subscribeToEvents()
{
    // Media Slot Event
    m_eventBus.subscribe<MediaSlotEvent>([this](const MediaSlotEvent& event) {
        mediaSlotEvents.push_back(event);
    });

    // Edit Mode Event
    m_eventBus.subscribe<EditModeEvent>([this](const EditModeEvent& event) {
        editModeEvents.push_back(event);
    });

    // Navigation Event
    m_eventBus.subscribe<NavigationEvent>([this](const NavigationEvent& event) {
        navigationEvents.push_back(event);
    });

    // ValueChange Event
    m_eventBus.subscribe<ValueChangeEvent>([this](const ValueChangeEvent& event) {
        valueChangeEvents.push_back(event);
    });

    // BankChange Event
    m_eventBus.subscribe<BankChangeEvent>([this](const BankChangeEvent& event) {
        bankChangeEvents.push_back(event);
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

bool UI::isBankChangeEventTriggered(int& bankId)
{
    for (auto iter = bankChangeEvents.rbegin(); iter != bankChangeEvents.rend(); ++iter) 
    {
       bankId = iter->bankId;
       return true;
    }
    return false;
}

bool UI::isValueChangeEventTriggered(ValueChangeEvent::Type eventType, int id)
{
    for (auto iter = valueChangeEvents.rbegin(); iter != valueChangeEvents.rend(); ++iter) 
    {
        if (iter->type == eventType && iter->id == id) {
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
    bankChangeEvents.clear();
    valueChangeEvents.clear();
}

void UI::StartOverlay(std::function<void()> overlay)
{
    m_overlay = overlay;
    auto now = std::chrono::steady_clock::now();
    m_overlayStartTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
}

void UI::ShowOverlay()
{
    auto now = std::chrono::steady_clock::now();
    int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    if (now_ms >= m_overlayStartTimeMs + m_overlayDurationMs) 
    {
        StopOverlay();
    }
    if(m_overlay)
    {
        m_overlay();
    }
}

void UI::StopOverlay()
{
    m_overlay = nullptr;
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
    int quadSize = 60;
    m_stbRenderer.drawEmptyRect(centerX - quadSize / 2, centerY - quadSize /2, quadSize, quadSize, COLOR::WHITE);

    CenteredText("VM-1");
}

void UI::Image(const ImageBuffer& imageBuffer)
{
    m_stbRenderer.drawImage(imageBuffer, m_x, m_y);
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

bool UI::Text(const std::string &label)
{
    UI::BeginListElement();
    float fontSize = 16.0f;
    Color color = COLOR::WHITE;
    bool selected = (m_focusedIdxPtr && ((*m_focusedIdxPtr) == m_listSize));
    if (selected) {
            m_stbRenderer.drawRect(m_x, m_y - 1, m_stbRenderer.width() - m_x, fontSize - 2, COLOR::WHITE);
            color = COLOR::BLACK;
    }
    m_stbRenderer.drawText(label, m_x, m_y, fontSize, color);
    UI::EndListElement();

    return selected;
}

void UI::PlainText(const std::string &label)
{
    UI::BeginListElement();
    float fontSize = 16.0f;
    Color color = COLOR::WHITE;
    // if (m_focusedIdxPtr) {
    //     if ((*m_focusedIdxPtr) == m_listSize){            
    //         m_stbRenderer.drawRect(m_x, m_y - 1, m_stbRenderer.width() - m_x, fontSize - 2, COLOR::WHITE);
    //         color = COLOR::BLACK;
    //     } 
    // }
    m_stbRenderer.drawText(label, m_x, m_y, fontSize, color);
    UI::EndListElement();
}

void UI::Break() {
    m_y += 15.0f;
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

void UI::ShowPopupMessage(std::string message)
{
    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();
    m_stbRenderer.clear();

    int fontSize = 16;
    int x = 4;
    int y = height/2 - fontSize/2;

    m_stbRenderer.drawText(message, x, y, fontSize, COLOR::WHITE);
}

void UI::ShowStringInputDialog(std::string title, int& cursorIdx, std::string& input)
{
    if (cursorIdx >= input.size()) return;

    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();

    char currentChar = input.at(cursorIdx);
    // printf("%c\n", currentChar);
    if(isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0)) {
        currentChar++;
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Type::Down, 0)) {
        currentChar--;
    }
    else if(isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        cursorIdx++;
        if (cursorIdx >= input.size()) {
            input.push_back('a');
            currentChar = input.at(cursorIdx);
        } 
    }
    else if(isNavigationEventTriggered(NavigationEvent::Type::NavigationLeft)) {
        cursorIdx--;
    }
    input.at(cursorIdx) = currentChar;
    m_stbRenderer.drawRect(width/10, height/10, width - (width/10*2), height - (height/10*2), COLOR::BLACK);
    m_stbRenderer.drawEmptyRect(width/10, height/10, width - (width/10*2), height - (height/10*2), COLOR::WHITE);
    m_stbRenderer.drawText(title, width/10+10, height/10+10, 26, COLOR::WHITE);
    m_stbRenderer.drawText(input, width/10+10, height/10+70, 22, COLOR::WHITE);
}

void UI::ShowButtonMatrix(std::vector<std::pair<char, Color>> buttonTexts)
{
    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();
    //m_stbRenderer.clear();
    
    int quadPadding = 2;
    int quadSize = (width / (buttonTexts.size() / 2)) - quadPadding;
    int fontSize = 16;
    for(int i = 0; i < buttonTexts.size(); i++)
    {
        int x = (quadPadding / 2) + (i % 8) * (quadSize + quadPadding);
        int y = (height/2 - quadSize / 2) + (i / 8) * (quadSize + quadPadding + 2);

        if (buttonTexts[i].second == COLOR::BLACK) {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, Color(30, 30, 30));
            m_stbRenderer.drawText(std::string(1, static_cast<char>(buttonTexts[i].first)), x + 4, y + 3, fontSize, Color(120, 120, 120));
        }
        else {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, buttonTexts[i].second);
            m_stbRenderer.drawText(std::string(1, static_cast<char>(buttonTexts[i].first)), x + 4, y + 3, fontSize, COLOR::WHITE);
            m_stbRenderer.drawEmptyRect(x, y, quadSize, quadSize, Color(30, 30, 30));
        }
    }
}

void UI::ShowBankInfo(int bank)
{
    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();
    m_stbRenderer.clear();
    
    int quadPadding = 5;
    int quadSize = (width / BANK_COUNT) - quadPadding;
    int fontSize = 16;
    for(int i = 0; i < BANK_COUNT; i++)
    {
        int x = quadPadding / 2 + i * (quadSize + quadPadding);
        int y = height/2 - quadSize / 2;
        if(bank == i) {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, COLOR::WHITE);
            m_stbRenderer.drawText(std::string(1, static_cast<char>(i + 65)), x + 4, y + 3, fontSize, COLOR::BLACK);
        } else {
            m_stbRenderer.drawEmptyRect(x, y, quadSize, quadSize, COLOR::WHITE);
            m_stbRenderer.drawText(std::string(1, static_cast<char>(i + 65)), x + 4, y + 3, fontSize, COLOR::WHITE);
        }
    }
}

bool UI::Action(const std::string& label)
{
    if (!m_focusedIdxPtr) return false;
    bool keyPressed = isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0) ||
                      isNavigationEventTriggered(NavigationEvent::Type::NavigationRight);
    bool focused = ((*m_focusedIdxPtr) == m_listSize);

    Text(label + " ->");
    return (focused && keyPressed);
}

bool UI::CheckBox(const std::string& label, bool checked)
{
    if (!m_focusedIdxPtr) return false;

    bool oldChecked = checked;
    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused) {
        if (isValueChangeEventTriggered(ValueChangeEvent::Type::Down, 0))  // deselect
        {
            checked = false;
        } 
        else if (isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0))  // select
        {
            checked = true;
        }
        else if (isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) { // toggle
            checked = !checked;
        }
    }

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
    bool keyPressed = false;

    if (isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0) ||
        isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        keyPressed = true;
    }
    
    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && !active && keyPressed) {
        active = true;
    }

    if(active) m_stbRenderer.drawRect(m_x, m_y + 2, 7, 7, COLOR::WHITE);
    m_x = m_listPaddingLeft;
    Text(label);
    m_x = 0;

    return focused && active;
}

bool UI::SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue, int step)
{
    bool hasChanged = false;
    if (!m_focusedIdxPtr) return;
    int diff = 0;
    if(isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0))
    {
        hasChanged = true;
        diff = step;    
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Type::Down, 0))
    {
        hasChanged = true;
        diff = -step;
    }

    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && diff != 0) {
        value += diff;
        if (value < minValue) value = minValue;
        else if (value > maxValue) value = maxValue;
    }

    std::string newLabel = label + ": " + std::to_string(value);
    Text(newLabel);
    return hasChanged;
}

bool UI::SpinBoxPlaneSelect(int& value, int minValue, int maxValue)
{
    bool hasChanged = false;
    if(isNavigationEventTriggered(NavigationEvent::Type::NavigationAuxDown))
    {
        hasChanged = true;
        value += 1;
        if (value > maxValue) value = maxValue;
    }
    else if(isNavigationEventTriggered(NavigationEvent::Type::NavigationAuxUp))
    {
        hasChanged = true;
        value -= 1;
        if (value < minValue) value = minValue;
    }

    int maxWidth = m_stbRenderer.width();
    int padding = 15;
    int boxSize = 30;
    int totalBoxWidth = (maxValue + 1) * boxSize + (maxValue * padding);
    int gap = (maxWidth - totalBoxWidth) / 2;
    int x = gap;
    for(int i = 0; i <= maxValue; ++i) {
        if (i == value){
            m_stbRenderer.drawRect(x, m_y, boxSize, boxSize);
            m_stbRenderer.drawText(std::to_string(i+1), x + boxSize/3, m_y + boxSize/4, 24, COLOR::BLACK);
        } else {
            m_stbRenderer.drawEmptyRect(x, m_y, boxSize, boxSize);
            m_stbRenderer.drawText(std::to_string(i+1), x + boxSize/3, m_y + boxSize/4, 24, COLOR::WHITE);
        }
        x += boxSize + padding;
    }
    m_y += boxSize;
    // std::string newLabel = "Plane: " + std::to_string(value);
    // Text(newLabel);
    return hasChanged;
}


bool UI::SpinBoxFloat(const std::string& label, float& value, float minValue, float maxValue, float step)
{
    bool hasChanged = false;
    if (!m_focusedIdxPtr) return;
    float diff = 0;
    if(isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0))
    {
        hasChanged = true;
        diff = step;    
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Type::Down, 0))
    {
        hasChanged = true;
        diff = -step;
    }

    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && diff != 0) {
        value += diff;
        if (value < minValue) value = minValue;
        else if (value > maxValue) value = maxValue;
    }

    std::string newLabel = label + ": " + strhlpr::formatFloat(value, 2);
    Text(newLabel);
    return hasChanged;
}

// TODO: Get rid of PlaneSettings::vec2. Maybe use GLM lib in the future?
bool UI::SpinBoxVec2(const std::string& label, glm::vec2& vec, float step)
{
    bool hasChanged = false;
    if (!m_focusedIdxPtr) return;
    float diffX = 0;
    float diffY = 0;
    if(isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0))
    {
        hasChanged = true;
        diffX = step;    
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Type::Down, 0))
    {
        hasChanged = true;
        diffX = -step;
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 1))
    {
        hasChanged = true;
        diffY = step;    
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Type::Down, 1))
    {
        hasChanged = true;
        diffY = -step;
    }

    bool focused = ((*m_focusedIdxPtr) == m_listSize);
    if (focused && diffX != 0) {
        vec.x += diffX;
        // if (value < minValue) value = minValue;
        // else if (value > maxValue) value = maxValue;
    }
    else if (focused && diffY != 0) {
        vec.y += diffY;
        // if (value < minValue) value = minValue;
        // else if (value > maxValue) value = maxValue;
    }

    std::string newLabel = label + ": " + strhlpr::formatFloat(vec.x, 2)+ "/" + strhlpr::formatFloat(vec.y, 2);;
    Text(newLabel);
    return hasChanged;
}

bool UI::previewPlanes(std::vector<PlaneSettings> planes, int& selectedPlane)
{
    bool hasChanged = false;
    if(isNavigationEventTriggered(NavigationEvent::Type::NavigationAuxDown))
    {
        hasChanged = true;
        selectedPlane += 1;
        if (selectedPlane > planes.size()-1) selectedPlane = planes.size() - 1;
    }
    else if(isNavigationEventTriggered(NavigationEvent::Type::NavigationAuxUp))
    {
        hasChanged = true;
        selectedPlane -= 1;
        if (selectedPlane < 0) selectedPlane = 0;
    }

    static int selectedVertex = 0;
    if(isNavigationEventTriggered(NavigationEvent::Type::NavigationDown))
    {
        // hasChanged = true;
        selectedVertex += 1;
        if (selectedVertex > planes[selectedPlane].coords.size()-1) selectedVertex = planes[selectedPlane].coords.size()-1;
    }
    else if(isNavigationEventTriggered(NavigationEvent::Type::NavigationUp))
    {
        // hasChanged = true;
        selectedVertex -= 1;
        if (selectedVertex < 0) selectedVertex = 0;
    }


    // draw screen outlines
    float width = float(m_stbRenderer.width()) / 2.5f;
    float aspectRatio = 16.0/9.0f;  // todo: get aspect ratio from screen(s)
    float height = int(float(width) / aspectRatio);
    
    float centerX[2] = {float(m_stbRenderer.width()) / 4.0f,
                        float(m_stbRenderer.width()) - centerX[0]};
    float centerY = m_y + height / 2;
    
    m_stbRenderer.drawEmptyCenteredRect(centerX[0], centerY, width, height, COLOR::GREY);
    m_stbRenderer.drawEmptyCenteredRect(centerX[1], centerY, width, height, COLOR::GREY);

    // draw planes
    int i = 0;
    Color colors[] = {COLOR::PLANE_0, COLOR::PLANE_1, COLOR::PLANE_2, COLOR::PLANE_3};
    for(PlaneSettings p : planes) {
        m_stbRenderer.drawPolygon(
            centerX[p.hdmiId] +  p.coords[0].x * width/2.0f  * p.scale + p.translation.x * width/2.0f, 
            centerY           + (p.coords[0].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

            centerX[p.hdmiId] +  p.coords[1].x * width/2.0f  * p.scale + p.translation.x * width/2.0f,
            centerY           + (p.coords[1].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

            centerX[p.hdmiId] +  p.coords[2].x * width/2.0f  * p.scale + p.translation.x * width/2.0f,
            centerY           + (p.coords[2].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

            centerX[p.hdmiId] +  p.coords[3].x * width/2.0f  * p.scale + p.translation.x * width/2.0f,
            centerY           + (p.coords[3].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

            colors[i]
        );

        float polygonCenterX = 0.0f;
        float polygonCenterY = 0.0f;
        for(int i = 0; i < p.coords.size(); i++) {
            polygonCenterX += (p.coords[i].x) * width/2.0f  * p.scale + p.translation.x * width/2.0f;
            polygonCenterY += ((p.coords[i].y) * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0;
        }
        polygonCenterX /= p.coords.size();
        polygonCenterY /= p.coords.size();
        float fontSize = 24.0f;
        m_stbRenderer.drawText(std::to_string(i+1), 
                                centerX[p.hdmiId] + polygonCenterX, 
                                centerY           + polygonCenterY - fontSize/4, 
                                fontSize, 
                                COLOR::BLACK);
        i++;
    }

    // outline selected plane
    PlaneSettings p = planes[selectedPlane];
    m_stbRenderer.drawEmptyPolygon(
        centerX[p.hdmiId] +  p.coords[0].x * width/2.0f  * p.scale + p.translation.x * width/2.0f, 
        centerY           + (p.coords[0].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

        centerX[p.hdmiId] +  p.coords[1].x * width/2.0f  * p.scale + p.translation.x * width/2.0f,
        centerY           + (p.coords[1].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

        centerX[p.hdmiId] +  p.coords[2].x * width/2.0f  * p.scale + p.translation.x * width/2.0f,
        centerY           + (p.coords[2].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 

        centerX[p.hdmiId] +  p.coords[3].x * width/2.0f  * p.scale + p.translation.x * width/2.0f,
        centerY           + (p.coords[3].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0, 
        COLOR::WHITE
    );

    // highlight selected vertex
    int s = p.coords.size()-1 - selectedVertex;
    m_stbRenderer.drawRect( 
        centerX[p.hdmiId] +  p.coords[s].x * width/2.0f  * p.scale + p.translation.x * width/2.0f - 5, 
        centerY           + (p.coords[s].y * height/2.0f * p.scale + p.translation.y * height/2.0f) * -1.0 - 5, 
        10, 
        10,
        COLOR::RED);
    
    m_y += height;
    return hasChanged;
}