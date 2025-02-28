/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <linux/v4l2-subdev.h>

class V4L2Controller {
private:
    int fd;
    std::string device_path;

    static constexpr size_t MAX_EDID_SIZE = 256;

public:
    V4L2Controller();
    ~V4L2Controller();

    static void listDevices();

    bool openDevice(const std::string& devicePath);
    void closeDevice();
    bool setEdid(const std::vector<unsigned char>& edid);
    bool setEdid(const std::string& edid_file);
    v4l2_dv_timings queryDvTimings();
    bool setDvTimings();
    void printDvTimings(const v4l2_dv_timings& timings);
};