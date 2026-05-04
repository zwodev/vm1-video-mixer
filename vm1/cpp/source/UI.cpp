/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "UI.h"
#include "VM1DeviceDefinitions.h"
#include "StringHelper.h"

#include <cmath>

ImageBuffer UI::m_gizmoImageBuffer = ImageBuffer("media/gizmo.png");

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
    if(m_clearFrame) 
        m_stbRenderer.clear();

    m_focusedIdxPtr = nullptr;
    // m_x = 0;
    NewLine();
    m_y = m_screenPaddingTop;
}

void UI::EndFrame()
{
    navigationEvents.clear();
    mediaSlotEvents.clear();
    editModeEvents.clear();
    bankChangeEvents.clear();
    valueChangeEvents.clear();
}

void UI::setClearFrame(bool clear)
{
    m_clearFrame = clear;
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

// void UI::SetFocusedElement(int idx)
// {
//     if (!m_focusedIdxPtr) return;
//     if(idx < m_listSize-1) {
//         *m_focusedIdxPtr = idx;
//     }
// }

void UI::TextStyle(BDF::TextStyle textStyle)
{
    m_currentTextStyle = textStyle;
    m_lineHeight = m_stbRenderer.getFontLineHeight(m_currentTextStyle) + m_textPaddingBottom;
}

void UI::TextColor(Color color)
{
    m_currentTextStyle.color = color;
    m_currentColor = color;

}

void UI::PushTranslate(int x, int y)
{
    m_translateStack.push_back(glm::vec2(m_x, m_y));
    m_x += x;
    m_y += y;
}

void UI::PopTranslate()
{
    if (m_translateStack.empty()) return;
    glm::vec2 translate = m_translateStack.back();
    m_translateStack.pop_back();
    m_x = translate.x;
    m_y = translate.y;
}

int UI::CurrentListSize() {
    return m_listSize;
}

void UI::BeginList(int* focusedIdxPtr) 
{   
    if (!focusedIdxPtr) return;
    //int maxWidth = m_stbRenderer.width();
    int maxHeight = m_stbRenderer.height();
    // float fontSize = 16.0f;
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

    // TODO: Why is *m_focusedIdxPtr a large integer? Is it the pointer address? Or uninitialized?
    if (m_focusedIdxPtr && m_listSize >= 1 && *m_focusedIdxPtr >= m_listSize) {
        //printf("m_focusedIdxPtr=%p *m_focusedIdxPtr=%d m_listSize=%d\n", (void*)m_focusedIdxPtr, m_focusedIdxPtr ? *m_focusedIdxPtr : -1, m_listSize);
        *m_focusedIdxPtr = m_listSize-1;
    }
}

void UI::BeginListElement()
{
    if(m_isHidden) return;

    int halfVisibleElements = m_visibleListElements / 2;
    int firstElementId = std::max(0, *m_focusedIdxPtr - halfVisibleElements);
    int lastElementId = firstElementId + m_visibleListElements;

    bool enableRendering = (firstElementId <= m_listSize && m_listSize <= lastElementId); 
    
    m_stbRenderer.setEnabled(enableRendering);
    m_currentElementHeight = m_lineHeight;
}

void UI::EndListElement(int elementSize)
{
    m_listSize += elementSize;
    if (m_stbRenderer.isEnabled() && !m_isHidden) {
        m_y += m_currentElementHeight;
        m_y += m_textPaddingBottom;
    }
}

void UI::Image(const ImageBuffer& imageBuffer, glm::uvec2 pos)
{
    //m_stbRenderer.drawImage(imageBuffer);
    m_stbRenderer.drawSubImage(imageBuffer, pos, glm::uvec2(0, 0), glm::uvec2(imageBuffer.width, imageBuffer.height));
}

bool UI::Text(const std::string &label)
{
    UI::BeginListElement();
    bool focused = (m_focusedIdxPtr && ((*m_focusedIdxPtr) == m_listSize));
    if(!m_isHidden){
        Color color = m_currentColor;
        int textWidth = m_stbRenderer.getTextWidth(label, m_currentTextStyle);

        DrawStyle drawStyle;
        if (m_currentTextStyle.align == TextAlign::CENTER) {
            m_x = m_stbRenderer.width() / 2;
            drawStyle.anchorPoint = AnchorPoint::CENTER_TOP;
        }
        if (focused) {           
            drawStyle.color = m_currentColor;
            drawStyle.isFilled = true;
            m_stbRenderer.drawRectNEW(glm::vec2(m_x, m_y - 1), glm::vec2(
                    textWidth, 
                    m_lineHeight), 
                    drawStyle);
            color = COLOR::BLACK;
        }
        m_currentTextStyle.color = color;
        m_stbRenderer.drawTextBdf(label, glm::uvec2(m_x, m_y), m_currentTextStyle);
    }
    UI::EndListElement();

    return focused;
}

void UI::Label(const std::string &label)
{
    if(m_isHidden) return;

    // UI::BeginListElement();
    m_currentElementHeight = m_lineHeight;
    m_currentTextStyle.color = m_currentColor;
    
    m_stbRenderer.drawTextBdf(label, glm::uvec2(m_x, m_y), m_currentTextStyle );

    // UI::EndListElement();
    if (m_stbRenderer.isEnabled() && !m_isHidden) {
        m_y += m_currentElementHeight;
        m_y += m_textPaddingBottom;
    }

}

void UI::Spacer(float value)
{
    m_y += value;
}

void UI::NewLine()
{
    if(m_translateStack.size() > 0)
        m_x = m_translateStack.back().x;
    else 
        m_x = 1;

    m_y += m_currentElementLineHeight;
    m_currentElementLineHeight = 0;
}

void UI::HideElements()
{
    m_isHidden = true;
}

void UI::ShowElements()
{
    m_isHidden = false;
}

void UI::SetElementLineHeight(int height)
{
    if(height > m_currentElementHeight)
        m_currentElementLineHeight = height;
}

void UI::MenuTitleWidget(std::string label, TextAlign textAlign, Color color)
{
    if (label.empty()) return;
    int textWidth = m_stbRenderer.getTextWidth(label, BDF::TEXTSTYLE::MENU_TITLE);
    int minBoxWidth = 48;
    int boxWidth = std::max(textWidth + m_horizontalMargin * 2, minBoxWidth);
    int boxHeight = m_stbRenderer.getFontLineHeight(BDF::TEXTSTYLE::MENU_TITLE) + 8;
    int xBox = m_x;
    int xText = m_x;
    BDF::TextStyle textStyle = BDF::TEXTSTYLE::MENU_TITLE;
    textStyle.align = textAlign;
    AnchorPoint anchorPoint;

    if(textAlign == TextAlign::CENTER)
    {
        anchorPoint = AnchorPoint::CENTER_TOP;
        xBox += boxWidth / 2;
        xText = xBox;
    }
    else
    {
        anchorPoint = AnchorPoint::LEFT_TOP;    // todo: TextAlign::RIGHT not needed right now
        xBox += m_elementPadding;
        xText = xBox + m_horizontalMargin;
    }
    m_stbRenderer.drawRectNEW(glm::vec2(xBox, m_y), glm::vec2(boxWidth, boxHeight), DrawStyle{COLOR::WHITE, false, 2, anchorPoint});
    m_stbRenderer.drawTextBdf(label, 
                            glm::uvec2(xText, m_y + m_menuTitlePaddingTop),
                            textStyle);

    m_x += boxWidth + m_elementPadding;
    SetElementLineHeight(boxHeight);
}

void UI::ShowPopupMessage(std::string message)
{
    m_stbRenderer.clear();
    
    int height = m_stbRenderer.height();
    int fontHeight = m_stbRenderer.getFontLineHeight(BDF::TEXTSTYLE::MENU_ITEM);
    int x = 4;
    int y = height/2 - fontHeight/2;

    m_stbRenderer.drawTextBdf(message, glm::vec2(x, y), BDF::TEXTSTYLE::MENU_ITEM);
}

void UI::ShowTextInputDialog(std::string title, int& cursorIdx, std::string& input)
{
    if (!m_focusedIdxPtr) return;
    if (cursorIdx >= int(input.size())) return;

    char currentChar = input.at(cursorIdx);
    if(isValueChangeEventTriggered(ValueChangeEvent::Up, 1)) 
    {
        if(currentChar < 'z')
            currentChar++;
        else 
            currentChar = 'a';
        input.at(cursorIdx) = currentChar;
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Down, 1)) 
    {
        if(currentChar > 'a')
            currentChar--;
        else 
            currentChar = 'z';
        input.at(cursorIdx) = currentChar;
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Up, 0) || 
            isNavigationEventTriggered(NavigationEvent::FnNavigationRight)) 
    {
        cursorIdx++;
        if (cursorIdx >= int(input.size())) {
            input.push_back('a');
        } 
        currentChar = input.at(cursorIdx);
    }
    else if(isValueChangeEventTriggered(ValueChangeEvent::Down, 0)) 
    {
        if(cursorIdx > 0)
            cursorIdx--;
        currentChar = input.at(cursorIdx);
    }
    else if (isNavigationEventTriggered(NavigationEvent::FnNavigationLeft))
    {
        if(cursorIdx > 0) {
            input.erase(cursorIdx, 1);
            cursorIdx--;
            currentChar = input.at(cursorIdx);
        }
    }


    int padding = 20;
    int margin = 10;
    glm::uvec2 dialogPos = glm::uvec2(padding, padding);
    glm::uvec2 dialogSize = glm::uvec2(m_stbRenderer.width()  - padding * 2, 
                                       m_stbRenderer.height() - padding * 2);

    m_stbRenderer.drawRectNEW(dialogPos, dialogSize, {.color = COLOR::BLACK, .isFilled = true});
    m_stbRenderer.drawRectNEW(dialogPos, dialogSize, {.color = COLOR::WHITE, .isFilled = false});
    
    m_x = dialogPos.x + margin;
    m_y = dialogPos.y + margin;
    
    BDF::TextStyle titleStyle = BDF::TEXTSTYLE::MENU_TITLE;
    titleStyle.align = TextAlign::LEFT;
    m_stbRenderer.drawTextBdf(title, glm::vec2(m_x, m_y), titleStyle);
    Spacer(50);

    UI::BeginListElement();
    bool focused = (*m_focusedIdxPtr) == m_listSize;

    BDF::TextStyle inputStyle = BDF::TEXTSTYLE::ROOT_MENU_ITEM;
    inputStyle.align = TextAlign::LEFT;
    inputStyle.color = COLOR::YELLOW;
    TextStyle(inputStyle);

    glm::uvec2 inputTextPos = glm::vec2(m_x, m_y);
    m_stbRenderer.drawTextBdf(input, inputTextPos, m_currentTextStyle );
    UI::EndListElement();
    
    if(focused)
    {
        // draw a rect at the position of cursorIdx:
        glm::uvec2 cursorPosPx = glm::uvec2(inputTextPos.x + m_stbRenderer.getTextWidth(input.substr(0, cursorIdx), m_currentTextStyle), 
                                            inputTextPos.y - 2);
        glm::uvec2 cursorSizePx= glm::uvec2(m_stbRenderer.getTextWidth(std::string(1, currentChar), m_currentTextStyle),
                                            m_currentTextStyle.lineHeight * 1.7);
        m_stbRenderer.drawRectNEW(cursorPosPx, cursorSizePx, {.isInverted=true});
    }

}

