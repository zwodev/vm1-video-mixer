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
            showMedia(event.slotId);
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

    m_audioSystem.initialize();
}

void PlaybackOperator::finalize()
{
    m_audioSystem.finalize();

    for (auto videoPlayer : m_videoPlayers) {
        delete videoPlayer;
    }
    m_videoPlayers.clear();

    for (auto cameraPlayer : m_cameraPlayers) {
        delete cameraPlayer;
    } 
    m_cameraPlayers.clear();

    for (auto planeRenderer : m_planeRenderers) {
        delete planeRenderer;
    }
    m_planeRenderers.clear();

    m_mediaPlayers.clear(); 
    m_planeMixers.clear();
    m_mediaSlotIdToPlayerId.clear();
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
    std::string fileName;
    std::string filePath;
    bool looping = false;

    InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
    if (!inputConfig)
        return;

    // Select plane
    float fadeTime = float(m_registry.settings().fadeTime); 
    int planeId = (mediaSlotId / (MEDIA_BUTTON_COUNT / 2)) % 2;
    m_planeMixers[planeId].setFadeTime(fadeTime);

    int playerId = -1;
    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        fileName = videoInputConfig->fileName;
        looping = videoInputConfig->looping;
        filePath = m_registry.mediaPool().getVideoFilePath(fileName);

        if (!getFreeVideoPlayerId(playerId)) return;
        AudioDevice* audioDevice = m_audioSystem.audioDevice(0);
        if (!m_mediaPlayers[playerId]->openFile(filePath, audioDevice)) return;
        if (m_planeMixers[planeId].startFade(playerId)) {
            m_mediaPlayers[playerId]->play(); 
            m_mediaSlotIdToPlayerId[mediaSlotId] = playerId;
        }
    }
    else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    {
        if (hdmiInputConfig->hdmiPort == 0)
        {
            fileName = "hdmi0";
            filePath = m_registry.mediaPool().getVideoFilePath(fileName);
            if (!getCameraPlayerIdFromPort(hdmiInputConfig->hdmiPort, playerId)) return;

            if (!m_mediaPlayers[playerId]->isPlaying()) {
                m_mediaPlayers[playerId]->play(); 
            }
            
            if (m_planeMixers[planeId].startFade(playerId))  {
                m_mediaSlotIdToPlayerId[mediaSlotId] = playerId;
            }
        }
    }
}

void PlaybackOperator::update(float deltaTime)
{
    for (auto& planeMixer : m_planeMixers) {
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

    for (auto id : idsToDelete) {
        m_mediaSlotIdToPlayerId.erase(id);    
    }

    updateDeviceController();
}

void PlaybackOperator::renderPlane(int planeId)
{
    if (planeId >= m_planeRenderers.size()) return;

    PlaneRenderer* planeRenderer = m_planeRenderers[planeId];
    PlaneMixer& planeMixer = m_planeMixers[planeId];
    int fromId = planeMixer.fromId();
    int toId = planeMixer.toId();
    GLuint texture0 = 0;
    if (fromId >= 0) {
        texture0 = m_mediaPlayers[fromId]->texture();
    }
    GLuint texture1 = 0;
    if (toId >= 0) {
        texture1 = m_mediaPlayers[toId]->texture();
    }

    planeRenderer->update(texture0, texture1, planeMixer.mixValue());
}

void PlaybackOperator::updateDeviceController()
{
    InputMappings &inputMappings = m_registry.inputMappings();
    VM1DeviceState vm1DeviceState;
    vm1DeviceState.bank = uint8_t(inputMappings.bank);
    
    
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
    m_deviceController.send(vm1DeviceState);
}