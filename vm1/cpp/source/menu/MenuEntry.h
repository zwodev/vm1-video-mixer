/*
 * Copyright (c) 2025 Julian Jungel & Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

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
    virtual void update(Registry& registry, int id) {}

    std::string displayName;
    MenuEntry* parentEntry = nullptr;
};

struct SubmenuEntry : public MenuEntry {
    SubmenuEntry(const std::string& displayName) : MenuEntry(displayName) {}
    
    void addSubmenuEntry(std::unique_ptr<MenuEntry> submenuEntry) {
        submenuEntry->parentEntry = this;
        submenus.push_back(std::move(submenuEntry));               
    }

    std::vector<std::unique_ptr<MenuEntry>> submenus;
};

struct ButtonEntry : public MenuEntry {
    ButtonEntry(const std::string& displayName) : 
        MenuEntry(displayName) {}

    bool isChecked = false;
    std::function<void()> action;
};

// struct LiveSelectionEntry : public SubmenuEntry {
//     void update(Registry& registry, int id) {
//         submenus.clear();

//         VideoInputConfig* videoInputConfig = registry.getVideoInputConfig(id);
//         if (videoInputConfig && !videoInputConfig->fileName.empty()) {
//             fileButton->isChecked = (videoInputConfig->fileName == filePath);
//         }
        
//         fileButton->action = [&registry, id, filePath]() {
//             if (videoInputConfig) {
//                 videoInputConfig->fileName = filePath;
//                 registry.addInputConfig(id, std::unique_ptr<InputConfig>(std::move(videoInputConfig)));
//             }
//         };
//     }
// };

struct VideoSelectionEntry : public SubmenuEntry {
    VideoSelectionEntry(const std::string& displayName, const std::string& path) : SubmenuEntry(displayName)
    {
        this->path = path;
    }

    void update(Registry& registry, int id) {
        submenus.clear();
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("h265") != std::string::npos || filename.find("hdmi") != std::string::npos) {       
                    std::unique_ptr<ButtonEntry> fileButton = std::make_unique<ButtonEntry>(filename);
                    std::string filePath = path + filename;
                    VideoInputConfig* videoInputConfig = registry.inputMappings().getVideoInputConfig(id);
                    if (videoInputConfig && !videoInputConfig->fileName.empty()) {
                        fileButton->isChecked = (videoInputConfig->fileName == filePath);
                    }
                    
                    fileButton->action = [&registry, id, filePath]() {
                        auto videoInputConfig = std::make_unique<VideoInputConfig>();
                        if (videoInputConfig) {
                            videoInputConfig->fileName = filePath;
                            registry.inputMappings().addInputConfig(id, std::unique_ptr<InputConfig>(std::move(videoInputConfig)));
                        }
                    };
                    submenus.push_back(std::move(fileButton));
                }
            }
        }
    }

    std::string path;
};