void UI::ShowDialog(std::string title, std::string subtitle)
{
    if (!m_focusedIdxPtr) return;

    int padding = 20;
    int margin = 10;
    glm::uvec2 dialogPos = glm::uvec2(padding, padding);
    glm::uvec2 dialogSize = glm::uvec2(m_stbRenderer.width()  - padding * 2, 
                                       m_stbRenderer.height() - padding * 2);

    m_stbRenderer.drawRectNEW(dialogPos, dialogSize, {.color = COLOR::BLACK, .isFilled = true});
    m_stbRenderer.drawRectNEW(dialogPos, dialogSize, {.color = COLOR::WHITE, .isFilled = false});
    
    m_x = dialogPos.x + margin;
    m_y = dialogPos.y + margin;
    
    BDF::TextStyle titleStyle = BDF::TEXTSTYLE::MENU_TITLE;
    titleStyle.align = TextAlign::LEFT;
    m_stbRenderer.drawTextBdf(title, glm::vec2(m_x, m_y), titleStyle);
    if(subtitle != "") {
        m_y += m_lineHeight * 1.5;
        titleStyle.font = BDF::TEXTSTYLE::MENU_ITEM.font;
        m_stbRenderer.drawTextBdf(subtitle, glm::vec2(m_x, m_y), titleStyle);
        Spacer(20);  
    } else {
        Spacer(50);  
    }
}


