#include "AudioSystem.h"
#include <stdio.h>

AudioSystem::AudioSystem()
{
    initialize();
}

AudioSystem::~AudioSystem()
{
    finalize();
}

void AudioSystem::initialize()
{
    int count = 0;
    SDL_AudioDeviceID *devices = SDL_GetAudioPlaybackDevices(&count);
    if (devices && count > 0) {
        SDL_AudioDeviceID deviceId = devices[0];
        char *name = SDL_GetAudioDeviceName(deviceId);
        printf("Device %u: %s\n", deviceId, name);
        SDL_free(name);
        SDL_free(devices);

        SDL_AudioSpec spec = {0};
        spec.freq = 48000;
        spec.format = SDL_AUDIO_F32;
        spec.channels = 2;
 
        SDL_AudioDeviceID id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
        if (id == 0) {
            SDL_Log("Failed to open device %u: %s\n", deviceId, SDL_GetError());
            return;
        }

        AudioDevice audioDevice(id, spec);
        m_audioDevices.push_back(audioDevice);
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
    if (index < m_audioDevices.size()) {
        audioDevice = &(m_audioDevices[index]);
    }

    return audioDevice;
}