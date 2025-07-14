#pragma once

#include <iostream>
#include <string>
#include <span>
#include "Settings.h"
#include "AppEventBus.h"
#include "AppEventHandler.h"

class Gui;
class GuiStateMachine;
class Settings;

// ==================================================================

/**
 * @brief Abstract base class representing a state in the GUI state machine.
 *
 * This class defines the interface for all GUI states, providing access to
 * shared settings and GUI objects, and requiring derived classes to implement
 * input handling and UI description.
 *
 * @note This class is intended to be subclassed for specific GUI states.
 */
class GuiState
{
private:
    GuiStateMachine &stateMachine;
    GuiState *activeSubMenu = nullptr;
    GuiState *parentMenu = nullptr;

public:
    AppEventBus &appEventBus;
    Settings &settings;
    Gui &gui;

    GuiState(GuiStateMachine &machine, Settings &settings, Gui &gui, AppEventBus &appEventBus)
        : stateMachine(machine),
          settings(settings),
          gui(gui),
          appEventBus(appEventBus)
    {
    }

    ~GuiState() = default;

    void setSubmenu(GuiState *state)
    {
        activeSubMenu = state;
    }

    GuiState *getSubmenu() const
    {
        return activeSubMenu;
    }

    void setParentMenu(GuiState *state)
    {
        parentMenu = state;
    }

    GuiState *getParentMenu() const
    {
        return parentMenu;
    }

    virtual void handleInput(const char &key) = 0;
    virtual void describeUI() = 0;
};

class ISelectableMenu
{
public:
    virtual ~ISelectableMenu() = default;

    virtual void focusNext() = 0;
    virtual void focusPrevious() = 0;

    virtual size_t getFocusedIndex() const = 0;
    virtual const char *getFocusedLabel() const = 0;
    virtual void handleSelectedMenuItem() = 0;
    // virtual AppEvent getAppEventForSelection() const = 0;
};

/**
 * @brief A generic selectable menu class for enumerated types.
 *
 * This template class provides a simple menu mechanism where each menu item is represented
 * by an enumerated value and a corresponding label. It allows cycling through the menu items,
 * selecting the next or previous item, and retrieving the currently focused item and its label.
 *
 * @tparam TEnum The enumeration type representing the selectable menu items.
 */
template <typename TEnum>
class SelectableMenu : public ISelectableMenu
{
protected:
    TEnum focused = static_cast<TEnum>(0);
    const std::span<const char *const> labels;

public:
    SelectableMenu(const std::span<const char *const> labels) : labels(labels) {}

    void focusNext() override
    {
        int val = static_cast<size_t>(focused);
        if (++val >= labels.size())
            val = 0;
        focused = static_cast<TEnum>(val);
    }

    void focusPrevious() override
    {
        int val = static_cast<size_t>(focused);
        if (--val < 0)
            val = labels.size() - 1;
        focused = static_cast<TEnum>(val);
    }

    TEnum getFocusedEnum() const
    {
        return focused;
    }

    const char *getFocusedLabel() const override
    {
        return labels[static_cast<size_t>(focused)];
    }

    size_t getFocusedIndex() const override
    {
        return static_cast<size_t>(focused);
    }
};

/**
 * @class SelectableMenuDynamic
 * @brief A dynamic selectable menu implementation.
 *
 * This class implements the ISelectableMenu interface, providing functionality
 * for navigating through a dynamic list of menu labels. The menu supports
 * selecting the next or previous item, retrieving the currently focused index,
 * and obtaining the label of the focused item.
 *
 * @note The menu wraps around when navigating past the first or last item.
 *
 * @see ISelectableMenu
 */
class SelectableMenuDynamic : public ISelectableMenu
{
protected:
    size_t focused = 0;
    std::span<const std::string> labels;

public:
    SelectableMenuDynamic(const std::span<const std::string> labels) : labels(labels) {}

    void focusNext() override
    {
        if (focused == labels.size() - 1)
            focused = 0;
        else
            ++focused;
    }

    void focusPrevious() override
    {
        if (focused == 0)
            focused = labels.size() - 1;
        else
            --focused;
    }

    size_t getFocusedIndex() const override
    {
        return focused;
    }

    const char *getFocusedLabel() const override
    {
        return labels[focused].c_str();
    }

    void onEnter();
};
