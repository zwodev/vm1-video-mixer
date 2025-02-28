/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include "MediaController.h"

#include <filesystem>
#include <system_error>

MediaController::MediaController() {
}

MediaController::~MediaController() {
    closeDevice();
}

bool MediaController::openDevice(const std::string& devicePath) {
    m_fd = open(devicePath.c_str(), O_RDWR);
    if (m_fd < 0) {
        std::cout << "Failed to open media device: " << std::string(strerror(errno)) << std::endl;
        return false;
    }

    return true;
}

void MediaController::closeDevice() {
    if (m_fd >= 0) {
        close(m_fd);
        m_fd = -1;
    }
}

void MediaController::listDevices() {
    auto deviceMap = fetchDevices();
    for (const auto& [key, value] : deviceMap) {
        std::cout << key << ": " << value << std::endl;
    }
}

std::string MediaController::getDevicePath(const std::string& deviceName) {
    std::string devicePath;
    auto deviceMap = fetchDevices();

    if (deviceMap.find(deviceName) != deviceMap.end()) {
        devicePath = deviceMap[deviceName];
    }

    return devicePath;
}

bool MediaController::getTopology() {
    struct media_v2_topology topology = {0};
    std::vector<media_v2_entity> entity_array;
    std::vector<media_v2_pad> pad_array;
    std::vector<media_v2_link> link_array;

    if (ioctl(m_fd, MEDIA_IOC_G_TOPOLOGY, &topology) < 0) {
        std::cout << "Failed to get topology: " <<  std::string(strerror(errno)) << std::endl;
        return false;
    }

    entity_array.resize(topology.num_entities);
    pad_array.resize(topology.num_pads);
    link_array.resize(topology.num_links);

    topology.ptr_entities = reinterpret_cast<__u64>(entity_array.data());
    topology.ptr_pads = reinterpret_cast<__u64>(pad_array.data());
    topology.ptr_links = reinterpret_cast<__u64>(link_array.data());

    if (ioctl(m_fd, MEDIA_IOC_G_TOPOLOGY, &topology) < 0) {
        std::cout << "Failed to get topology: " << std::string(strerror(errno)) << std::endl;
        return false;
    }

    m_idToEntity.clear();
    m_links.clear();

    // Process entities and their pads
    for (const auto& entity : entity_array) {
        EntityInfo entity_info = {entity.id, std::string(entity.name), {}};
        for (const auto& pad : pad_array) {
            if (pad.entity_id == entity.id) {
                entity_info.pads.push_back(pad);
            }
        }
        m_idToEntity[entity.id] = entity_info;     
    }

    // Process links
    for (const auto& link : link_array) {
        m_links.push_back({link.id, link.source_id, link.sink_id, link.flags});
    }

    return true;
}

bool MediaController::resetLinks() {
    for (auto& link : m_links) {
        struct media_link_desc link_desc = {0};
        link_desc.source.entity = link.source_id;
        link_desc.sink.entity = link.sink_id;
        link_desc.flags = 0;

        // Find the correct pad indices
        auto& source_entity = m_idToEntity[link.source_id];
        auto& sink_entity = m_idToEntity[link.sink_id];

        std::cout << "Source Pad Count: " << source_entity.pads.size() << std::endl;
        std::cout << "Sink Pad Count: " << sink_entity.pads.size() << std::endl;
        for (auto& pad : source_entity.pads) {
            std::cout << "Pad Id: " << pad.id << std::endl;
        }
        
        auto source_pad = std::find_if(source_entity.pads.begin(), source_entity.pads.end(),
            [&](const media_v2_pad& pad) { return (pad.id == link.source_id) /*&& (pad.flags & MEDIA_PAD_FL_SOURCE)*/; });
        auto sink_pad = std::find_if(sink_entity.pads.begin(), sink_entity.pads.end(),
            [&](const media_v2_pad& pad) { return (pad.id == link.sink_id) /*&& (pad.flags & MEDIA_PAD_FL_SINK) */; });

        if (source_pad != source_entity.pads.end() && sink_pad != sink_entity.pads.end()) {
            link_desc.source.index = source_pad->index;
            link_desc.sink.index = sink_pad->index;

            if (ioctl(m_fd, MEDIA_IOC_SETUP_LINK, &link_desc) < 0) {
                std::cout << "Warning: Failed to disable link from entity " 
                          << link.source_id << " to entity " << link.sink_id 
                          << ": " << strerror(errno) << std::endl;
                continue;
            }
        } else {
            std::cout << "Warning: Unable to find appropriate source or sink pad for link" << std::endl;
            continue;
        }
    }

    std::cout << "All links have been reset.\n";
    return true;
}

