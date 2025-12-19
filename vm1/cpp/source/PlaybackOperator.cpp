#include "PlaybackOperator.h"
#include "VM1DeviceDefinitions.h"

PlaybackOperator::PlaybackOperator(Registry& registry, EventBus& eventBus, DeviceController& deviceController) : 
    m_registry(registry),
    m_eventBus(eventBus),
    m_deviceController(deviceController)
{
    subscribeToEvents();
}

PlaybackOperator::~PlaybackOperator()
{
    finalize();
}

void PlaybackOperator::subscribeToEvents()
{
    m_eventBus.subscribe<MediaSlotEvent>([this](const MediaSlotEvent& event) {
        if(event.triggerPlayback) {
            m_selectedMediaButton = -1;
            showMedia(event.slotId);
        } else {
            m_selectedMediaButton = event.slotId % MEDIA_BUTTON_COUNT;
            std::cout << "m_selectedMediaButton: " << m_selectedMediaButton << std::endl;
        }
    });

    m_eventBus.subscribe<EditModeEvent>([this](const EditModeEvent& event) {
        m_selectedEditButton = event.modeId;
    });
}

void PlaybackOperator::initialize()
{
    for (int i = 0; i < 2; ++i) {
        m_planeMixers.push_back(PlaneMixer());
    } 

    for (int i = 0; i < 2; ++i) {
        m_planeRenderers.push_back(new PlaneRenderer());
    } 

    for (int i = 0; i < 4; ++i) {
        m_videoPlayers.push_back(new VideoPlayer());
        MediaPlayer* mediaPlayer = m_videoPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    for (int i = 0; i < 1; ++i) {
        m_cameraPlayers.push_back(new CameraPlayer());
        MediaPlayer* mediaPlayer = m_cameraPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    for (int i = 0; i < 1; ++i) {
        m_webcamPlayers.push_back(new WebcamPlayer());
        MediaPlayer* mediaPlayer = m_webcamPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    m_audioSystem.initialize();
    for (int i = 0; i < m_mediaPlayers.size(); ++i) {
        AudioDevice* audioDevice = m_audioSystem.audioDevice(0);
        AudioStream* audioStream = nullptr;
        if (audioDevice) {
            audioStream = audioDevice->createStream();
        }
        m_audioStreams.push_back(audioStream);
    }

    m_isInitialized = true;
}

void PlaybackOperator::finalize()
{
    m_isInitialized = false;

    for (auto videoPlayer : m_videoPlayers) {
        delete videoPlayer;
    }
    m_videoPlayers.clear();

    for (auto cameraPlayer : m_cameraPlayers) {
        delete cameraPlayer;
    } 
    m_cameraPlayers.clear();

    for (auto webcamPlayer : m_webcamPlayers) {
        delete webcamPlayer;
    } 
    m_webcamPlayers.clear();

    for (auto planeRenderer : m_planeRenderers) {
        delete planeRenderer;
    }
    m_planeRenderers.clear();

    m_mediaPlayers.clear(); 
    m_planeMixers.clear();
    m_mediaSlotIdToPlayerId.clear();
    m_audioStreams.clear();

    m_audioSystem.finalize();
}

bool PlaybackOperator::getFreeVideoPlayerId(int& id)
{
    for (int i = 0; i < m_videoPlayers.size(); ++i) {
        if(!isPlayerIdActive(i) && dynamic_cast<VideoPlayer *>(m_mediaPlayers[i])) {
            id = i;
            return true;
        }
    }

    return false;
}

bool PlaybackOperator::getCameraPlayerIdFromPort(int port, int& id)
{
    for (int i = 0; i < m_mediaPlayers.size(); ++i) {     
        if(CameraPlayer* cameraPlayer = dynamic_cast<CameraPlayer *>(m_mediaPlayers[i])) {
            if (cameraPlayer->getPort() == port) {
                id = i;
                return true;
            }
        }
    }

    return false;
}

bool PlaybackOperator::getWebcamPlayerIdFromPort(int port, int& id)
{
    for (int i = 0; i < m_mediaPlayers.size(); ++i) {     
        if(WebcamPlayer* webcamPlayer = dynamic_cast<WebcamPlayer *>(m_mediaPlayers[i])) {
            if (webcamPlayer->getPort() == port) {
                id = i;
                return true;
            }
        }
    }

    return false;
}

bool PlaybackOperator::isPlayerIdActive(int playerId)
{
    for (auto planeMixer : m_planeMixers) {
        auto activeIds = planeMixer.activeIds();
        for (int activePlayerId : activeIds) {
            if (activePlayerId == playerId) return true;
        }
    }

    return false;
}

void PlaybackOperator::showMedia(int mediaSlotId)
{
    if (!m_isInitialized) return;

    if (!m_registry.settings().isHdmiOutputReady) {
        m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoDisplay, "Still scanning"));
        return;
    }

    std::string fileName;
    std::string filePath;
    bool looping = false;

    InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
    if (!inputConfig) {
        m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoMedia, "No media"));
        return;
    }


    float fadeTime = float(m_registry.settings().fadeTime); 
    int planeId = (mediaSlotId / (MEDIA_BUTTON_COUNT / 2)) % 2;
    m_planeMixers[planeId].setFadeTime(fadeTime);

    if (m_registry.settings().hdmiOutputs[planeId].empty()) {
        std::string message = "No HDMI-OUT" + std::to_string(planeId+1);
        m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoDisplay, message));
        return;
    }

    int playerId = -1;
    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        fileName = videoInputConfig->fileName;
        looping = videoInputConfig->looping;
        filePath = m_registry.mediaPool().getVideoFilePath(fileName);

        if (!getFreeVideoPlayerId(playerId)) return;
        AudioStream* audioStream = m_audioStreams[playerId];
        if (!m_mediaPlayers[playerId]->openFile(filePath, audioStream)) {
            printf("Could not play!!\n");
            m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::FileNotSupported, "File not supported"));
            return;
        }
        if (m_planeMixers[planeId].startFade(playerId)) {
            m_mediaPlayers[playerId]->play(); 
            if (m_mediaSlotIdToPlayerId.find(mediaSlotId) != m_mediaSlotIdToPlayerId.end()) {
                int oldPlayerId = m_mediaSlotIdToPlayerId[mediaSlotId];
                m_recentlyUsedPlayerIds.push_back(oldPlayerId);
            }
            m_mediaSlotIdToPlayerId[mediaSlotId] = playerId;
        } 
    }
    // else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    // {
    //     if (!m_registry.settings().isHdmiInputReady) {
    //         m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::InputNotReady, "Still scanning"));
    //         return;
    //     }

    //     if (m_registry.settings().hdmiInputs[hdmiInputConfig->hdmiPort] != "1920x1080/30Hz") {
    //         std::string message = "No HDMI-IN" + std::to_string(hdmiInputConfig->hdmiPort+1);
    //         m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoDisplay, message));
    //         return;
    //     }

    //     if (hdmiInputConfig->hdmiPort == 0)
    //     {
    //         if (!getCameraPlayerIdFromPort(hdmiInputConfig->hdmiPort, playerId)) return;

    //         if (!m_mediaPlayers[playerId]->isPlaying()) {
    //             m_mediaPlayers[playerId]->play(); 
    //         }
            
    //         if (m_planeMixers[planeId].startFade(playerId))  {
    //             m_mediaSlotIdToPlayerId[mediaSlotId] = playerId;
    //         }
    //     }
    // }
     else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    {
        if (!m_registry.settings().isHdmiInputReady) {
            m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::InputNotReady, "Still scanning"));
            return;
        }

        if (!m_registry.settings().useUvcCaptureDevice) {
            if (m_registry.settings().hdmiInputs[hdmiInputConfig->hdmiPort] != "1920x1080/30Hz") {
                std::string message = "No HDMI-IN" + std::to_string(hdmiInputConfig->hdmiPort+1);
                m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoDisplay, message));
                return;
            }
        }

        if (hdmiInputConfig->hdmiPort == 0)
        {
            if (!getWebcamPlayerIdFromPort(hdmiInputConfig->hdmiPort, playerId)) return;

            if (!m_mediaPlayers[playerId]->isPlaying()) {
                if(WebcamPlayer* webcamPlayer = dynamic_cast<WebcamPlayer *>(m_mediaPlayers[playerId])) {
                    CaptureType captureType = CaptureType::CT_CSI;
                    if (m_registry.settings().useUvcCaptureDevice) {
                        captureType = CaptureType::CT_WEBCAM_NON_ZERO;
                    }
                    webcamPlayer->setCaptureType(captureType);
                }
                m_mediaPlayers[playerId]->openFile(m_registry.settings().captureDevicePath);
                printf("Device Path: %s\n", m_registry.settings().captureDevicePath.c_str());
                m_mediaPlayers[playerId]->play(); 
            }
            
            if (m_planeMixers[planeId].startFade(playerId))  {
                m_mediaSlotIdToPlayerId[mediaSlotId] = playerId;
            }
        }
    }
    // else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    // {
    //     if (hdmiInputConfig->hdmiPort == 0)
    //     {
    //         if (!getWebcamPlayerIdFromPort(hdmiInputConfig->hdmiPort, playerId)) return;

    //         if (!m_mediaPlayers[playerId]->isPlaying()) {
    //             m_mediaPlayers[playerId]->play(); 
    //         }
            
    //         if (m_planeMixers[planeId].startFade(playerId))  {
    //             m_mediaSlotIdToPlayerId[mediaSlotId] = playerId;
    //         }
    //     }
    // }
}

