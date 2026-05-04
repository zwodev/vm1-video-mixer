/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

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

void PlaybackOperator::reloadPlaneShader(int planeId)
{
    const std::string& extShaderFilename = m_registry.planes()[planeId].extShaderFilename;
    std::string completeFilePath;
    if (!extShaderFilename.empty()) {
        completeFilePath = extShaderFilename;
    }
    PlaneRenderer* planeRenderer = m_planeRenderers[planeId];
    planeRenderer->loadShader(completeFilePath);
    m_registry.planes()[planeId].shaderConfig.update(planeRenderer->shaderConfig());
}

void PlaybackOperator::subscribeToEvents()
{
    m_eventBus.subscribe<MediaSlotEvent>([this](const MediaSlotEvent& event) {
        // m_selectedMediaButton = event.slotId % MEDIA_BUTTON_COUNT;
        // std::cout << "m_selectedMediaButton: " << m_selectedMediaButton << std::endl;
        if(event.triggerPlayback) {
            // m_selectedMediaButton = -1;
            showMedia(event.slotId);
        }
        // else {
        //     m_selectedMediaButton = event.slotId % MEDIA_BUTTON_COUNT;
        //     std::cout << "m_selectedMediaButton: " << m_selectedMediaButton << std::endl;
        // }
    });

    m_eventBus.subscribe<EditModeEvent>([this](const EditModeEvent& event) {
        m_selectedEditButton = event.modeId;
    });

    m_eventBus.subscribe<EffectShaderEvent>([this](const EffectShaderEvent& event) {
        reloadPlaneShader(event.planeId);
    });

    m_eventBus.subscribe<PlaneEvent>([this](const PlaneEvent& event) {
        if (event.planeId < 0) return;

        std::vector<int> activeSlotsToClear;
        std::vector<int> activeSlotIds = m_registry.inputMappings().activeSlotIds();
        for (int activeSlotId : activeSlotIds) {
            InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(activeSlotId);
            
            if (inputConfig && inputConfig->planeId == event.planeId) {
                int playerId = inputConfig->playerId;
                if (playerId >= 0) {
                    MediaPlayer* mediaPlayer = m_mediaPlayers[playerId];
                    if (mediaPlayer && !dynamic_cast<HdmiInputConfig*>(inputConfig)) {
                        mediaPlayer->close();
                    }
                    activeSlotsToClear.push_back(activeSlotId);
                }   
                
            }
        }
        
        for (auto id : activeSlotsToClear) {
            m_registry.inputMappings().removeConfig(id);
        }
    });
}

