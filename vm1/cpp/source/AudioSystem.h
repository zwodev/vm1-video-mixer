#pragma once

#include "AudioDevice.h"
#include <SDL3/SDL.h>
#include <vector>


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
    std::vector<AudioDevice> m_audioDevices; 
};
