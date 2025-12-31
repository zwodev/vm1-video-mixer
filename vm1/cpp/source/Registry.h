#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>
#include <functional>
#include <sstream>
#include <algorithm> 

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <fstream>

#include "ImageBuffer.h"
#include "stb/stb_image.h"

enum class DisplayType {
    SSD1351_OLED = 0,      // Original 1.5" OLED display
    ILI9341_IPS_LCD = 1    // New 2.8" IPS LCD display
};

class InputConfig
{
public:
    InputConfig() = default;
    virtual ~InputConfig() = 0;

    bool isActive = false;

    template <class Archive>
    void serialize(Archive& ar) {}
};
inline InputConfig::~InputConfig() = default;

class VideoInputConfig : public InputConfig
{
public:
    VideoInputConfig() = default;
    ~VideoInputConfig() = default;

    std::string fileName;
    bool looping = false;
    bool backwards = false;

    template <class Archive>
    void serialize(Archive& ar)
    {
        // ar(cereal::base_class<InputConfig>(this), fileName, looping, backwards);
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

    int hdmiPort = 0;

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(cereal::base_class<InputConfig>(this), hdmiPort);
    }
};

CEREAL_REGISTER_TYPE(VideoInputConfig);
CEREAL_REGISTER_TYPE(HdmiInputConfig);
CEREAL_REGISTER_POLYMORPHIC_RELATION(InputConfig, VideoInputConfig)
CEREAL_REGISTER_POLYMORPHIC_RELATION(InputConfig, HdmiInputConfig)

// TODO: Could be simplified with templates
class InputMappings
{
public:
    InputMappings() {}
    ~InputMappings() {}

    void removeConfig(int id)
    {
        m_idsToValue.erase(id);
    }

    void addInputConfig(int id, std::unique_ptr<InputConfig> inputConfig)
    {
        m_idsToValue[id] = std::move(inputConfig);
    }

    InputConfig* getInputConfig(int id)
    {
        InputConfig *inputConfig = nullptr;

        if (m_idsToValue.find(id) != m_idsToValue.end())
        {
            inputConfig = m_idsToValue[id].get();
            // printf("Get -> ID: %d, Value: %s\n", id, value.c_str());
        }

        return inputConfig;
    }

    VideoInputConfig* getVideoInputConfig(int id)
    {
        InputConfig *inputConfig = getInputConfig(id);
        if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            return videoInputConfig;
        }

        return nullptr;
    }

    HdmiInputConfig* getHdmiInputConfig(int id)
    {
        InputConfig *inputConfig = getInputConfig(id);
        if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
        {
            return hdmiInputConfig;
        }

        return nullptr;
    }

    template <class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(bank),
            cereal::make_nvp("media_slots", m_idsToValue)
        );
    }

public:
    int bank = 0;

private:
    std::map<int, std::unique_ptr<InputConfig>> m_idsToValue;
};

class MediaPool
{
public:
    MediaPool()
    {
        loadQrCodeImageBuffer();
        updateVideoFiles(); // TODO: Should be updated on filesystem change
    }

    ~MediaPool() = default;

    std::vector<std::string>& getVideoFiles()
    {
        // TODO: Update only when new files are present.
        updateVideoFiles();
        return m_videoFiles;
    }

    std::string getVideoFilePath(const std::string& fileName)
    {
        return m_videoFilePath + fileName;
    }

    void updateVideoFiles()
    {
        m_videoFiles.clear();
        for (const auto &entry : std::filesystem::directory_iterator(m_videoFilePath))
        {
            if (entry.is_regular_file())
            {
                std::string filename = entry.path().filename().string();
                std::string filePath = filename;
                m_videoFiles.push_back(filePath);
            }
        }

        // sort the files by name
        std::sort(m_videoFiles.begin(), m_videoFiles.end());
    }

    void loadQrCodeImageBuffer()
    {
        int width, height, channels;
        std::string filename("data/wifi_qr.png");
        unsigned char *data = stbi_load(
            filename.c_str(),
            &width,
            &height,
            &channels,
            0);

        if (data == nullptr)
        {
            std::cerr << "Failed to load PNG: " << filename << std::endl;
            return;
        }

        std::cout << "Loaded PNG: " << filename << std::endl;
        std::cout << "Size: " << width << " x " << height << std::endl;
        std::cout << "Channels: " << channels << std::endl;

        // if (channels != 3)
        //     return;

        m_qrCodeImageBuffer = ImageBuffer(width, height, channels, reinterpret_cast<char*>(data));
    }

    const ImageBuffer& getQrCodeImageBuffer()
    {
        return m_qrCodeImageBuffer;
    }

private:
    std::string m_videoFilePath = "../videos/";
    std::vector<std::string> m_videoFiles;
    ImageBuffer m_qrCodeImageBuffer;
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
    bool useUvcCaptureDevice = false;
    int rotarySensitivity = 5;
    std::string videoFilePath = "../videos/";
    std::string serialDevice = "/dev/ttyACM0";
    int autoPlayOnHDMI0 = -1;
    int autoPlayOnHDMI1 = -1;
    KioskSettings kiosk;
    bool useFader = false;
    bool useRotaryAsFader = false;
    // DisplayType displayType = DisplayType::SSD1351_OLED;
    DisplayType displayType = DisplayType::ILI9341_IPS_LCD;

    // Volatile
    bool isProVersion = false;
    bool isHdmiOutputReady = false;
    bool isHdmiInputReady = false;
    std::string captureDevicePath = "";
    std::vector<std::string> hdmiOutputs = std::vector<std::string>(2, std::string());
    std::vector<std::string> hdmiInputs = std::vector<std::string>(2, std::string());
    float analog0 = 0;
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
            CEREAL_NVP(videoFilePath), 
            CEREAL_NVP(serialDevice),
            CEREAL_NVP(autoPlayOnHDMI0),
            CEREAL_NVP(autoPlayOnHDMI1),
            CEREAL_NVP(kiosk),
            CEREAL_NVP(displayType)
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
            archive(m_inputMappings, m_settings);
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
            archive(m_inputMappings, m_settings);
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
        archive(m_inputMappings, m_settings);
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
};
