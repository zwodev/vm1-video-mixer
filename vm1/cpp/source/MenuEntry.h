#pragma once
#include <string>
#include <vector>

struct MenuEntry {
    MenuEntry(const std::string& displayName) {
        this->displayName = displayName;
    }

    std::string displayName;
    MenuEntry* parentEntry = nullptr;

    virtual ~MenuEntry(){};
};

struct SubmenuEntry : public MenuEntry {
    SubmenuEntry(const std::string& displayName) : MenuEntry(displayName) {}
    void addSubmenuEntry(MenuEntry *submenuEntry) {
        submenus.push_back(submenuEntry);
        submenuEntry->parentEntry = this;
    }
    std::vector<MenuEntry*> submenus;
};

struct ButtonEntry : public MenuEntry {
    ButtonEntry(const std::string& displayName) : MenuEntry(displayName) {}

    bool (*function)();
};