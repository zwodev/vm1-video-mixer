#pragma once

#include "AudioDevice.h"
#include <SDL3/SDL.h>
#include <vector>
#include <memory>


class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

public:
    void initialize();
    void finalize();
    AudioDevice* audioDevice(int index);

private: 


private:
    std::vector<std::unique_ptr<AudioDevice>> m_audioDevices; 
};
