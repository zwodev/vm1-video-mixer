
#pragma once

#include "PlaneRenderer.h"
#include "CameraPlayer.h"
#include "VideoPlayer.h"
#include "AudioSystem.h"
#include "DeviceController.h"
#include "Registry.h"
#include "EventBus.h"

#include <vector>
#include <map>

class PlaneMixer {
public:

    std::vector<int> activeIds()
    {
        std::vector<int> ids;
        if (m_fromId >= 0) ids.push_back(m_fromId);
        if (m_toId >= 0) ids.push_back(m_toId);
        return ids;
    }

    int fromId()
    {
        return m_fromId;
    }

    int toId()
    {
        return m_toId;
    }

    float fadeTime() const
    {
        return m_fadeTime;
    }

    void setFadeTime(int fadeTime)
    {
        m_fadeTime = fadeTime;
    }

    float mixValue() const
    {
        return m_mixValue;
    }

    bool startFade(int toId)
    {
        if (m_isFading) return false;

        m_toId = toId;
        m_isFading = true;
        m_fadeDir = 1.0f;
        //if (m_mixValue > 0.0f) m_fadeDir = -1.0f;

        return true;
    }

    void update(float deltaTime)
    {
        if (!m_isFading) return;

        m_mixValue = m_mixValue + ((deltaTime / m_fadeTime) * m_fadeDir);
        // if (m_mixValue <= 0.0f) {
        //     m_mixValue = 0.0f;
        //     m_isFading = false;
        //     m_fromId = m_toId;
        //     m_toId = -1;
        // } 
        //else 
        if (m_mixValue >= 1.0f) {
            m_mixValue = 0.0f;
            m_isFading = false;
            m_fromId = m_toId;
            m_toId = -1;
        }
    }

private:
    bool m_isFading = false;
    float m_fadeTime = 2.0f;
    float m_fadeDir = 1.0f;
    float m_mixValue = 0.0f;
    int m_fromId = -1;
    int m_toId = -1;
};

class PlaybackOperator {
public:
    PlaybackOperator(Registry& registry, EventBus& eventBus, DeviceController& deviceController);
    ~PlaybackOperator();

    void initialize();
    void finalize();
    void showMedia(int mediaId);
    void update(float deltaTime);
    void renderPlane(int planeId);
    
private:
    void subscribeToEvents();
    bool getCameraPlayerIdFromPort(int port, int& id);
    bool getFreeVideoPlayerId(int& id);
    bool isPlayerIdActive(int playerId);
    void updateDeviceController();

private:
    Registry& m_registry;
    EventBus& m_eventBus;
    DeviceController& m_deviceController;
    
    AudioSystem m_audioSystem;
    std::vector<PlaneMixer> m_planeMixers;
    std::vector<PlaneRenderer*> m_planeRenderers;

    std::vector<VideoPlayer*> m_videoPlayers;
    std::vector<CameraPlayer*> m_cameraPlayers;
    std::vector<MediaPlayer*> m_mediaPlayers;
    std::map<int, int> m_mediaSlotIdToPlayerId;

    int m_selectedEditButton = -1;

};