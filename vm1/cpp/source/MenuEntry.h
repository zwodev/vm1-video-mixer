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
    
    void addSubmenuEntry(std::unique_ptr<MenuEntry> submenuEntry) {
        submenuEntry->parentEntry = this;
        submenus.push_back(std::move(submenuEntry));               
    }

    void deleteSubmenus() {
        submenus.clear();
    }

    std::vector<std::unique_ptr<MenuEntry>> submenus;
};

struct ButtonEntry : public MenuEntry {
    ButtonEntry(const std::string& displayName) : MenuEntry(displayName) {}

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
                    std::unique_ptr<ButtonEntry> fileButton = std::make_unique<ButtonEntry>(filename);
                    fileButton->action = [this, &registry, filename]() {
                        registry.addEntry(id, path + filename);
                    };
                    submenus.push_back(std::move(fileButton));
                }
            }
        }
    }

    std::string path;
    int id;
};
