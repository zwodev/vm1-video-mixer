/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <vector>
#include <map>
#include <variant>
#include <string>
#include <memory>
#include <filesystem>
#include <functional>
#include <sstream>
#include <algorithm> 

#include <glm/vec2.hpp>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <fstream>

#include "ImageBuffer.h"
#include "stb/stb_image.h"
#include "ShaderConfig.h"
#include "ScreenOptions.h"
#include "MediaPool.h"
#include "VM1DeviceDefinitions.h"

class InputConfig
{
public:
    InputConfig() = default;
    virtual ~InputConfig() = 0;
    virtual std::unique_ptr<InputConfig> copy_unique() = 0;

    bool isActive = false;
    int planeId = 0;
    int playerId = -1;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(planeId)
        );
    }
};
inline InputConfig::~InputConfig() = default;

class VideoInputConfig : public InputConfig
{
public:
    VideoInputConfig() = default;
    ~VideoInputConfig() = default;
    std::unique_ptr<InputConfig> copy_unique() override {
        return std::make_unique<VideoInputConfig>(*this);
    }

    std::string fileName;
    bool looping = false;
    bool backwards = false;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::base_class<InputConfig>(this),
            CEREAL_NVP(fileName),
            CEREAL_NVP(looping),
            CEREAL_NVP(backwards)
        );
    }
};

class HdmiInputConfig : public InputConfig
{
public:
    HdmiInputConfig() = default;
    ~HdmiInputConfig() = default;
    std::unique_ptr<InputConfig> copy_unique() override {
        return std::make_unique<HdmiInputConfig>(*this);
    }

    int hdmiPort = 0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::base_class<InputConfig>(this), 
            CEREAL_NVP(hdmiPort)
        );
    }
};

class ShaderInputConfig : public InputConfig
{
public:
    ShaderInputConfig() = default;
    ~ShaderInputConfig() = default;
    std::unique_ptr<InputConfig> copy_unique() override {
        return std::make_unique<ShaderInputConfig>(*this);
    }

    std::string fileName;
    ShaderConfig shaderConfig;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            cereal::base_class<InputConfig>(this),
            CEREAL_NVP(fileName),
            CEREAL_NVP(shaderConfig)
        );
    }
};

CEREAL_REGISTER_TYPE(VideoInputConfig);
CEREAL_REGISTER_TYPE(HdmiInputConfig);
CEREAL_REGISTER_TYPE(ShaderInputConfig);
CEREAL_REGISTER_POLYMORPHIC_RELATION(InputConfig, VideoInputConfig)
CEREAL_REGISTER_POLYMORPHIC_RELATION(InputConfig, HdmiInputConfig)
CEREAL_REGISTER_POLYMORPHIC_RELATION(InputConfig, ShaderInputConfig)


// TODO: Could be simplified with templates
class InputMappings
{
public:
    InputMappings() {}
    ~InputMappings() {}

    void removeConfig(int id, bool clearStaged = false)
    {
        m_activeSlots.erase(id);
        if(clearStaged)
            m_stagedSlots.erase(id);
    }

    void stageInputConfig(int id, std::unique_ptr<InputConfig> inputConfig)
    {
        if (id < 0) return;
        m_stagedSlots[id] = std::move(inputConfig);
        // if (m_activeSlots.contains(id)) {
        //     m_stagedSlots[id]->planeId = m_activeSlots[id]->planeId;
        // }
    }

    void setFocusedMediaSlot(int mediaSlotId)
    {
        if(mediaSlotId >= 0) 
        {
            focusedBank = mediaSlotId / MEDIA_BUTTON_COUNT;
            focusedMediaButton = mediaSlotId % MEDIA_BUTTON_COUNT;
        } 
        else
        {
            focusedBank = 0;
            focusedMediaButton = -1;    
        }
    }

    inline int getFocusedMediaSlot()
    {
        if (focusedMediaButton >= 0)
        {
            return focusedBank * MEDIA_BUTTON_COUNT + focusedMediaButton;
        }
        else
        {
            return -1;
        }
    }

    InputConfig* getFocusedInputConfig(bool staged = false)
    {
        return getInputConfig(getFocusedMediaSlot(), staged);
    }
    
    inline std::string focusedMediaButtonName()
    {
        if(focusedMediaButton < 0) return "";
        return std::string(1, focusedBank + 65) + std::to_string(focusedMediaButton + 1);
    }

