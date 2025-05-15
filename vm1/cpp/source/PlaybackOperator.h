
#pragma once

#include "VideoPlane.h"
#include "CameraPlayer.h"
#include "VideoPlayer.h"
#include "KeyboardController.h"
#include "Registry.h"
#include <map>

class PlaybackOperator {
public:
    PlaybackOperator(Registry &registry);
    ~PlaybackOperator() = default;

    void initialize();
    void update();
    void showMedia(int mediaId);
    
    // Rendering related functions 
    // TODO: Decouple this class from rendering and move them somewhere else!?
    void lockCameras();
    void unlockCameras(); 
    void renderPlane(int planeId, float deltaTime);

private:
    void updateRunningPlayer(VideoInputConfig* videoInputConfig, int playerId);
    void updateKeyboardController();

private:
    Registry& m_registry;
    KeyboardController m_keyboardController;

    // Player related
    CameraPlayer m_cameraPlayers[2];
    VideoPlayer m_videoPlayers[4];
    VideoPlane m_planes[2];
    std::map<int, int> m_mediaSlotIdToPlayerId;
};