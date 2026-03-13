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
    void Image(const ImageBuffer& imageBuffer);
    bool Text(const std::string &label);
    void PlainText(const std::string &label);
    void Spacer(float value = 15.0f);
    void MenuTitle(std::string menuTitle);
    void MenuInfo(std::string menuInfo);
    void InfoScreen(int bank, int id, std::string filename);
    void ShowPopupMessage(std::string message);
    void ShowStringInputDialog(std::string title, int& cursorIdx, std::string& input);
    void ShowBankInfo(int bank);
    void ShowButtonMatrix(std::vector<std::pair<char, Color>> buttonTexts);
    bool Action(const std::string& label);
    bool CheckBox(const std::string& label, bool checked);
    bool RadioButton(const std::string& label, bool active);
    bool SpinBoxInt(const std::string& label, int& value, int minValue, int maxValue, int step = 1);
    bool SpinBoxFloat(const std::string& label, float& value, float minValue, float maxValue, float step = 0.01f);
    bool SpinBoxVec2(const std::string& label, glm::vec2& vec, float step = 0.1f);
    void PlanePreview(std::vector<PlaneSettings> planes, int& selectedPlane, PlanePreviewStyle style);

    bool isValueChangeEventTriggered(ValueChangeEvent::Type eventType, int id);
    bool isNavigationEventTriggered(NavigationEvent::Type eventType);
    bool isBankChangeEventTriggered(int& bankId);
    bool isMediaSlotEventTriggered(int mediaSlotId);
    bool isEditModeEventTriggered(int modeId);

    void savePNG(const std::string& filename);

    std::vector<int> getTriggeredMediaSlotIds();
    std::vector<int> getTriggeredEditButtons();

    
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