void PlaybackOperator::initialize()
{
    size_t planeCount = m_registry.planes().size();
    size_t videoPlayerCount = planeCount * 2;
    size_t cameraPlayerCount = 1;
    size_t shaderPlayerCount = planeCount * 2;

    for (size_t i = 0; i < planeCount; ++i) {
        m_planeMixers.push_back(PlaneMixer());
    } 

    for (size_t i = 0; i < planeCount; ++i) {
        PlaneRenderer* planeRenderer = new PlaneRenderer();
        m_planeRenderers.push_back(planeRenderer);
        // TODO: We need to be able to update this after custom shader has changed.
        // MAYBE: Event that triggers this in VM1Application?
        //m_registry.planes()[i].shaderConfig.update(planeRenderer->shaderConfig());
        reloadPlaneShader(i);
    }

    for (size_t i = 0; i < videoPlayerCount; ++i) {
        m_videoPlayers.push_back(new VideoPlayer());
        MediaPlayer* mediaPlayer = m_videoPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    for (size_t i = 0; i < cameraPlayerCount; ++i) {
        m_webcamPlayers.push_back(new WebcamPlayer());
        MediaPlayer* mediaPlayer = m_webcamPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    for (size_t i = 0; i < shaderPlayerCount; ++i) {
        m_shaderPlayers.push_back(new ShaderPlayer());
        MediaPlayer* mediaPlayer = m_shaderPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    m_audioSystem.initialize();
    for (size_t i = 0; i < m_mediaPlayers.size(); ++i) {
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

    for (auto webcamPlayer : m_webcamPlayers) {
        delete webcamPlayer;
    } 
    m_webcamPlayers.clear();

    for (auto shaderPlayer : m_shaderPlayers) {
        delete shaderPlayer;
    } 
    m_shaderPlayers.clear();

    for (auto planeRenderer : m_planeRenderers) {
        delete planeRenderer;
    }
    m_planeRenderers.clear();

    m_mediaPlayers.clear(); 
    m_planeMixers.clear();
    m_audioStreams.clear();

    m_audioSystem.finalize();
}


bool PlaybackOperator::getFreeVideoPlayerId(int& id, int planeId)
{
    if(m_registry.settings().useFader) {
        auto activeIds = m_planeMixers[planeId].activeIds();
        if(activeIds.size() > 1) {
            if(m_planeMixers[planeId].mixValue() > 0.5) {
                id = activeIds[0];
            } else {
                id = activeIds[1];
            }
            return true;
        }
        else {
            //printf("looking for empty videoplayerId...\n");
            for (size_t i = 0; i < m_videoPlayers.size(); ++i) {
                //printf("videoplayerId: %d isPlayerIdActive: %d \n", i, isPlayerIdActive(i));
                if(!isPlayerIdActive(i) && dynamic_cast<VideoPlayer *>(m_mediaPlayers[i])) {
                    //printf("id = %d\n", i);
                    id = int(i);
                    return true;
                }
            }
        }
        return false;
    }
    else {        
        for (size_t i = 0; i < m_videoPlayers.size(); ++i) {
            if(!isPlayerIdActive(i) && dynamic_cast<VideoPlayer *>(m_mediaPlayers[i])) {
                id = i;
                return true;
            }
        }
    }
    return false;
}

bool PlaybackOperator::getWebcamPlayerIdFromPort(int port, int& id)
{
    for (size_t i = 0; i < m_mediaPlayers.size(); ++i) {     
        if(WebcamPlayer* webcamPlayer = dynamic_cast<WebcamPlayer *>(m_mediaPlayers[i])) {
            if (webcamPlayer->getPort() == port) {
                id = int(i);
                return true;
            }
        }
    }

    return false;
}

bool PlaybackOperator::getFreeShaderPlayerId(int& id, int planeId)
{
    for (size_t i = 0; i < m_mediaPlayers.size(); ++i) {     
        if(!isPlayerIdActive(i) && dynamic_cast<ShaderPlayer *>(m_mediaPlayers[i])) {
            id = int(i);
            return true;
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

    std::string filePath;

    InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId, true);
    
    if (!inputConfig) {
        m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoMedia, "No media"));
        return;
    }

    float fadeTime = float(m_registry.settings().fadeTime); 
    int planeId = inputConfig->planeId;
    int hdmiId = m_registry.planes()[planeId].hdmiId;
    m_planeMixers[planeId].setFadeTime(fadeTime);

    if (m_registry.settings().hdmiOutputs[hdmiId].empty()) {
        std::string message = "No HDMI-OUT" + std::to_string(hdmiId);
        m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoDisplay, message));
        return;
    }

    int playerId = -1;
    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        filePath = videoInputConfig->fileName;
        if (!getFreeVideoPlayerId(playerId, planeId)) return;

        // Open video file
        AudioStream* audioStream = m_audioStreams[playerId];
        if (!m_mediaPlayers[playerId]->openFile(filePath, audioStream)) {
            printf("Could not play!!\n");
            m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::FileNotSupported, "File not supported"));
            return;
        }

        // Start fade
        if (m_planeMixers[planeId].startFade(playerId)) {
            MediaPlayer* mediaPlayer = m_mediaPlayers[playerId];
            VideoPlayer* videoPlayer = dynamic_cast<VideoPlayer*>(mediaPlayer);
            if (videoPlayer)
            {
                bool looping = videoInputConfig->looping;
                videoPlayer->setLooping(looping);
                videoPlayer->play();
            }
            
            videoInputConfig->playerId = playerId;
        } 
    
    }
    else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    {
        if (!m_registry.settings().isHdmiInputReady) {
            m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::InputNotReady, "Still scanning"));
            return;
        }

        if (!m_registry.settings().useUvcCaptureDevice) {
            if (m_registry.settings().hdmiInputs[hdmiInputConfig->hdmiPort] != "1920x1080/30Hz") {
                std::string message = "No HDMI-IN" + std::to_string(hdmiInputConfig->hdmiPort);
                m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::NoDisplay, message));
                return;
            }
        }

        if (hdmiInputConfig->hdmiPort == 0)
        {
            if (!getWebcamPlayerIdFromPort(hdmiInputConfig->hdmiPort, playerId)) return;

            // Start capture if not already running
            if (!m_mediaPlayers[playerId]->isPlaying()) {
                if(WebcamPlayer* webcamPlayer = dynamic_cast<WebcamPlayer *>(m_mediaPlayers[playerId])) {
                    CaptureType captureType = CaptureType::CT_CSI;
                    if (m_registry.settings().useUvcCaptureDevice) {
                        captureType = CaptureType::CT_WEBCAM_NON_ZERO;
                    }
                    webcamPlayer->setCaptureType(captureType);
                }
                m_mediaPlayers[playerId]->openFile(m_registry.settings().captureDevicePath);
                m_mediaPlayers[playerId]->play(); 
            }
            
            // Start fade
            if (m_planeMixers[planeId].startFade(playerId))  {
                hdmiInputConfig->playerId = playerId;
            }
        }
    }
    else if (ShaderInputConfig *shaderInputConfig = dynamic_cast<ShaderInputConfig *>(inputConfig))
    {
        if (!getFreeShaderPlayerId(playerId, planeId)) return;
        
        // Open shader file
        filePath = shaderInputConfig->fileName;
        if (!m_mediaPlayers[playerId]->openFile(filePath)) {
            printf("Could not open custom shader!!\n");
            m_eventBus.publish(PlaybackEvent(PlaybackEvent::Type::FileNotSupported, "File not supported"));
            return;
        }

        // Update shader parameters
        if (ShaderPlayer* shaderPlayer = dynamic_cast<ShaderPlayer*>(m_mediaPlayers[playerId])) {
            shaderInputConfig->shaderConfig.update(shaderPlayer->shaderConfig());
        }

        // Start fade
        if (m_planeMixers[planeId].startFade(playerId))  {
            //m_planeMixers[planeId].activate();            
            shaderInputConfig->playerId = playerId;
            printf("Shader Player ID: %d\n", playerId);
        }
    }

    m_registry.inputMappings().activateInputConfig(mediaSlotId);
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
        if(m_registry.settings().useFader) {
            planeMixer.setMixValue(m_registry.settings().analog0);
        }
        else {
            planeMixer.updateAutoFade(deltaTime);
        }
    }

    std::vector<int> activePlanes;
    std::vector<int> activePlayerIds;
    std::vector<int> activeSlotsToClear;
    std::vector<int> activeSlotIds = m_registry.inputMappings().activeSlotIds();
    for (int activeSlotId : activeSlotIds) {
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(activeSlotId);
        
        if (inputConfig) {
            int playerId = inputConfig->playerId;
            if (playerId < 0) continue;
            MediaPlayer* mediaPlayer = m_mediaPlayers[playerId];
            if (!mediaPlayer) {
                activeSlotsToClear.push_back(activeSlotId);
                printf("No Media Player on Slot: %d\n", activeSlotId);
                continue;
            }
            if (isPlayerIdActive(playerId)) {
                // Gather active planes and active player ids
                activePlanes.push_back(inputConfig->planeId);
                activePlayerIds.push_back(playerId);

                // Set shader params
                if (ShaderInputConfig* shaderInputConfig = dynamic_cast<ShaderInputConfig*>(inputConfig))
                {
                    if (ShaderPlayer* shaderPlayer = dynamic_cast<ShaderPlayer*>(mediaPlayer)) {
                        // TODO: Move to registry, maybe "Animation System"
                        ShaderConfig& shaderConfig = shaderInputConfig->shaderConfig;
                        if (shaderConfig.params.contains("iTime")) {
                            auto& param = shaderConfig.params["iTime"];
                            if (std::holds_alternative<FloatParameter>(param)) {
                                auto& intParam = std::get<FloatParameter>(param);
                                intParam.value = m_registry.settings().currentTime;
                            }
                        } 
                        shaderPlayer->setShaderUniforms(shaderConfig);
                    } 
                }
            }
            else {
                HdmiInputConfig* hdmiInputConfig = dynamic_cast<HdmiInputConfig*>(inputConfig);
                if (!hdmiInputConfig) {
                    mediaPlayer->close();
                }
                activeSlotsToClear.push_back(activeSlotId);
                printf("Delete Inactive Slot: %d\n", activeSlotId);
            }
        }
    }
    
    for (auto id : activeSlotsToClear) {
        //InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(id);
        m_registry.inputMappings().removeConfig(id);
    }

    for (int i = 0; i < int(m_mediaPlayers.size()); ++i) {
        MediaPlayer* mediaPlayer = m_mediaPlayers[i];
        if (std::find(activePlayerIds.begin(), activePlayerIds.end(), i) == activePlayerIds.end()) {
            if (!dynamic_cast<WebcamPlayer*>(mediaPlayer)) {
                mediaPlayer->close();
            }
        }
        mediaPlayer->update();
    }

    for (int i = 0; i < PLANE_COUNT; i++) {
        if (std::find(activePlanes.begin(), activePlanes.end(), i) == activePlanes.end()) {
            m_planeMixers[i].reset();
        }
    }

    updateDeviceController();
}