void UI::ButtonMatrixWidget(std::vector<std::pair<char, Color>> buttonTexts)
{
    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();
    //m_stbRenderer.clear();
    
    int quadPadding = 2;
    int quadSize = (width / (buttonTexts.size() / 2)) - quadPadding;
    // int fontSize = 16;
    for(size_t i = 0; i < buttonTexts.size(); i++)
    {
        int x = (quadPadding / 2) + (i % 8) * (quadSize + quadPadding);
        int y = (height/2 - quadSize / 2) + (i / 8) * (quadSize + quadPadding + 2);

        if (buttonTexts[i].second == COLOR::BLACK) {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, Color(30, 30, 30));
            m_stbRenderer.drawTextBdf(std::string(1, static_cast<char>(buttonTexts[i].first)), glm::vec2(x + 4, y + 3), BDF::TEXTSTYLE::MENU_ITEM);
        }
        else {
            m_stbRenderer.drawRect(x, y, quadSize, quadSize, buttonTexts[i].second);
            m_stbRenderer.drawTextBdf(std::string(1, static_cast<char>(buttonTexts[i].first)), glm::vec2(x + 4, y + 3), BDF::TEXTSTYLE::MENU_ITEM);
            // m_stbRenderer.drawEmptyRect(x, y, quadSize, quadSize, Color(30, 30, 30));
            m_stbRenderer.drawRectNEW(glm::vec2(x, y), glm::vec2(quadSize, quadSize), DrawStyle{Color(30, 30, 30), false, 2, AnchorPoint::LEFT_TOP});
        }
    }
}