    InputConfig* activateInputConfig(int id)
    {   
        InputConfig* inputConfig = nullptr; 
        if (m_stagedSlots.contains(id)) {
            m_activeSlots[id] = std::move(m_stagedSlots[id]->copy_unique());
            inputConfig = m_activeSlots[id].get();
        }

        return inputConfig;
    }

    InputConfig* getInputConfig(int id, bool staged = false)
    {
        if (id < 0) return nullptr;

        InputConfig *inputConfig = nullptr;

        if (staged && m_stagedSlots.find(id) != m_stagedSlots.end())
        {
            inputConfig = m_stagedSlots[id].get();
        }
        else if (m_activeSlots.find(id) != m_activeSlots.end())
        {
            inputConfig = m_activeSlots[id].get();
        }

        return inputConfig;
    }

    VideoInputConfig* getVideoInputConfig(int id, bool staged = false)
    {
        InputConfig *inputConfig = getInputConfig(id, staged);
        if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            return videoInputConfig;
        }

        return nullptr;
    }

    HdmiInputConfig* getHdmiInputConfig(int id, bool staged = false)
    {
        InputConfig *inputConfig = getInputConfig(id, staged);
        if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
        {
            return hdmiInputConfig;
        }

        return nullptr;
    }

    ShaderInputConfig* getShaderInputConfig(int id, bool staged = false)
    {
        InputConfig *inputConfig = getInputConfig(id, staged);
        if (ShaderInputConfig *shaderInputConfig = dynamic_cast<ShaderInputConfig *>(inputConfig))
        {
            return shaderInputConfig;
        }

        return nullptr;
    }

    std::vector<int> activeSlotIds() const {
        std::vector<int> ids;
        ids.reserve(m_activeSlots.size());
        for (const auto &kv : m_activeSlots) {
            ids.push_back(kv.first);
        }

        return ids;
    } 

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            // CEREAL_NVP(focusedBank),
            cereal::make_nvp("media_slots", m_stagedSlots)
        );
    }

public:
    int focusedPlane = 0;
    int focusedMediaButton = -1;
    int focusedBank = 0;
    // int focusedMediaSlot = -1;

private:
    std::map<int, std::unique_ptr<InputConfig>> m_stagedSlots;
    std::map<int, std::unique_ptr<InputConfig>> m_activeSlots;
};


namespace glm {
    template <class Archive>
    void serialize(Archive &ar, glm::vec2 &v) {
        ar(v.x, v.y);
    }
}

struct PlaneSettings
{
    enum BlendMode {
        BM_None,
        BM_Alpha,
        BM_Multiply
    };

    void resetMapping() {
        coords = { glm::vec2(-1.0f, -1.0f),   // bottom left
                   glm::vec2(1.0f, -1.0f),    // bottom right
                   glm::vec2(1.0f,  1.0f),    // top right
                   glm::vec2(-1.0f,  1.0f) }; // top left
        rotation = 0.0f;
        scale = 1.0f;
        scaleXY = {1.0f, 1.0f};
        translation = {0.0f, 0.0f};
    }


    int hdmiId = 0;
    BlendMode blendMode = BlendMode::BM_None;
    float opacity = 1.0f;
    bool useFaderForOpacity = false;
    ShaderConfig shaderConfig;
    std::string extShaderFilename;

    // Mapping
    std::vector<glm::vec2> coords = { glm::vec2(-1.0f, -1.0f),   // bottom left
                                      glm::vec2(1.0f, -1.0f),    // bottom right
                                      glm::vec2(1.0f,  1.0f),    // top right
                                      glm::vec2(-1.0f,  1.0f) }; // top left

    int rotation = 0;
    float scale = 1.0f;
    glm::vec2 scaleXY = {1.0f, 1.0f};
    glm::vec2 translation = {0.0f, 0.0f};
    
    template <class Archive>
    void serialize(Archive &ar)
    {
        ar(
            CEREAL_NVP(hdmiId),
            CEREAL_NVP(blendMode),
            CEREAL_NVP(opacity),
            CEREAL_NVP(shaderConfig),
            CEREAL_NVP(extShaderFilename),
            CEREAL_NVP(coords),
            // CEREAL_NVP(rotation),
            CEREAL_NVP(scale),
            // CEREAL_NVP(scaleXY),
            CEREAL_NVP(translation)
        );
    }
};

struct Settings
{
    struct KioskSettings {
        bool enabled = false;
        int resetTime = 60; // seconds