void PlaybackOperator::update(float deltaTime)
{
    if (!m_isInitialized) return; 

    for (auto& planeMixer : m_planeMixers) {
        int playerId = planeMixer.toId();
        if (playerId >= 0 && m_mediaPlayers[playerId]->isFrameReady()) {
            planeMixer.activate();
        }
        
        // todo: enable analog input
        float fadeValue = float(m_registry.settings().analog0) / 1024.0;
        if (fadeValue < 0.01) 
        fadeValue = 0.0;
        else if (fadeValue > 0.99)
        fadeValue = 1.0;
        // printf("fadeValue: %d, %f\n", m_registry.settings().analog0, fadeValue);
        // planeMixer.setMixValue(fadeValue);
        
        planeMixer.update(deltaTime);
    }

    for (auto mediaPlayer : m_mediaPlayers) {
        mediaPlayer->update();
    }

    std::vector<int> idsToDelete;
    for (const auto &[key, value] : m_mediaSlotIdToPlayerId)
    {
        int mediaSlotId = key;
        int playerId = value;
        MediaPlayer* mediaPlayer = m_mediaPlayers[playerId];

        if (!isPlayerIdActive(playerId)) {
            if(m_mediaSlotIdToPlayerId.contains(mediaSlotId)) {
                idsToDelete.push_back(mediaSlotId);
            }
            if (VideoPlayer* videoPlayer = dynamic_cast<VideoPlayer*>(mediaPlayer)) {
                videoPlayer->close();
            }
        }

        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
        if (!inputConfig) continue;

        if (inputConfig) inputConfig->isActive = isPlayerIdActive(playerId);
        if (VideoInputConfig* videoInputConfig = dynamic_cast<VideoInputConfig*>(inputConfig))
        {
            bool looping = videoInputConfig->looping;
            VideoPlayer* videoPlayer = dynamic_cast<VideoPlayer*>(mediaPlayer);
            if (videoPlayer && videoPlayer->isPlaying())
            {
                videoPlayer->setLooping(looping);
            }

            if (!videoPlayer || !videoPlayer->isPlaying())
            {
                if(m_mediaSlotIdToPlayerId.contains(mediaSlotId)) {
                    idsToDelete.push_back(mediaSlotId);
                }
            }
        }
        else if (HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(inputConfig))
        {
            CameraPlayer* cameraPlayer = dynamic_cast<CameraPlayer*>(mediaPlayer);
            // if (cameraPlayer && cameraPlayer->isPlaying())
            // {
            //     cameraPlayer->update();
            // }
        }
    }

    std::vector<int> recentlyUsedPlayerIds = m_recentlyUsedPlayerIds;
    for (int playerId : recentlyUsedPlayerIds) {
        MediaPlayer* mediaPlayer = m_mediaPlayers[playerId];
        if (!isPlayerIdActive(playerId)) {
            if (VideoPlayer* videoPlayer = dynamic_cast<VideoPlayer*>(mediaPlayer)) {
                videoPlayer->close();
                m_recentlyUsedPlayerIds.erase(std::find(m_recentlyUsedPlayerIds.begin(), m_recentlyUsedPlayerIds.end(), playerId));
            }
        } 
    }

    for (auto id : idsToDelete) {
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(id);
        if (inputConfig) inputConfig->isActive = false;
        m_mediaSlotIdToPlayerId.erase(id);    
    }

    updateDeviceController();
}