bool MediaController::setupLink(const std::string& source, const std::string& sink, int flags) {
    __u32 source_entity_id;
    if (!findEntityId(source, source_entity_id)) {
        std::cout << "Invalid source entity name: " << source << std::endl;
        return false;
    }

    __u32 sink_entity_id;
    if (!findEntityId(sink, sink_entity_id)) {
        std::cout << "Invalid sink entity name: " << sink << std::endl;
        return false;
    }

    auto& source_entity = m_idToEntity[source_entity_id];
    auto& sink_entity = m_idToEntity[sink_entity_id];
    
    struct media_link_desc link_desc = {0};
    link_desc.source.entity = source_entity_id;
    link_desc.sink.entity = sink_entity_id;
    link_desc.flags = flags;

    auto source_pad = std::find_if(source_entity.pads.begin(), source_entity.pads.end(),
        [](const media_v2_pad& pad) { return pad.flags & MEDIA_PAD_FL_SOURCE; });
    auto sink_pad = std::find_if(sink_entity.pads.begin(), sink_entity.pads.end(),
        [](const media_v2_pad& pad) { return pad.flags & MEDIA_PAD_FL_SINK; });

    if (source_pad == source_entity.pads.end() || sink_pad == sink_entity.pads.end()) {
        std::cout << "Unable to find appropriate source or sink pad" << std::endl;
        return false;
    }

    link_desc.source.index = source_pad->index;
    link_desc.sink.index = sink_pad->index;

    if (ioctl(m_fd, MEDIA_IOC_SETUP_LINK, &link_desc) < 0) {
        std::cout << "Failed to setup link: " << std::string(strerror(errno)) << std::endl;
        return false;
    }

    std::cout << "Link setup successful." << std::endl;

    return true;
}

bool MediaController::setFormat(const std::string& entity, int padIndex, const std::string& format, int width, int height) {
    __u32 entity_id;
    if (!findEntityId(entity, entity_id)) {
        std::cout << "Invalid entity name: " << entity << std::endl;
        return false;
    }

    // Check if the specified pad index exists for this entity
    auto entity_it = m_idToEntity.find(entity_id);
    if (entity_it == m_idToEntity.end() || 
        padIndex >= static_cast<int>(entity_it->second.pads.size())) {
        std::cout << "Invalid entity or pad index" << std::endl;
        return false;
    }

    struct v4l2_subdev_format fmt;
    memset(&fmt, 0, sizeof(fmt));

    fmt.pad = padIndex;
    fmt.which = V4L2_SUBDEV_FORMAT_ACTIVE;
    fmt.format.width = width;
    fmt.format.height = height;
    
    // Set the format code based on the string input
    // This is a simplified example; you might need to expand this
    if (format == "UYVY8_1X16") {
        fmt.format.code = MEDIA_BUS_FMT_UYVY8_1X16;
    } else {
        std::cout << "Unsupported format: "  << format << std::endl;
        return false;
    }

    std::cout << "Finding subdev: " << entity_id << std::endl;
    std::string subdev_path = getSubdevPath(entity_id);
    if (subdev_path.empty()) return false;
    //std::string subdev_path = "/dev/v4l-subdev" + std::to_string(entity_id);
    int subdev_fd = open(subdev_path.c_str(), O_RDWR);
    if (subdev_fd < 0) {
        std::cout << "Failed to open subdevice: " << subdev_path << " " << std::string(strerror(errno)) << std::endl;
        return false;
    }

    if (ioctl(subdev_fd, VIDIOC_SUBDEV_S_FMT, &fmt) < 0) {
        close(subdev_fd);
        std::cout << "Failed to set format: " << std::string(strerror(errno)) << std::endl;
        return false;
    }

    close(subdev_fd);
    std::cout << "Format set successfully for entity " << entity << " pad " << padIndex << "\n";

    return true;
}

void MediaController::printTopology() {
    std::cout << "Media Topology:\n";
    
    // Print Entities and their Pads
    std::cout << "Entities: " << m_idToEntity.size() << std::endl;
    for (const auto& [id, info] : m_idToEntity) {
        std::cout << "  Entity " << id << ": name=" << info.name << std::endl;
        std::cout << "    Pads:" << std::endl;
        for (const auto& pad : info.pads) {
            std::cout << "      Pad " << pad.index << ": ";
            if (pad.flags & MEDIA_PAD_FL_SINK) {
                std::cout << "Sink";
            } else if (pad.flags & MEDIA_PAD_FL_SOURCE) {
                std::cout << "Source";
            } else {
                std::cout << "Unknown type";
            }
            std::cout << std::endl;
        }
    }

    // Print Links
    std::cout << std::endl;
    std::cout << "Links: " << m_links.size() << std::endl;
    for (const auto& link : m_links) {
        std::cout << "  Link " << link.id << ": "
                  << m_idToEntity[link.source_id].name 
                  << " (Entity " << link.source_id << ") -> "
                  << m_idToEntity[link.sink_id].name
                  << " (Entity " << link.sink_id << ")"
                  << " [Flags: " << link.flags << "]"
                  << std::endl;
    }
}


