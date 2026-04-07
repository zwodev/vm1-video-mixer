/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "AudioSystem.h"
#include <stdio.h>

AudioSystem::AudioSystem()
{
}

AudioSystem::~AudioSystem()
{
}

void AudioSystem::initialize()
{
    int count = 0;
    SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);
    if (devices && count > 0) {
        SDL_AudioDeviceID deviceId = devices[0];
        const char *name = SDL_GetAudioDeviceName(deviceId);
        printf("Device %u: %s\n", deviceId, name);
        SDL_free(devices);

        SDL_AudioSpec spec{};
        spec.freq = 48000;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 2;
 
        SDL_AudioDeviceID id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (id == 0) {
            SDL_Log("Failed to open device %u: %s\n", deviceId, SDL_GetError());
            return;
        }

        auto audioDevice = std::make_unique<AudioDevice>(id, spec);
        m_audioDevices.push_back(std::move(audioDevice));
        SDL_ResumeAudioDevice(id);
        SDL_Log("Initialized audio system.");
    }
}

void AudioSystem::finalize()
{
    m_audioDevices.clear();
}

AudioDevice* AudioSystem::audioDevice(int index)
{
    AudioDevice* audioDevice = nullptr;
    if (index >= 0 && index < int(m_audioDevices.size())) {
        audioDevice = m_audioDevices[index].get();
    }

    return audioDevice;
}

