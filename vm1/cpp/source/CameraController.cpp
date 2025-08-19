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

CameraController::CameraController(EventBus& eventBus) :
    m_eventBus(eventBus)
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
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_thread = std::thread(&CameraController::setup, this);
}

bool CameraController::setup() 
{
    v4l2_dv_timings timings;
    std::string subdevPath;
    const std::string captureDeviceName = "rp1-cfe";
    std::string mediaPath = MediaController::getDevicePath(captureDeviceName);
    if (mediaPath.empty()) {
        std::cout << "Could not find capture device: " <<  captureDeviceName << std::endl;        
        goto unknownerror;
    }

    // Media Controller
    if (!m_mediaController.openDevice(mediaPath)) goto unknownerror;
    if (!m_mediaController.getTopology()) goto unknownerror;
    //m_mediaController.printTopology();

    // V4L2 Controller
    subdevPath = m_mediaController.getSubdeviceFromName("tc358743");
    if (!m_v4l2Controller.openDevice(subdevPath)) goto unknownerror;
    if (!m_v4l2Controller.setEdid(edid_1080p30)) goto unknownerror;
    SDL_Delay(5000);
    if (!m_v4l2Controller.setDvTimings()) goto notconnected;
    timings =  m_v4l2Controller.queryDvTimings();
    if (timings.bt.width == 0 && timings.bt.height == 0) {
        goto notconnected;
    }
    else if (timings.bt.width != 1920 || timings.bt.height != 1080) {
        goto resmismatch;
    }
    else if (timings.bt.pixelclock != 74250000) {
        goto fpsmismatch;
    }

    m_v4l2Controller.printDvTimings(timings);
    m_v4l2Controller.closeDevice();
    

    //if (!m_mediaController.resetLinks()) return false;
    if (!m_mediaController.setupLink("csi2", "rp1-cfe-csi2_ch0", MEDIA_LNK_FL_ENABLED)) goto unknownerror;
    m_mediaController.closeDevice();

    if (!m_mediaController.setFormat("csi2", 0, "UYVY8_1X16", 1920, 1080)) goto unknownerror;
    if (!m_mediaController.setFormat("csi2", 4, "UYVY8_1X16", 1920, 1080)) goto unknownerror;
    if (!m_mediaController.setFormat("tc358743", 0, "UYVY8_1X16", 1920, 1080)) goto unknownerror;
    
    m_eventBus.enqueue(HdmiCaptureInitEvent(std::string("1920x1080/30Hz")));
    return true;

    notconnected:
    m_eventBus.enqueue(HdmiCaptureInitEvent(std::string("Not connected")));
    m_v4l2Controller.closeDevice();
    m_mediaController.closeDevice();
    return false;

    resmismatch:
    m_eventBus.enqueue(HdmiCaptureInitEvent(std::string("RES mismatch"))); 
    m_v4l2Controller.closeDevice();
    m_mediaController.closeDevice();
    return false;

    fpsmismatch:
    m_eventBus.enqueue(HdmiCaptureInitEvent(std::string("FPS mismatch")));   
    m_v4l2Controller.closeDevice();
    m_mediaController.closeDevice();
    return false;  
        
    unknownerror:
    m_eventBus.enqueue(HdmiCaptureInitEvent(std::string("Error")));   
    m_v4l2Controller.closeDevice();
    m_mediaController.closeDevice();
    
    return false;
}