void PlaybackOperator::renderPlane(int planeId)
{
    if (!m_isInitialized) return;

    if (planeId >= m_planeRenderers.size()) return;

    float volume = float(m_registry.settings().volume) / 10.0f;
    PlaneRenderer* planeRenderer = m_planeRenderers[planeId];
    PlaneMixer& planeMixer = m_planeMixers[planeId];
    
    int fromId = planeMixer.fromId();
    int toId = planeMixer.toId();

    GLuint texture0 = 0;
    if (fromId >= 0) {
        texture0 = m_mediaPlayers[fromId]->texture();
        AudioStream* audioStream = m_audioStreams[fromId];
        if (audioStream) audioStream->setVolume((1.0f - planeMixer.mixValue()) * volume);
    }
    GLuint texture1 = 0;
    if (toId >= 0) {
        texture1 = m_mediaPlayers[toId]->texture();
        AudioStream* audioStream = m_audioStreams[toId];
        if (audioStream) audioStream->setVolume(planeMixer.mixValue() * volume);
    }

    planeRenderer->update(texture0, texture1, planeMixer.mixValue());
}

void PlaybackOperator::updateDeviceController()
{
    InputMappings &inputMappings = m_registry.inputMappings();
    VM1DeviceState vm1DeviceState;
    vm1DeviceState.bank = static_cast<uint8_t>(inputMappings.bank);
    vm1DeviceState.rotarySensitivity = static_cast<uint8_t>(m_registry.settings().rotarySensitivity);
    
    
    for (int i = 0; i < EDIT_BUTTON_COUNT; ++i)
    {
        if (i == m_selectedEditButton) {
            vm1DeviceState.editButtons[i] = ButtonState::EMPTY; // "EMPTY" is simply white color. todo: rename
        } else {
            vm1DeviceState.editButtons[i] = ButtonState::NONE;
        }
    }
    
    for (int i = 0; i < MEDIA_BUTTON_COUNT; ++i)
    {
        int mediaSlotId = (inputMappings.bank * MEDIA_BUTTON_COUNT) + i;
        InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
        if (!inputConfig)
        {
            vm1DeviceState.mediaButtons[i] = ButtonState::NONE;
        }
        else if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            if (m_mediaSlotIdToPlayerId.contains(mediaSlotId)) {
                vm1DeviceState.mediaButtons[i] = ButtonState::FILE_ASSET_ACTIVE;
            }
            else
                vm1DeviceState.mediaButtons[i] = ButtonState::FILE_ASSET;
        }
        else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
        {
            // TODO: How to handle active HDMI or IMAGE, etc.
            if (m_mediaSlotIdToPlayerId.contains(mediaSlotId)) {
                vm1DeviceState.mediaButtons[i] = ButtonState::LIVECAM_ACTIVE;
            }
            else
                vm1DeviceState.mediaButtons[i] = ButtonState::LIVECAM;
        }
    }

    if(m_selectedMediaButton > -1) {
        vm1DeviceState.mediaButtons[m_selectedMediaButton] = ButtonState::MEDIABUTTON_SELECTED;
    }

    m_deviceController.send(vm1DeviceState);
}