void UI::BankInfoWidget(int bank)
{
    int width = m_stbRenderer.width();
    int height = m_stbRenderer.height();
    // m_stbRenderer.clear();
    
    int quadPadding = 5;
    int quadSize = (width / BANK_COUNT) - quadPadding;
    // int fontSize = 16;
    m_stbRenderer.drawRectNEW(glm::vec2(0, height - quadSize - 10), glm::vec2(width, quadSize + 10), DrawStyle{COLOR::BLACK, true, 0, AnchorPoint::LEFT_TOP});
    for(size_t i = 0; i < BANK_COUNT; i++)
    {
        int x = quadPadding / 2 + i * (quadSize + quadPadding);
        int y = height - quadSize - 8;
        if(bank == int(i)) {
            m_stbRenderer.drawRectNEW(glm::vec2(x, y), glm::vec2(quadSize, quadSize), DrawStyle{COLOR::WHITE, true, 2, AnchorPoint::LEFT_TOP});
            BDF::TextStyle textStyle = BDF::TEXTSTYLE::MENU_ITEM;
            textStyle.color = COLOR::BLACK;
            m_stbRenderer.drawTextBdf(std::string(1, static_cast<char>(i + 65)),  glm::vec2(x + 4, y + 3), textStyle);
        } else {
            m_stbRenderer.drawRectNEW(glm::vec2(x, y), glm::vec2(quadSize, quadSize), DrawStyle{COLOR::WHITE, false, 2, AnchorPoint::LEFT_TOP});
            m_stbRenderer.drawTextBdf(std::string(1, static_cast<char>(i + 65)),  glm::vec2(x + 4, y + 3), BDF::TEXTSTYLE::MENU_ITEM);
        }
    }
}

bool UI::Action(const std::string& label)
{
    if (!m_focusedIdxPtr) return false;
    bool keyPressed = isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0) ||
                      isNavigationEventTriggered(NavigationEvent::Type::NavigationRight);
    bool focused = ((*m_focusedIdxPtr) == m_listSize);

    Text(label);
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
        m_stbRenderer.drawRectNEW(glm::vec2(m_x, m_y + 2), glm::vec2(7, 7), DrawStyle{COLOR::WHITE, false, 1, AnchorPoint::LEFT_TOP});
    }
    m_x += m_listPaddingLeft;
    Text(label);
    m_x -= m_listPaddingLeft;

    return checked != oldChecked;
}

