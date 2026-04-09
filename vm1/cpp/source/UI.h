/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "imgui.h"
#include "FontManager.h"
#include "Registry.h"
#include "StbRenderer.h"
#include "EventBus.h"
#include "ImageBuffer.h"

#include <string>
#include <algorithm>
#include <functional>

#include <glm/vec2.hpp>

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

    enum PlanePreviewStyle
    {
        PLANE_PREVIEW_SMALL,
        PLANE_PREVIEW_LARGE,
        PLANE_PREVIEW_VERTICES
    };

    UI() = delete;
    ~UI() = default;
    UI(StbRenderer &stbRenderer, EventBus &eventBus);
    void NewFrame();
    void EndFrame();
    
    // Specialized Screen Items
    void ShowPlanePreview(std::vector<PlaneSettings> planes, int& selectedPlane, int& selectedVertex, PlanePreviewStyle style);
    void ShowAnimationFrame(const ImageBuffer& image, int& frameIndex);
    
    // Overlays
    void StartOverlay(std::function<void()> overlay);
    void ShowOverlay();
    void StopOverlay();
    void ShowPopupMessage(std::string message);
    void ShowStringInputDialog(std::string title, int& cursorIdx, std::string& input);
    
    // obsolete
    // void DrawTitle(const std::string& label);
    // void CenteredText(const std::string &label);
    
    // List
    void BeginList(int* focusedIdxPtr);
    void BeginListElement();
    void EndListElement();
    void EndList();
    int CurrentListSize();
    
    // Screens and Widgets
    void InfoScreen(int bank, int id, std::string filename);
    void ShowButtonMatrix(std::vector<std::pair<char, Color>> buttonTexts);
    void ShowBankInfo(int bank);
    
    // Generic UI Items
    void Image(const ImageBuffer& imageBuffer);
    
    bool Text(const std::string &label);
    void PlainText(const std::string &label);
    void MenuTitle(std::string menuTitle, TextAlign textAlign = TextAlign::LEFT, Color color = COLOR::WHITE);

    bool Action(const std::string& label);
    bool CheckBox(const std::string& label, bool checked);
    bool RadioButton(const std::string& label, bool active);
    bool SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue, int step = 1);
    bool SpinBoxFloat(const std::string& label, float& value, float minValue, float maxValue, float step = 0.01f);
    bool SpinBoxVec2(const std::string& label, glm::vec2& vec, float step = 0.1f);

    // Style and Layout
    void TextStyle(BDF::TextStyle textStyle);
    void TextColor(Color color);
    void pushTranslate(int x, int y);
    void popTranslate();
    void Spacer(float value = 15.0f);
    void NewLine();
    void HideElements();
    void ShowElements();
    void SetElementLineHeight(int height);

    // Navigation & Events
    void FocusNextElement();
    void FocusPreviousElement();
    bool isValueChangeEventTriggered(ValueChangeEvent::Type eventType, int id);
    bool isNavigationEventTriggered(NavigationEvent::Type eventType);
    bool isBankChangeEventTriggered(int& bankId);
    bool isMediaSlotEventTriggered(int mediaSlotId);
    bool isEditModeEventTriggered(int modeId);
    std::vector<int> getTriggeredMediaSlotIds();
    std::vector<int> getTriggeredEditButtons();
    
    // Helpers
    void savePNG(const std::string& filename);
    
private:
    void subscribeToEvents();
    
    std::vector<ValueChangeEvent> valueChangeEvents;
    std::vector<NavigationEvent> navigationEvents;
    std::vector<BankChangeEvent> bankChangeEvents;
    std::vector<MediaSlotEvent> mediaSlotEvents;
    std::vector<EditModeEvent> editModeEvents;
    
    StbRenderer& m_stbRenderer;
    EventBus& m_eventBus;
    int m_x = 0;
    int m_y = 0;
    std::vector<glm::vec2> m_translateStack;
    int m_listSize = 0;
    int m_lineHeight = 0;
    int m_menuHeight = 0;
    int m_visibleListElements = 0;
    int* m_focusedIdxPtr = 0;
    int m_firstLine = 0;
    int m_currentElementHeight = 0;
    bool m_isHidden = false;
    
    int m_textPaddingBottom = 2;
    int m_screenPaddingTop = 2;
    int m_listPaddingLeft = 10;
    int m_menuTitlePaddingTop = 4;
    int m_elementPadding = 2;
    int m_currentElementLineHeight = 0;
    int m_horizontalMargin = 4;
    
    BDF::TextStyle m_currentTextStyle = BDF::TEXTSTYLE::MENU_ITEM;
    Color m_currentColor = COLOR::WHITE;
    
    std::function<void()> m_overlay;
    int64_t m_overlayStartTimeMs = 0;
    int64_t m_overlayDurationMs = 600;
    
    static ImageBuffer m_gizmoImageBuffer;
    ImageBuffer m_mediaPreviewImageBuffer;
    
    int m_mediaPreviewFrameIndex = 0;
    std::string m_previewMediaFileNameOld;
};