// PRIVATE
std::map<std::string, std::string>
MediaController::fetchDevices() {
    std::map<std::string, std::string> deviceMap;

    for (int i = 0; i < 64; i++) {
        std::string devicePath = "/dev/media" + std::to_string(i);
        
        int fd = open(devicePath.c_str(), O_RDWR);
        if (fd == -1) continue;

        struct media_device_info mdi;
        if (ioctl(fd, MEDIA_IOC_DEVICE_INFO, &mdi) == 0) {
            deviceMap[std::string(mdi.model)] = std::string(devicePath);
        }
        
        close(fd);
    }

    return deviceMap;
}

bool MediaController::findEntityId(const std::string& name, __u32& outId) {
    auto it = std::find_if(m_idToEntity.begin(), m_idToEntity.end(),
        [&name](const auto& pair) { return pair.second.name == name; });
    
    if (it != m_idToEntity.end()) {
        outId = it->first;
        return true;
    }

    std::cout << "Entity not found: " << name << std::endl;
    return false;
}

namespace fs = std::filesystem;
std::string readSymlink(const std::string& path) {
    try {
        fs::path symlinkPath(path);
        fs::path target = fs::read_symlink(symlinkPath);
        return target.string();
    } catch (const fs::filesystem_error& e) {
        std::cout << "Filesystem error: " << e.what() << std::endl;
        std::cout << "Error code: " << e.code() << std::endl;
        return "";
    } catch (const std::exception& e) {
        std::cout << "Unexpected error: " << e.what() << std::endl;
        return "";
    }
}

std::string MediaController::getSubdevPath(uint32_t entity_id) {
    struct media_v2_topology topology = {0};
    std::vector<media_v2_entity> entities;
    std::vector<media_v2_interface> interfaces;
    std::vector<media_v2_link> links;

    // First query to get counts
    if (ioctl(m_fd, MEDIA_IOC_G_TOPOLOGY, &topology) < 0) {
        std::cout << "Failed to query topology: " << std::string(strerror(errno)) << std::endl;
        return std::string();
    }

    // Resize vectors based on counts
    entities.resize(topology.num_entities);
    interfaces.resize(topology.num_interfaces);
    links.resize(topology.num_links);

    // Second query to retrieve actual data
    topology.ptr_entities = reinterpret_cast<__u64>(entities.data());
    topology.ptr_interfaces = reinterpret_cast<__u64>(interfaces.data());
    topology.ptr_links = reinterpret_cast<__u64>(links.data());
    if (ioctl(m_fd, MEDIA_IOC_G_TOPOLOGY, &topology) < 0) {
        std::cout << "Failed to retrieve topology data: " << std::string(strerror(errno)) << std::endl;
        return std::string();
    }

    // Find the interface connected to the given entity
    bool found = false;
    uint32_t interface_id = 0;
    for (const auto& link : links) {
        if (link.sink_id == entity_id && (link.flags & MEDIA_LNK_FL_INTERFACE_LINK)) {
            interface_id = link.source_id;
            found = true;
            break;
        }
    }

    if (!found) {
        std::cout << "No interface found for entity ID: " << std::to_string(entity_id) << std::endl;
        return std::string();
    }

    // Find the interface with the matching ID
    for (const auto& intf : interfaces) {
        if (intf.id == interface_id && intf.intf_type == MEDIA_INTF_T_V4L_SUBDEV) {
            struct media_v2_intf_devnode devnode = intf.devnode;
            int major = intf.devnode.major;
            int minor = intf.devnode.minor;
            std::string symLink = "/sys/dev/char/" + std::to_string(major) + ":" + std::to_string(minor);
            std::string dest = readSymlink(symLink);
            size_t pos = dest.find_last_of("/");
            if (pos == std::string::npos) {
                std::cout << "Symlink points to an invalid device path: " << dest << std::endl;
                return std::string();
            }
            int l = dest.size() - (pos + 1);
            std::string deviceName = dest.substr(pos+1, l);
            std::string devicePath = "/dev/" + deviceName;
            return std::string(devicePath);
        }
    }

    std::cout << "No associated subdevice found for entity ID: " << std::to_string(entity_id) << std::endl;
    return std::string();
}