bool UI::RadioButton(const std::string& label, bool active, bool* auxFunctionTriggered)
{
    if (!m_focusedIdxPtr) return false;
    bool keyPressed = false;

    if (isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0) ||
        isNavigationEventTriggered(NavigationEvent::Type::NavigationRight)) {
        keyPressed = true;
    }

    bool focused = ((*m_focusedIdxPtr) == m_listSize);

    if(focused && (isNavigationEventTriggered(NavigationEvent::Type::FnNavigationRight) ||
                   isValueChangeEventTriggered(ValueChangeEvent::Type::Up, 0)))  {
        if(auxFunctionTriggered != nullptr)
            *auxFunctionTriggered = true;
    }
    
    bool wasTriggered = false;
    if (focused && !active && keyPressed) {
        wasTriggered = true;
    }

    if(active) m_stbRenderer.drawRect(m_x, m_y + 2, 7, 7, COLOR::WHITE);
    m_x += m_listPaddingLeft;
    Text(label);
    m_x -= m_listPaddingLeft;

    return wasTriggered;
}

bool UI::SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue, int step, std::vector<std::string> optionNames)
{
    bool hasChanged = false;
    if (!m_focusedIdxPtr) return false;
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

    std::string newLabel;
    if ((0 <= value) && (value < int(optionNames.size()))) {
        newLabel = label + ": " + optionNames[value];
    }
    else {
        newLabel = label + ": " + std::to_string(value);
    }
    Text(newLabel);
    return hasChanged;
}

