#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>

class AudioDevice
{
public:
    explicit AudioDevice(SDL_AudioDeviceID deviceId, SDL_AudioSpec spec) : m_deviceId(deviceId), m_spec(spec)
    {      
        SDL_Log("Creating audio device: %d", m_deviceId);
    }

    ~AudioDevice()
    {
        // for (SDL_AudioStream* stream : m_streams) {
        //     SDL_UnbindAudioStream(stream);
        //     SDL_DestroyAudioStream(stream);
        // }
        // m_streams.clear();
        // SDL_CloseAudioDevice(m_deviceId);
    }

    SDL_AudioStream* addStream(SDL_AudioSpec srcSpec)
    {
        SDL_AudioStream* audioStream = SDL_CreateAudioStream(&srcSpec, &m_spec);
        if (audioStream) {
            if(!SDL_BindAudioStream(m_deviceId, audioStream)) {
                SDL_Log("Failed binding audio stream: %s", SDL_GetError());
            }
            m_streams.push_back(audioStream);
            SDL_ResumeAudioDevice(m_deviceId);
        }

        return audioStream;
    }

    void removeStream(SDL_AudioStream* stream)
    {
        m_streams.erase(std::find(m_streams.begin(), m_streams.end(), stream));
        SDL_UnbindAudioStream(stream);
        SDL_DestroyAudioStream(stream);
        SDL_Log("Unbind and Destroy audio stream!");
    }

    SDL_AudioDeviceID deviceId() const
    {
        return m_deviceId;
    }

    SDL_AudioSpec audioSpec() const
    {
        return m_spec;
    }

private:
    SDL_AudioDeviceID m_deviceId;
    SDL_AudioSpec m_spec;
    std::vector<SDL_AudioStream*> m_streams;
};