void PlaybackOperator::renderPlane(int hdmiId)
{
    if (!m_isInitialized) return;

    const auto& planes = m_registry.planes();

    std::vector<int> activePlanesIds;
    std::vector<int> activeSlotIds = m_registry.inputMappings().activeSlotIds();
    for (int activeSlotId : activeSlotIds)
    {
        InputConfig* inputConfig = m_registry.inputMappings().getInputConfig(activeSlotId);
        if (inputConfig) {
            int planeId = inputConfig->planeId;
            if(std::find(activePlanesIds.begin(), activePlanesIds.end(), planeId) == activePlanesIds.end()) {
                activePlanesIds.push_back(planeId);
            }
        }
    }
    std::sort(activePlanesIds.begin(), activePlanesIds.end(), [](int x, int y){return x < y;});

    for (size_t i = 0; i < activePlanesIds.size(); ++i) {
        int currentPlaneId = activePlanesIds[i];
        
        if(m_registry.planes()[currentPlaneId].useFaderForOpacity) {
            m_registry.planes()[currentPlaneId].opacity = m_registry.settings().analog0;    // todo: just for testing
        }

        if (hdmiId == planes[currentPlaneId].hdmiId) {
            if (i >= m_planeRenderers.size()) return;
            
            
            PlaneRenderer* planeRenderer = m_planeRenderers[currentPlaneId];
            PlaneMixer& planeMixer = m_planeMixers[currentPlaneId];
            
            int fromId = planeMixer.fromId();
            int toId = planeMixer.toId();

            PlaneRenderer::InternalShaderParams internalShaderParams;
            //float volume = float(m_registry.settings().volume) / 10.0f;
            if (fromId >= 0) {
                internalShaderParams.texture0 = m_mediaPlayers[fromId]->texture();
                internalShaderParams.isTex0Valid = true;
                AudioStream* audioStream = m_audioStreams[fromId];
                //if (audioStream) audioStream->setVolume((1.0f - planeMixer.mixValue()) * volume);
                if (audioStream) audioStream->setVolume(0.0f);
            }
            if (toId >= 0) {
                internalShaderParams.texture1 = m_mediaPlayers[toId]->texture();
                internalShaderParams.isTex1Valid = true;
                AudioStream* audioStream = m_audioStreams[toId];
                //if (audioStream) audioStream->setVolume(planeMixer.mixValue() * volume);
                if (audioStream) audioStream->setVolume(0.0f);
            }
;
            
            internalShaderParams.mixValue = planeMixer.mixValue();
            internalShaderParams.iTime = m_registry.settings().currentTime;
            planeRenderer->update(m_registry.planes()[currentPlaneId], m_registry.settings().hdmiRotation0, internalShaderParams);
        }
    }
}