bool UI::SpinBoxFloat(const std::string& label, float& value, float minValue, float maxValue, float step)
{
    bool hasChanged = false;
    if (!m_focusedIdxPtr) return false;
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
    if (!m_focusedIdxPtr) return false;
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

void UI::PlanePreviewWidget(std::vector<PlaneSettings>& planes, int& selectedPlane, PlanePreviewStyle style)
{
    if (selectedPlane < 0) return;
    
    float rectWidth;
    float rectHeight;
    float centerX;
    float centerY;
    float rectCenterX[2];
    float aspectRatio = 16.0/9.0f; // todo: get aspect ratio from screen(s)
    int polygonThickness = 2;

    if (style == PLANE_PREVIEW_SMALL) 
    {
        m_y = m_screenPaddingTop;
        int boxHeight =  m_stbRenderer.getFontLineHeight(BDF::TEXTSTYLE::MENU_TITLE) + 8;
        rectHeight = boxHeight;
        rectWidth = float(rectHeight) * aspectRatio;
        float spacing = 7.0f;
        float paddingRight = 15.0f;
        centerX = float(m_stbRenderer.width() - rectWidth - spacing * 2.0f - paddingRight);
        rectCenterX[0] = centerX - rectWidth / 1.75f - spacing;
        rectCenterX[1] = centerX + rectWidth / 1.75f + spacing;
        centerY = m_y + rectHeight / 2.0f;
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES) 
    {
        m_y += 10;
        rectWidth = float(m_stbRenderer.width()) / 3.0f;
        rectHeight = float(rectWidth) / aspectRatio;
        centerX = float(m_stbRenderer.width()) / 2.0f;
        rectCenterX[0] = centerX - rectWidth / 1.75f;
        rectCenterX[1] = centerX + rectWidth / 1.75f;
        centerY = m_y + rectHeight / 2.0f;
    }

    rectWidth = std::round(rectWidth);
    rectHeight = std::round(rectHeight);
    centerY = std::round(centerY);
    rectCenterX[0] = std::round(rectCenterX[0]);
    rectCenterX[1] = std::round(rectCenterX[1]);

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
    DrawStyle drawStyle = DrawStyle{COLOR::WHITE, false, 1, AnchorPoint::CENTER_CENTER}; 
    m_stbRenderer.drawRectNEW(glm::vec2(rectCenterX[0], centerY), glm::vec2(rectWidth, rectHeight), drawStyle);
    m_stbRenderer.drawRectNEW(glm::vec2(rectCenterX[1], centerY), glm::vec2(rectWidth, rectHeight), drawStyle);

    
    // draw planes
    Color colors[] = {COLOR::PLANE_0, COLOR::PLANE_1, COLOR::PLANE_2, COLOR::PLANE_3};
    float opacity = style == PLANE_PREVIEW_VERTICES ? 0.3f : 0.0f;
    
    if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES)
    {
        // draw all planes but the selected
        int i = 0;
        for(PlaneShape p : correctedPlanes) {
            if(p.vertices.size() >= 4){
                if (i != selectedPlane) {
                    Color color = (i == selectedPlane) ? colors[i] : COLOR::GREY;
                    opacity = style == PLANE_PREVIEW_VERTICES ? 0.3f : 0.0f;
                    m_stbRenderer.setBoundingBox(glm::vec2(rectCenterX[p.hdmiId], centerY), glm::vec2(rectWidth, rectHeight), AnchorPoint::CENTER_CENTER, opacity);
                    m_stbRenderer.drawPolygonNEW(p.vertices, DrawStyle{color, false, polygonThickness, AnchorPoint::LEFT_TOP});
                }
                i++;
            }
        }
    }
    // draw selected plane on top
    PlaneShape p = correctedPlanes[selectedPlane];
    Color color = colors[selectedPlane];
    if(p.vertices.size() >= 4)
    {
        m_stbRenderer.setBoundingBox(glm::vec2(rectCenterX[p.hdmiId], centerY), glm::vec2(rectWidth, rectHeight), AnchorPoint::CENTER_CENTER, opacity);
        m_stbRenderer.drawPolygonNEW(p.vertices, DrawStyle{color, false, polygonThickness, AnchorPoint::LEFT_TOP});
    }
    m_stbRenderer.resetBoundingBox();


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
            
        DrawStyle drawStyle;
        drawStyle.color = i == selectedPlane ? colors[i] : COLOR::WHITE;
        drawStyle.strokeWidth = 2;
        if(p.hdmiId == 0)
        {
            int x = rectCenterX[0] - rectWidth/2 - 5;
            m_stbRenderer.drawLineNEW(glm::vec2(x, y), glm::vec2(x - layerlineLength, y), drawStyle);
        }
        else if(p.hdmiId == 1)
        {
            int x = rectCenterX[1] + rectWidth/2 + 5;
            m_stbRenderer.drawLineNEW(glm::vec2(x, y), glm::vec2(x + layerlineLength, y), drawStyle);
        }
        i++;
    }

    
    // draw plane indices as numbers
    if (style == PLANE_PREVIEW_SMALL) 
    {
        // draw only currently selected plane index between the two rectangles
        
        BDF::TextStyle textStyle = BDF::TEXTSTYLE::MENU_ITEM;
        uint textHeight = m_stbRenderer.getFontLineHeight(textStyle);
        uint textWidth = m_stbRenderer.getTextWidth(std::to_string(selectedPlane + 1), textStyle);
        textStyle.color = colors[selectedPlane];
        // todo: use icons instead of text numbers?
        m_stbRenderer.drawTextBdf(std::to_string(selectedPlane + 1), 
                                glm::vec2(centerX - textWidth / 4, 
                                          centerY - textHeight / 2), 
                                textStyle);
    }
    else if (style == PLANE_PREVIEW_LARGE || style == PLANE_PREVIEW_VERTICES) 
    {
        // get positions of the texts/numbers to check if they are too close to each others
        std::unordered_map<int, glm::vec2> textPositions;
        textPositions.reserve(correctedPlanes.size());
        for(size_t i = 0; i < correctedPlanes.size(); i++)
        {
            PlaneShape p = correctedPlanes[i];
            glm::vec2 polygonCenter = {0.0f, 0.0f};
            for(std::size_t j = 0; j < p.vertices.size(); j++) {
                polygonCenter += p.vertices[j];
            }
            polygonCenter /= p.vertices.size();
            textPositions.insert({int(i), polygonCenter});
        }

        // check the positions and move if necessary
        const int digitWidthPx = 16; //m_stbRenderer.getTextWidth("4", textStyle);
        const int digitHeightPx = 24;
        const int offsetX = digitWidthPx + 2;
        const float offsetHalfX = offsetX / 2.0f;
        bool changed = true;
        int pass = 0;
        while (changed && pass++ < 4)
        {
            changed = false;
            const int n = static_cast<int>(correctedPlanes.size());
            for (int a = 0; a < n; a++)
            {
                for (int b = a + 1; b < n; b++)
                {
                    const float dx = std::fabs(textPositions[a].x - textPositions[b].x);
                    const float dy = std::fabs(textPositions[a].y - textPositions[b].y);
                    if (dx < static_cast<float>(digitWidthPx) && dy < static_cast<float>(digitHeightPx))
                    {
                        // Move them symmetrically around their current midpoint
                        if (textPositions[a].x <= textPositions[b].x) {
                            textPositions[a].x -= offsetHalfX;
                            textPositions[b].x += offsetHalfX;
                        } else {
                            textPositions[a].x += offsetHalfX;
                            textPositions[b].x -= offsetHalfX;
                        }
                        changed = true;
                    }
                }
            }
        }

        // draw plane indices in the center of the polygons
        for(const auto& pos : textPositions) 
        {        
            BDF::TextStyle textStyle = BDF::TEXTSTYLE::MENU_ITEM;
            uint textHeight = m_stbRenderer.getFontLineHeight(BDF::TEXTSTYLE::MENU_ITEM);
            // uint textWidth = m_stbRenderer.getTextWidth(std::to_string(selectedPlane + 1), BDF::TEXTSTYLE::MENU_ITEM);
            textStyle.color = pos.first == selectedPlane ? colors[pos.first] : COLOR::GREY;
            m_stbRenderer.drawTextBdf(std::to_string(pos.first + 1), 
                                    glm::vec2(pos.second.x, 
                                    pos.second.y - textHeight / 1.75f), 
                                    textStyle);
        }
    }

    // handle vertex selection and highlight selected vertex    
    if(style == PLANE_PREVIEW_VERTICES)
    {
        BeginListElement();

        int selectedVertex = *m_focusedIdxPtr - CurrentListSize();
        if( selectedVertex >= 0 && selectedVertex < int(correctedPlanes.size()))    
        {
            // highlight selected vertex
            PlaneShape p = correctedPlanes[selectedPlane];
            int i = p.vertices.size() - selectedVertex - 1; // (vertices are in reverse order)
            m_stbRenderer.drawImageNEW(m_gizmoImageBuffer, p.vertices[i] - glm::vec2(9.0,9.0));

            // change vertex position on plane
            bool hasChanged = false;
            float diffX = 0;
            float diffY = 0;
            float step = 0.01f;
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
            if(hasChanged)
            {
                int vertexCount = planes[selectedPlane].coords.size() - 1;
                if(vertexCount > 0 ) {
                    planes[selectedPlane].coords[vertexCount - selectedVertex].x += diffX;
                    planes[selectedPlane].coords[vertexCount - selectedVertex].y += diffY;
                }
            }
        }
        EndListElement(4);
    }

    SetElementLineHeight(rectHeight);
}

void UI::AnimationFrameWidget(const ImageBuffer& image, int& frameIndex)
{
    if (!image.isValid) return;

    int tilesX = 10;
    int tilesY = 10;
    int srcPosX = 160 * (frameIndex % tilesX);
    int srcPosY = 90 * (frameIndex / tilesY);
    // printf("MediaPreview frame %d:  srcPosX: %d, srcPosY: %d\n",m_mediaPreviewFrameIndex, srcPosX, srcPosY);

    m_stbRenderer.drawSubImage(image, 
                              glm::uvec2(156, 96),    // destPos
                              glm::uvec2(srcPosX, srcPosY),     // srcPos
                              glm::uvec2(160, 90)); // srcSize
    
    frameIndex++;
    frameIndex %= (tilesX*tilesY);
}

void UI::savePNG(const std::string& filename){
    m_stbRenderer.savePNG(filename);
}
