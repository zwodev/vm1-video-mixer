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
    m_menuTitleHeight = m_stbRenderer.getFontLineHeight(FONT::TEXTSTYLE::MENU_TITLE);
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

void UI::TextStyle(FONT::TextStyle textStyle)
{
    m_currentTextStyle = textStyle;
    m_lineHeight = m_stbRenderer.getFontLineHeight(m_currentTextStyle) + m_textPaddingBottom;
}

void UI::TextColor(Color color)
{
    m_currentColor = color;
}

void UI::pushTranslate(int x, int y)
{
    m_translateStack.push_back(glm::vec2(m_x, m_y));
    m_x += x;
    m_y += y;
}

void UI::popTranslate()
{
    if (m_translateStack.empty()) return;
    glm::vec2 translate = m_translateStack.back();
    m_translateStack.pop_back();
    m_x = translate.x;
    m_y = translate.y;
}

void UI::BeginList(int* focusedIdxPtr) 
{   
    if (!focusedIdxPtr) return;
    //int maxWidth = m_stbRenderer.width();
    int maxHeight = m_stbRenderer.height();
    float fontSize = 16.0f;
    m_listSize = 0;
    m_focusedIdxPtr = focusedIdxPtr;
    m_lineHeight = m_stbRenderer.getFontLineHeight(m_currentTextStyle) + m_textPaddingBottom;
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
    // float fontSize = 16.0f;
    float fontWidth = m_stbRenderer.getTextWidth(label, m_currentTextStyle);
    float fontHeight = m_stbRenderer.getFontLineHeight(m_currentTextStyle);

    int centerX = m_stbRenderer.width() / 2;
    int centerY = m_stbRenderer.height() / 2;

    m_stbRenderer.drawText(label, 
                           centerX - fontWidth / 2, 
                           centerY - fontHeight / 2, 
                           m_currentTextStyle, 
                           m_currentColor);
}

bool UI::Text(const std::string &label)
{
    UI::BeginListElement();
    // float fontSize = m_currentTextStyle.size;
    Color color = m_currentColor;
    float textWidth = m_stbRenderer.getTextWidth(label, m_currentTextStyle);
    bool selected = (m_focusedIdxPtr && ((*m_focusedIdxPtr) == m_listSize));
    if (selected) {
            m_stbRenderer.drawRect(m_x, 
                                   m_y - 1, 
                                // m_stbRenderer.width() - m_x, 
                                   textWidth, 
                                   m_lineHeight, 
                                   color);
            color = COLOR::BLACK;
    }
    m_stbRenderer.drawText(label, m_x, m_y, m_currentTextStyle, color);
    UI::EndListElement();

    return selected;
}

void UI::PlainText(const std::string &label)
{
    UI::BeginListElement();
    // float fontSize = 16.0f;
    // Color color = COLOR::WHITE;
    // if (m_focusedIdxPtr) {
    //     if ((*m_focusedIdxPtr) == m_listSize){            
    //         m_stbRenderer.drawRect(m_x, m_y - 1, m_stbRenderer.width() - m_x, fontSize - 2, COLOR::WHITE);
    //         color = COLOR::BLACK;
    //     } 
    // }
    m_stbRenderer.drawText(label, m_x, m_y, m_currentTextStyle, m_currentColor);
    UI::EndListElement();
}

void UI::Spacer(float value) {
    m_y += value;
}

void UI::ShowMenuTitle(std::string menuTitle, Color color)
{
    m_y = m_screenPaddingTop;
    // m_stbRenderer.drawText(menuTitle, 0, 0, FONT::TEXTSTYLE::MENU_TITLE, color);
    // m_y += m_stbRenderer.getFontLineHeight(FONT::TEXTSTYLE::MENU_TITLE);
    // m_y += m_titlePaddingBottom;
    m_menuTitleWidth = m_stbRenderer.getTextWidth(menuTitle, FONT::TEXTSTYLE::MENU_TITLE);
    m_stbRenderer.drawEmptyRect(
        m_x, 
        m_y, 
        m_menuTitleWidth + 10, 
        m_menuTitleHeight, 
        COLOR::WHITE, 2);
    m_stbRenderer.drawText(menuTitle, 
        m_x + 5, 
        m_y, 
        FONT::TEXTSTYLE::MENU_TITLE, 
        COLOR::WHITE);
}

