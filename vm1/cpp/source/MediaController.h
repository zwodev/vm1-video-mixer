/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <map>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/media.h>
#include <linux/v4l2-subdev.h>


class MediaController {

private:
    int fd;
    std::string device_path;

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

    std::map<__u32, EntityInfo> entities;
    std::vector<LinkInfo> links;

public:
    MediaController(const std::string& mediaDevice);
    ~MediaController();
    void getTopology();
    void resetLinks();
    void setupLink(const std::string& source, const std::string& sink, int flags);
    void setFormat(const std::string& entity, int pad_index, const std::string& format, int width, int height);
    void printTopology();

private:
    __u32 findEntityId(const std::string& name);
};