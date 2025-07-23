#include "PlaybackOperator.h"

PlaybackOperator::PlaybackOperator(Registry &registry) : m_registry(registry)
{
}

PlaybackOperator::~PlaybackOperator()
{
    finalize();
}

void PlaybackOperator::initialize()
{
    // Add plane mixers
    for (int i = 0; i < 2; ++i) {
        m_planeMixers.push_back(PlaneMixer());
    } 

    // Add plane renderers
    for (int i = 0; i < 2; ++i) {
        m_planeRenderers.push_back(new PlaneRenderer());
    } 

    // Add video players
    for (int i = 0; i < 4; ++i) {
        m_videoPlayers.push_back(new VideoPlayer());
        MediaPlayer* mediaPlayer = m_videoPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    // Add camera players
    for (int i = 0; i < 1; ++i) {
        m_cameraPlayers.push_back(new CameraPlayer());
        MediaPlayer* mediaPlayer = m_cameraPlayers[i];
        m_mediaPlayers.push_back(mediaPlayer);
    }

    // Open serial port
    std::string serialDevice = m_registry.settings().serialDevice;
    if (!m_serialController.connect(serialDevice))
    {
        printf("Could not open serial device: %s\n", serialDevice.c_str());
    }
}

void PlaybackOperator::finalize()
{
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

    m_mediaPlayers.clear();
    m_planeRenderers.clear();
    m_planeMixers.clear();
    m_mediaSlotIdToPlayerId.clear();
    m_serialController.disconnect();
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
    int planeId = (mediaSlotId / 8) % 2;
    m_planeMixers[planeId].setFadeTime(fadeTime);

    int playerId = -1;
    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        fileName = videoInputConfig->fileName;
        looping = videoInputConfig->looping;
        filePath = m_registry.mediaPool().getVideoFilePath(fileName);

        if (!getFreeVideoPlayerId(playerId)) return;
        if (m_planeMixers[planeId].startFade(playerId)) {
            m_mediaPlayers[playerId]->openFile(filePath);
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

    updateSerialController();
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

void PlaybackOperator::updateSerialController()
{
    InputMappings &inputMappings = m_registry.inputMappings();

    VM1DeviceState vm1DeviceState;
    vm1DeviceState.bank = uint8_t(inputMappings.bank);
    for (int i = 0; i < 16; ++i)
    {
        int mediaSlotId = (inputMappings.bank * 16) + i;
        InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
        if (!inputConfig)
        {
            vm1DeviceState.media[i] = ButtonState::NONE;
        }
        else if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            if (m_mediaSlotIdToPlayerId.contains(i)) {
                vm1DeviceState.media[i] = ButtonState::FILE_ASSET_ACTIVE;
            }
            else
                vm1DeviceState.media[i] = ButtonState::FILE_ASSET;
        }
        else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
        {
            // TODO: How to handle active HDMI or IMAGE, etc.
            if (m_mediaSlotIdToPlayerId.contains(i)) {
                vm1DeviceState.media[i] = ButtonState::LIVECAM_ACTIVE;
            }
            else
                vm1DeviceState.media[i] = ButtonState::LIVECAM;
        }
    }
    m_serialController.send(vm1DeviceState);
}