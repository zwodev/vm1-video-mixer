/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

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