void PlaybackOperator::updateDeviceController()
{
    InputMappings &inputMappings = m_registry.inputMappings();
    VM1DeviceState vm1DeviceState;
    vm1DeviceState.bank = static_cast<uint8_t>(inputMappings.focusedBank);
    vm1DeviceState.rotarySensitivity = static_cast<uint8_t>(m_registry.settings().rotarySensitivity);
    
    
    for (int i = 0; i < EDIT_BUTTON_COUNT; ++i)
    {
        if (i == m_selectedEditButton) {
            vm1DeviceState.editButtons[i] |= ASSIGNED_MASK;
        } else {
            vm1DeviceState.editButtons[i] = 0;
        }
    }
    
    for (int i = 0; i < MEDIA_BUTTON_COUNT; ++i)
    {
        int mediaSlotId = (inputMappings.focusedBank * MEDIA_BUTTON_COUNT) + i;
        InputConfig *inputConfigStaged = m_registry.inputMappings().getInputConfig(mediaSlotId, true);
        InputConfig *inputConfigActive = m_registry.inputMappings().getInputConfig(mediaSlotId, false);

        // empty
        if(!inputConfigStaged)
        {
            vm1DeviceState.mediaButtonsStates[i] = 0;
        } 
        else 
        {
            // not empty
            vm1DeviceState.mediaButtonsStates[i] |= ASSIGNED_MASK;
            
            // currently playing
            if(inputConfigActive)
            {
                vm1DeviceState.mediaButtonsStates[i] |= PLAYING_MASK;
            }
            else
            {
                vm1DeviceState.mediaButtonsStates[i] &= ~PLAYING_MASK;
            }         
        }

        // focused (also an empty mediaslot can be focused when the user selects it)
        int focusedMediaButton = m_registry.inputMappings().focusedMediaButton;
        if(focusedMediaButton > -1)
        {
            vm1DeviceState.mediaButtonsStates[focusedMediaButton] |= FOCUSED_MASK;
        }
    }
    m_deviceController.send(vm1DeviceState);
}