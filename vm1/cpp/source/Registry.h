#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>
#include <functional>
#include <sstream>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/memory.hpp>
#include <fstream>

class InputConfig
{
public:
    InputConfig() = default;
    virtual ~InputConfig() = 0;

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
        ar(cereal::base_class<InputConfig>(this), fileName, looping, backwards);
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
        ar(bank, m_idsToValue);
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

private:
    std::string m_videoFilePath = "../videos/";
    std::vector<std::string> m_videoFiles;
};

struct Settings
{
    bool showUI = true;
    bool defaultLooping = true;
    int fadeTime = 2.0f;
    std::string videoFilePath = "../videos/";
    std::string serialDevice = "/dev/ttyACM0";

    template <class Archive>
    void serialize(Archive &ar)
    {
        ar(showUI, defaultLooping, fadeTime, videoFilePath, serialDevice);
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
        //printf("Time Since Last Hash: %f\n", m_timeSinceLastHash); 
        if (m_timeSinceLastHash >= m_autosaveInterval) {
            m_timeSinceLastHash -= m_autosaveInterval;
            size_t currentHash = hash();
            if (currentHash != m_lastHash) {
                m_lastHash = currentHash;
                save();
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