void UI::ShowMediaSlotInfo(std::string menuInfo)
{
    m_y = m_screenPaddingTop;
    m_x = 70;
    int textWidth = m_stbRenderer.getTextWidth(menuInfo, FONT::TEXTSTYLE::MENU_TITLE);

    m_stbRenderer.drawEmptyRect(m_x, 
                                m_y, 
                                textWidth + 10, 
                                m_menuTitleHeight, 
                                COLOR::WHITE, 2);
    m_stbRenderer.drawText(menuInfo, 
                            m_x + 5, 
                            m_y, 
                            FONT::TEXTSTYLE::MENU_TITLE, 
                            COLOR::WHITE);
    m_x = 0;
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

    int fontSize = FONT::TEXTSTYLE::STANDARD.size;
    int x = 4;
    int y = height/2 - fontSize/2;

    m_stbRenderer.drawText(message, x, y, FONT::TEXTSTYLE::STANDARD, COLOR::WHITE);
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
    m_stbRenderer.drawText(title, width/10+10, height/10+10, FONT::TEXTSTYLE::STANDARD, COLOR::WHITE);
    m_stbRenderer.drawText(input, width/10+10, height/10+70, FONT::TEXTSTYLE::STANDARD, COLOR::WHITE);
}

void UI::ShowButtonMatrix(std::vector<std::pair<char, Color>> buttonTexts)
{
    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();
    //m_stbRenderer.clear();
    
    int quadPadding = 2;
    int quadSize = (width / (buttonTexts.size() / 2)) - quadPadding;
    // int fontSize = 16;
    for(int i = 0; i < buttonTexts.size(); i++)
    {
        int x = (quadPadding / 2) + (i % 8) * (quadSize + quadPadding);
        int y = (height/2 - quadSize / 2) + (i / 8) * (quadSize + quadPadding + 2);

        if (buttonTexts[i].second == COLOR::BLACK) {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, Color(30, 30, 30));
            m_stbRenderer.drawText(std::string(1, static_cast<char>(buttonTexts[i].first)), x + 4, y + 3, FONT::TEXTSTYLE::STANDARD, Color(120, 120, 120));
        }
        else {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, buttonTexts[i].second);
            m_stbRenderer.drawText(std::string(1, static_cast<char>(buttonTexts[i].first)), x + 4, y + 3, FONT::TEXTSTYLE::STANDARD, COLOR::WHITE);
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
    // int fontSize = 16;
    for(int i = 0; i < BANK_COUNT; i++)
    {
        int x = quadPadding / 2 + i * (quadSize + quadPadding);
        int y = height/2 - quadSize / 2;
        if(bank == i) {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, COLOR::WHITE);
            m_stbRenderer.drawText(std::string(1, static_cast<char>(i + 65)), x + 4, y + 3, FONT::TEXTSTYLE::STANDARD, COLOR::BLACK);
        } else {
            m_stbRenderer.drawEmptyRect(x, y, quadSize, quadSize, COLOR::WHITE);
            m_stbRenderer.drawText(std::string(1, static_cast<char>(i + 65)), x + 4, y + 3, FONT::TEXTSTYLE::STANDARD, COLOR::WHITE);
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

void UI::PlanePreview(std::vector<PlaneSettings> planes, int& selectedPlane, PlanePreviewStyle style)
{
    float rectWidth;
    float rectHeight;
    float centerX;
    float centerY;
    float rectCenterX[2];
    float aspectRatio = 16.0/9.0f;                         // todo: get aspect ratio from screen(s)
    float polygonThickness = 2.0f;
    
    if (style == PLANE_PREVIEW_SMALL) 
    {
        m_y = m_screenPaddingTop;
        // rectWidth = float(m_stbRenderer.width()) / 8.0f;
        // rectHeight = int(float(rectWidth) / aspectRatio);
        rectHeight = m_menuTitleHeight;
        rectWidth = int(float(rectHeight) * aspectRatio);
        float spacing = 7.0f;
        float paddingRight = 15.0f;
        centerX = float(m_stbRenderer.width() - rectWidth - spacing * 2.0f - paddingRight);
        rectCenterX[0] = centerX - rectWidth/1.75f - spacing;
        rectCenterX[1] = centerX + rectWidth/1.75f + spacing;
        centerY = m_y + rectHeight / 2;
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES) 
    {
        rectWidth = float(m_stbRenderer.width()) / 3.0f;
        rectHeight = int(float(rectWidth) / aspectRatio);
        centerX = float(m_stbRenderer.width()) / 2.0f;
        rectCenterX[0] = centerX - rectWidth/1.75f;
        rectCenterX[1] = centerX + rectWidth/1.75f;
        centerY = m_y + rectHeight / 2;
    }

    // create preview scale plane shapes
    struct PlaneShape {
        std::vector<glm::vec2> vertices;
        int hdmiId;
    };
    std::vector<PlaneShape> correctedPlanes;
    correctedPlanes.reserve(planes.size());
    for (const PlaneSettings& p : planes) {
        correctedPlanes.push_back(PlaneShape{});
        correctedPlanes.back().hdmiId = p.hdmiId;
        correctedPlanes.back().vertices.reserve(p.coords.size());
        const float halfWidth  = rectWidth * 0.5f;
        const float halfHeight = rectHeight * 0.5f;
        const float cx         = rectCenterX[p.hdmiId];
        const float cy         = centerY;
        const float scale      = p.scale;
        const float tx         = p.translation.x * halfWidth;
        const float ty         = p.translation.y * halfHeight;
        
        for (const glm::vec2& coord : p.coords) {
            glm::vec2 pos;
            pos.x = cx +  coord.x * halfWidth  * scale + tx;
            pos.y = cy + (coord.y * halfHeight * scale + ty) * -1.0f;
            correctedPlanes.back().vertices.push_back(pos);
        }
    }

    // draw screens outlines
    if (style == PLANE_PREVIEW_SMALL) 
    {
        m_stbRenderer.drawEmptyCenteredRect(rectCenterX[0], centerY, rectWidth, rectHeight, COLOR::WHITE, 2.0f);
        m_stbRenderer.drawEmptyCenteredRect(rectCenterX[1], centerY, rectWidth, rectHeight, COLOR::WHITE, 2.0f);
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES) 
    {
        m_stbRenderer.drawEmptyCenteredRect(rectCenterX[0], centerY, rectWidth, rectHeight, COLOR::WHITE);
        m_stbRenderer.drawEmptyCenteredRect(rectCenterX[1], centerY, rectWidth, rectHeight, COLOR::WHITE);
    }

    // draw planes
    Color colors[] = {COLOR::PLANE_0, COLOR::PLANE_1, COLOR::PLANE_2, COLOR::PLANE_3};
    if (style == PLANE_PREVIEW_SMALL) 
    {
        // draw only one plane 
        PlaneShape p = correctedPlanes[selectedPlane];
        Color color = colors[selectedPlane];
        if(p.vertices.size() >= 4)
        {
            m_stbRenderer.drawEmptyPolygon (
                p.vertices[0].x, p.vertices[0].y, 
                p.vertices[1].x, p.vertices[1].y,
                p.vertices[2].x, p.vertices[2].y,
                p.vertices[3].x, p.vertices[3].y,
                color,
                polygonThickness);
        }
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES)
    {
        // draw all planes
        int i = 0;
        for(PlaneShape p : correctedPlanes) {
            if(p.vertices.size() >= 4){
                Color color = i == selectedPlane ? colors[i] : COLOR::GREY;

                m_stbRenderer.drawEmptyPolygon(
                    p.vertices[0].x, p.vertices[0].y, 
                    p.vertices[1].x, p.vertices[1].y,
                    p.vertices[2].x, p.vertices[2].y,
                    p.vertices[3].x, p.vertices[3].y,
                    color,
                    polygonThickness);
                i++;
            }
        }
    }

    // draw tiny horizontal lines to indicate the layer position
    int layerlineLength;
    float layerSpacing;
    if (style == PLANE_PREVIEW_SMALL) 
    {
        layerlineLength = 10;
        layerSpacing = 5.0f;
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES)
    {
        layerlineLength = 15;
        layerSpacing = 5.0f;
    }
    int layerPreviewHeight = correctedPlanes.size() * layerSpacing;
    int layerPreviewBottom = centerY + layerPreviewHeight / 2.0f;
    int i = 0;
    for(PlaneShape p : correctedPlanes)
    {
        int y = int(layerPreviewBottom - float(i) * layerSpacing);
        Color color = i == selectedPlane ? colors[i] : COLOR::WHITE;
            
        if(p.hdmiId == 0)
        {
            int x = rectCenterX[0] - rectWidth/2 - 5;
            m_stbRenderer.drawLine(x, y, x - layerlineLength, y, color, 2);
        }
        else if(p.hdmiId == 1)
        {
            int x = rectCenterX[1] + rectWidth/2 + 5;
            m_stbRenderer.drawLine(x, y, x + layerlineLength, y, color, 2);
        }
        i++;
    }

    
    // draw plane indices as numbers
    if (style == PLANE_PREVIEW_SMALL) 
    {
        // draw only currently selected plane index between the two rectangles
        Color color = colors[selectedPlane];
        FONT::TextStyle textStyle = FONT::TEXTSTYLE::STANDARD;
        textStyle.size = 24.0f;
        m_stbRenderer.drawText(std::to_string(selectedPlane + 1), 
                                centerX - textStyle.size / 4, 
                                centerY - textStyle.size / 2, 
                                textStyle,
                                color);
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES) 
    {
        // draw plane indices in the center of the polygons
        int i = 0;
        for(PlaneShape p : correctedPlanes) 
        {
            glm::vec2 polygonCenter = {0.0f, 0.0f};
            for(std::size_t i = 0; i < p.vertices.size(); i++) {
                polygonCenter += p.vertices[i];
            }
            polygonCenter /= p.vertices.size();
        
            FONT::TextStyle textStyle = FONT::TEXTSTYLE::STANDARD;
            textStyle.size = 24.0f;
            Color color = i == selectedPlane ? colors[i] : COLOR::GREY;
            m_stbRenderer.drawText(std::to_string(i+1), 
                                    polygonCenter.x, 
                                    polygonCenter.y - textStyle.size/4, 
                                    textStyle, 
                                    color);
            i++;
        }
    }

    // handle vertex selection and highlight selected vertex
    static int selectedVertex = 0;
    if(style == PLANE_PREVIEW_VERTICES)    
    {
        if(isNavigationEventTriggered(NavigationEvent::Type::NavigationDown))
        {
            selectedVertex += 1;
            if (selectedVertex > planes[selectedPlane].coords.size()-1) selectedVertex = planes[selectedPlane].coords.size()-1;
        }
        else if(isNavigationEventTriggered(NavigationEvent::Type::NavigationUp))
        {
            selectedVertex -= 1;
            if (selectedVertex < 0) selectedVertex = 0;
        }

        // highlight selected vertex
        PlaneShape p = correctedPlanes[selectedPlane];
        int i = p.vertices.size() - 1 - selectedVertex; // (vertices are in reverse order)
        glm::vec2 vertex = p.vertices[i];
        m_stbRenderer.drawRect(vertex.x, vertex.y, 5, 5, COLOR::RED);
    }
}

void UI::savePNG(const std::string& filename){
    m_stbRenderer.savePNG(filename);
}

int UI::getMenuTitleHeight() const
{
    return m_menuTitleHeight;
}