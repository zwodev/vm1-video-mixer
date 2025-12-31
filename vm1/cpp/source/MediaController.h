/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/media.h>
#include <linux/v4l2-subdev.h>


class MediaController {

private:
    int m_fd = -1;

    struct EntityInfo {
        __u32 id;
        std::string name;
        std::vector<media_v2_pad> pads;
    };

    struct LinkInfo {
        __u32 id;
        __u32 source_id;
        __u32 sink_id;
        __u32 flags;
    };

    struct InterfaceInfo {
        __u32 id;
        __u32 intf_type;
	    __u32 flags;
        __u32 major;
	    __u32 minor;
    };

    std::map<__u32, EntityInfo> m_idToEntity;
    std::map<__u32, EntityInfo> m_padIdToEntity;
    std::vector<LinkInfo> m_links;
    std::vector<InterfaceInfo> m_interfaces;

public:
    MediaController();
    ~MediaController();

    static void listDevices();
    static std::string getDevicePath(const std::string& deviceName);

    bool openDevice(const std::string& devicePath);
    void closeDevice();
    bool getTopology();
    std::string getSubdeviceFromName(const std::string& devieName);
    bool resetLinks();
    bool setupLink(const std::string& source, const std::string& sink, int flags);
    bool setFormat(const std::string& entity, int pad_index, const std::string& format, int width, int height);
    void printTopology();

private:
    static std::map<std::string, std::string> fetchDevices();
    bool findEntityId(const std::string& name, __u32& outId);
    std::string getSubdevPath(uint32_t entity_id);
};