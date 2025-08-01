#pragma once

#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include <memory>

class AudioStream
{
public:
    AudioStream(SDL_AudioDeviceID deviceId, SDL_AudioSpec spec) : m_deviceId(deviceId), m_dstSpec(spec)
    {
    }

    ~AudioStream()
    {
        if (m_sdlStream) {
            SDL_UnbindAudioStream(m_sdlStream);
            SDL_DestroyAudioStream(m_sdlStream);
            m_sdlStream = nullptr;
        }
    }

    void setVolume(float volume) {
        if (volume > 1.0f) m_volume = 1.0f;
        else if (volume < 0.0f) m_volume = 0.0f;
        else m_volume = volume;
    }

    void printSpec() 
    {
        SDL_Log("Spec: %d %d %d", m_dstSpec.format, m_dstSpec.channels, m_dstSpec.freq);
    }

    void createAndBind(SDL_AudioSpec spec)
    {        
        SDL_AudioStream* audioStream = SDL_CreateAudioStream(&spec, &m_dstSpec); 
        if (audioStream) {
            if(SDL_BindAudioStream(m_deviceId, audioStream)) {
                m_sdlStream = audioStream;
                m_srcSpec = spec;
                SDL_Log("Create and bind audio stream!");
            }
            else {
                SDL_Log("Failed binding audio stream: %s", SDL_GetError());
            }
            SDL_ResumeAudioDevice(m_deviceId);
        }
        else {
            SDL_Log("Failed creating audio stream: %s", SDL_GetError());
        }
    }

    void putData(std::vector<Uint8>& data)
    {
        if (!m_sdlStream) return;

        std::vector<Uint8> newData;
        newData.resize(data.size(), 0);
        SDL_MixAudio(newData.data(), data.data(), m_srcSpec.format, data.size(), m_volume);

        if (!SDL_PutAudioStreamData(m_sdlStream, newData.data(), newData.size())) {
            SDL_Log("Failed to put audio stream data: %s", SDL_GetError());
        }
    }

    void unbindAndDestroy()
    {  
        if (m_sdlStream) {
            SDL_UnbindAudioStream(m_sdlStream);
            SDL_DestroyAudioStream(m_sdlStream);
            m_sdlStream = nullptr;
            SDL_Log("Unbind and Destroy audio stream!");
        }
    }

private:
    float m_volume = 1.0f;
    SDL_AudioDeviceID m_deviceId;
    SDL_AudioSpec m_srcSpec;
    SDL_AudioSpec m_dstSpec;
    SDL_AudioStream* m_sdlStream = nullptr;
};

class AudioDevice
{
public:
    explicit AudioDevice(SDL_AudioDeviceID deviceId, SDL_AudioSpec spec) : m_deviceId(deviceId), m_spec(spec)
    {      
        SDL_Log("Creating audio device: %d", m_deviceId);
    }

    ~AudioDevice()
    {
        m_streams.clear();
        SDL_CloseAudioDevice(m_deviceId);
        SDL_Log("Closed audio device!!");
    }

    void printSpec() 
    {
        SDL_Log("Spec: %d %d %d", m_spec.format, m_spec.channels, m_spec.freq);
    }

    AudioStream* createStream()
    {
        auto audioStream = std::make_unique<AudioStream>(m_deviceId, m_spec);
        m_streams.push_back(std::move(audioStream));
        return m_streams.back().get();
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
    std::vector<std::unique_ptr<AudioStream>> m_streams;
};