/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include "CameraController.h"
#include <SDL3/SDL.h>
#include "edid/1080p30edid.h"

CameraController::CameraController() 
{
    // 
}

CameraController::~CameraController() 
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void CameraController::setupDetached()
{
    m_thread = std::thread(&CameraController::setup, this);
}

bool CameraController::setup() 
{
    const std::string captureDeviceName = "rp1-cfe";
    std::string mediaPath = MediaController::getDevicePath(captureDeviceName);
    if (mediaPath.empty()) {
        std::cout << "Could not find capture device: " <<  captureDeviceName << std::endl;
        return false;
    }

    // Media Controller
    if (!m_mediaController.openDevice(mediaPath)) return false;
    if (!m_mediaController.getTopology()) return false;
    //m_mediaController.printTopology();

    // V4L2 Controller
    std::string subdevPath = m_mediaController.getSubdeviceFromName("tc358743");
    if (!m_v4l2Controller.openDevice(subdevPath)) return false;
    if (!m_v4l2Controller.setEdid(edid_1080p30)) return false;
    SDL_Delay(5000);
    if (!m_v4l2Controller.setDvTimings()) return false;
    v4l2_dv_timings timings =  m_v4l2Controller.queryDvTimings();
    m_v4l2Controller.printDvTimings(timings);
    m_v4l2Controller.closeDevice();
    

    //if (!m_mediaController.resetLinks()) return false;
    if (!m_mediaController.setupLink("csi2", "rp1-cfe-csi2_ch0", MEDIA_LNK_FL_ENABLED)) return false;
    m_mediaController.closeDevice();

    if (!m_mediaController.setFormat("csi2", 0, "UYVY8_1X16", 1920, 1080)) return false;
    if (!m_mediaController.setFormat("csi2", 4, "UYVY8_1X16", 1920, 1080)) return false;
    if (!m_mediaController.setFormat("tc358743", 0, "UYVY8_1X16", 1920, 1080)) return false;
    
    return true;
}