        template <class Archive>
        void serialize(Archive& ar) {
            ar(
                CEREAL_NVP(enabled),
                CEREAL_NVP(resetTime)
            );
        }
    };

    // Saved
    bool showUI = false;
    bool defaultLooping = true;
    int fadeTime = 2;
    int volume = 10;
    bool useUvcCaptureDevice = true;
    int rotarySensitivity = 5;
    std::string serialDevice = "/dev/ttyACM0";
    int autoPlayOnHDMI0 = -1;
    int autoPlayOnHDMI1 = -1;
    KioskSettings kiosk;
    bool useFader = false;
    bool useRotaryAsFader = false;
    ScreenRotation hdmiRotation0 = ScreenRotation::SR_Rotate_0;
    ScreenRotation hdmiRotation1 = ScreenRotation::SR_Rotate_0;

    // Volatile
    bool isProVersion = false;
    bool isHdmiOutputReady = false;
    bool isHdmiInputReady = false;
    std::string captureDevicePath = "";
    std::vector<std::string> hdmiOutputs = std::vector<std::string>(2, std::string());
    std::vector<std::string> hdmiInputs = std::vector<std::string>(2, std::string());
    
    float currentTime = 0.0f;
    float analog0 = 0.5f;
    int32_t rotary = 0;

    template <class Archive>
    void serialize(Archive &ar)
    {
        ar(
            CEREAL_NVP(showUI), 
            CEREAL_NVP(defaultLooping), 
            CEREAL_NVP(fadeTime),
            CEREAL_NVP(useFader),
            CEREAL_NVP(useRotaryAsFader),
            CEREAL_NVP(volume), 
            CEREAL_NVP(rotarySensitivity),
            CEREAL_NVP(useUvcCaptureDevice),
            CEREAL_NVP(serialDevice),
            CEREAL_NVP(autoPlayOnHDMI0),
            CEREAL_NVP(autoPlayOnHDMI1),
            CEREAL_NVP(kiosk),
            CEREAL_NVP(hdmiRotation0),
            CEREAL_NVP(hdmiRotation1)
        );
    }
};

class Registry
{
public:
    Registry() { load(); }
    ~Registry() {}

    Settings& settings() { return m_settings; }
    InputMappings& inputMappings() { return m_inputMappings; }
    MediaPool& mediaPool() { return m_mediaPool; }
    auto& planes() { return m_planes; }

    void update(float deltaTime) {
        m_timeSinceLastHash += deltaTime;
        if (m_timeSinceLastHash >= m_autosaveInterval) {
            m_timeSinceLastHash -= m_autosaveInterval;
            size_t currentHash = hash();
            if (currentHash != m_lastHash) {
                m_lastHash = currentHash;
                if(!settings().kiosk.enabled) {
                    save();
                } else {
                    printf("Kiosk mode enabled, skipped saving the registry.\n");
                }
            }
        }
    }

    void save()
    {
        try {
            std::ofstream file("vm1-registry.json");
            cereal::JSONOutputArchive archive(file);
            archive(m_inputMappings, m_settings, m_planes);
            printf("Successfully saved registry!\n");
        }
        catch(const std::exception &e) {
            std::cerr << "Serialization failed: " << e.what() << std::endl;
        }
        
    }

    void load()
    {
        try
        {
            std::ifstream file("vm1-registry.json");
            cereal::JSONInputArchive archive(file);
            archive(m_inputMappings, m_settings, m_planes);
            m_lastHash = hash();
        }
        catch (const std::exception &e)
        {
            std::cerr << "Deserialization failed: " << e.what() << std::endl;
            // Handle error, e.g., fallback, alert user, etc.
        }
    }

private:
    std::size_t hash()
    {
        std::stringstream stream;
        cereal::BinaryOutputArchive archive(stream);
        archive(m_inputMappings, m_settings, m_planes);
        std::string serialized = stream.str();
        return std::hash<std::string>{}(serialized);

        // Syntax explanation: Curly brackets create a temporary functor.
        // Equivalent to this:
        // std::hash<std::string> hasher;
        // size_t hash = hasher(serialized);
    }

private:
    float m_autosaveInterval = 5.0f;
    float m_timeSinceLastHash = 0.0f;
    size_t m_lastHash = 0;

    Settings m_settings;
    InputMappings m_inputMappings;
    MediaPool m_mediaPool;
    std::vector<PlaneSettings> m_planes = std::vector<PlaneSettings>(PLANE_COUNT);
};
