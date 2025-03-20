#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <functional>
#include "Registry.h"

struct MenuEntry {
    MenuEntry(const std::string& displayName) {
        this->displayName = displayName;
    }
    virtual ~MenuEntry() {}
    virtual void process(Registry& registry) {}

    std::string displayName;
    MenuEntry* parentEntry = nullptr;
};

struct SubmenuEntry : public MenuEntry {
    SubmenuEntry(const std::string& displayName) : MenuEntry(displayName) {}
    void addSubmenuEntry(MenuEntry *submenuEntry) {
        submenus.push_back(submenuEntry);
        submenuEntry->parentEntry = this;
    }

    void deleteSubmenus() {
        for (int i = 0; i < submenus.size(); ++i) {
            delete submenus[i];
        }

        submenus.clear();
    }
    std::vector<MenuEntry*> submenus;
};

struct ButtonEntry : public MenuEntry {
    ButtonEntry(const std::string& displayName) : MenuEntry(displayName) {}

    // bool (*function)();
    std::function<void()> action;
};

struct FilesystemEntry : public SubmenuEntry {
    FilesystemEntry(const std::string& displayName, const std::string& path, int id) : SubmenuEntry(displayName)
    {
        this->path = path;
        this->id = id;
    }

    void process(Registry& registry) {
        deleteSubmenus();
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("h265") != std::string::npos) {
                    ButtonEntry *fileButton = new ButtonEntry(filename);
                    fileButton->action = [&]() {
                        registry.addEntry(id, path + "/" + filename);
                    };
                    //fileButton->action = [&]() {int debug = 1;};
                    submenus.push_back(fileButton);
                }
            }
        }
    }

    std::string path;
    int id;
};
