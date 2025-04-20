#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>
#include <filesystem>


class InputConfig
{
public:
    InputConfig() {};
    virtual ~InputConfig() {}; // Should be abstract
};

class VideoInputConfig : public InputConfig
{
public:
    VideoInputConfig() {};
    ~VideoInputConfig() {};

    std::string fileName;
    bool looping = false;
    bool backwards = false;
};

struct HdmiInputConfig : public InputConfig
{
public:
    HdmiInputConfig() {};
    ~HdmiInputConfig() {};
    
    int hdmiPort = 0;
};


// TODO: Could be simplified with templates
class InputMappings 
{
public:
    InputMappings() {}
    ~InputMappings() {}

    void addInputConfig(int id, std::unique_ptr<InputConfig> inputConfig) {
        m_idsToValue[id] = std::move(inputConfig);
    }

    InputConfig* getInputConfig(int id) {
        InputConfig* inputConfig = nullptr;

        if (m_idsToValue.find(id) != m_idsToValue.end()) {
            inputConfig = m_idsToValue[id].get();
            //printf("Get -> ID: %d, Value: %s\n", id, value.c_str());
        }

        return inputConfig;
    }

    VideoInputConfig* getVideoInputConfig(int id) {
        InputConfig* inputConfig = getInputConfig(id);
        if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(inputConfig)) {
            return videoInputConfig;
        }

        return nullptr;
    }

    HdmiInputConfig* getHdmiInputConfig(int id) {
        InputConfig* inputConfig = getInputConfig(id);
        if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(inputConfig)) {
            return hdmiInputConfig;
        }

        return nullptr;
    }

private:
    std::map<int, std::unique_ptr<InputConfig>> m_idsToValue;
};

class MediaPool 
{
public:
    MediaPool() {
        updateVideoFiles(); // TODO: Should be updated on filesystem change
    }

    ~MediaPool() = default;

    std::vector<std::string>& getVideoFiles() {
        return m_videoFiles;
    }

    std::string getVideoFilePath(const std::string& fileName) {
        return m_videoFilePath + fileName;
    }

    void updateVideoFiles() {
        m_videoFiles.clear();
        for (const auto& entry : std::filesystem::directory_iterator(m_videoFilePath)) {
            if (entry.is_regular_file()) {
                std::string filename = entry.path().filename().string();
                if (filename.find("h265") != std::string::npos || filename.find("hdmi") != std::string::npos) {       
                    std::string filePath = filename;
                    m_videoFiles.push_back(filePath);
                }
            }
        }
    }

private:
    std::string m_videoFilePath = "../videos/";
    std::vector<std::string> m_videoFiles;

};

struct Settings 
{
    bool showUI = true;
    std::string videoFilePath = "../videos/";
};

class Registry 
{
public:
    Registry() {}
    ~Registry() {}

    Settings& settings() { return m_settings; }
    InputMappings& inputMappings() { return m_inputMappings; }
    MediaPool& mediaPool() { return m_mediaPool; }

private:
    Settings m_settings;
    InputMappings m_inputMappings;
    MediaPool m_mediaPool;
};