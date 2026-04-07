/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

enum CaptureType {
    CT_CSI,
    CT_WEBCAM,
    CT_WEBCAM_NON_ZERO
};

struct CaptureDevice
{
    std::string name;
    std::string devicePath;
    std::string busInfo;
    std::string driver;
};