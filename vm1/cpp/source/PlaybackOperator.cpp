#include "PlaybackOperator.h"

PlaybackOperator::PlaybackOperator(Registry &registry) : m_registry(registry)
{
    initialize();
}

void PlaybackOperator::initialize()
{
    // Open serial port
    std::string serialDevice = m_registry.settings().serialDevice;
    if (!m_keyboardController.connect(serialDevice))
    {
        printf("Could not open serial device: %s\n", serialDevice.c_str());
    }

    // First screen
    m_planes[0].addVideoPlayer(&(m_videoPlayers[0]));
    m_planes[0].addVideoPlayer(&(m_videoPlayers[1]));
    m_planes[0].addCameraPlayer(&(m_cameraPlayers[0]));

    // Second screen
    m_planes[1].addVideoPlayer(&(m_videoPlayers[2]));
    m_planes[1].addVideoPlayer(&(m_videoPlayers[3]));
    m_planes[1].addCameraPlayer(&(m_cameraPlayers[0]));
}

void PlaybackOperator::showMedia(int mediaSlotId)
{
    std::string fileName;
    std::string filePath;
    bool looping = false;

    InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
    if (!inputConfig)
        return;

    if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
    {
        fileName = videoInputConfig->fileName;
        looping = videoInputConfig->looping;
        filePath = m_registry.mediaPool().getVideoFilePath(fileName);
    }
    else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
    {
        if (hdmiInputConfig->hdmiPort == 0)
        {
            fileName = "hdmi0";
            filePath = m_registry.mediaPool().getVideoFilePath(fileName);
        }
    }

    // Select plane
    float fadeTime = float(m_registry.settings().fadeTime); 
    int oddRow = (mediaSlotId / 8) % 2;
    if (oddRow == 0)
    {
        printf("Play Left: (ID: %d, FILE: %s, LOOP: %d)\n", mediaSlotId, filePath.c_str(), looping);
        m_planes[0].setFadeTime(fadeTime);
        int playerIndex = m_planes[0].playAndFade(filePath, looping);
        printf("Player Index: %d", playerIndex);
        if (playerIndex >= 0)
        {
            m_mediaSlotIdToPlayerId[mediaSlotId] = playerIndex;
        }
    }
    else
    {
        printf("Play Right: (ID: %d, FILE: %s, LOOP: %d)\n", mediaSlotId, fileName.c_str(), looping);
        m_planes[1].setFadeTime(fadeTime);
        int playerIndex = m_planes[1].playAndFade(filePath, looping);
        if (playerIndex >= 0)
        {
            playerIndex += 2;
            m_mediaSlotIdToPlayerId[mediaSlotId] = playerIndex;
        }
    }
}

void PlaybackOperator::updateRunningPlayer(VideoInputConfig *videoInputConfig, int playerId)
{
    // Delete ??
}

void PlaybackOperator::update()
{
    std::vector<int> idsToDelete;
    for (const auto &[key, value] : m_mediaSlotIdToPlayerId)
    {
        int mediaSlotId = key;
        int playerId = value;

        InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
        if (!inputConfig)
            continue;

        if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            bool looping = videoInputConfig->looping;
            VideoPlayer &videoPlayer = m_videoPlayers[playerId];
            if (videoPlayer.isPlaying())
            {
                 videoPlayer.setLooping(looping);
            }
            else
            {
                if(m_mediaSlotIdToPlayerId.contains(mediaSlotId)) {
                    idsToDelete.push_back(mediaSlotId);
                }
            }
        }
    }

    for (auto id : idsToDelete) {
        m_mediaSlotIdToPlayerId.erase(id);
    }

    updateKeyboardController();
}

void PlaybackOperator::lockCameras()
{
    for (int i = 0; i < 1; ++i)
    {
        m_cameraPlayers[i].lockBuffer();
    }
}

void PlaybackOperator::unlockCameras()
{
    for (int i = 0; i < 1; ++i)
    {
        m_cameraPlayers[i].unlockBuffer();
    }
}

void PlaybackOperator::renderPlane(int planeId, float deltaTime)
{
    m_planes[planeId].update(deltaTime);
}

void PlaybackOperator::updateKeyboardController()
{
    InputMappings &inputMappings = m_registry.inputMappings();

    ControllerState controllerState;
    controllerState.bank = uint8_t(inputMappings.bank);
    for (int i = 0; i < 16; ++i)
    {
        int mediaSlotId = (inputMappings.bank * 16) + i;
        InputConfig *inputConfig = m_registry.inputMappings().getInputConfig(mediaSlotId);
        if (!inputConfig)
        {
            controllerState.media[i] = ButtonState::NONE;
        }
        else if (VideoInputConfig *videoInputConfig = dynamic_cast<VideoInputConfig *>(inputConfig))
        {
            if (m_mediaSlotIdToPlayerId.contains(i))
                controllerState.media[i] = ButtonState::FILE_ASSET_ACTIVE;
            else
                controllerState.media[i] = ButtonState::FILE_ASSET;
        }
        else if (HdmiInputConfig *hdmiInputConfig = dynamic_cast<HdmiInputConfig *>(inputConfig))
        {
            // TODO: How to handle active HDMI or IMAGE, etc.
            controllerState.media[i] = ButtonState::LIVECAM;
        }
    }
    m_keyboardController.send(controllerState);
}