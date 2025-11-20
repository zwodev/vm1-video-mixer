/*
 * Copyright (c) 2025 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include "V4L2Controller.h"
#include "MediaController.h"
#include "EventBus.h"
#include "CaptureType.h"

#include <thread>

class CameraController 
{
public:
    CameraController(EventBus& eventBus);
    ~CameraController();
     
    std::vector<CaptureDevice> getCaptureDevices(const std::string& driverName);
    void setupDetached(bool useUvcCaptureDevice);
    bool setupCSI();
    bool setupUVC();

private:
    V4L2Controller m_v4l2Controller;
    MediaController m_mediaController;
    std::thread m_thread;
    EventBus& m_eventBus;
};