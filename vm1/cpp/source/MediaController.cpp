/*
 * Copyright (c) 2023-2024 Nils Zweiling
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */


#include "MediaController.h"

MediaController::MediaController(const std::string& mediaDevice) {
    device_path = "/dev/media" + mediaDevice;
    fd = open(device_path.c_str(), O_RDWR);
    if (fd < 0) {
        throw std::runtime_error("Failed to open media device: " + std::string(strerror(errno)));
    }
}

MediaController::~MediaController() {
    if (fd >= 0) {
        close(fd);
    }
}

void MediaController::getTopology() {
    struct media_v2_topology topology = {0};
    std::vector<media_v2_entity> entity_array;
    std::vector<media_v2_pad> pad_array;
    std::vector<media_v2_link> link_array;

    if (ioctl(fd, MEDIA_IOC_G_TOPOLOGY, &topology) < 0) {
        throw std::runtime_error("Failed to get topology: " + std::string(strerror(errno)));
    }

    entity_array.resize(topology.num_entities);
    pad_array.resize(topology.num_pads);
    link_array.resize(topology.num_links);

    topology.ptr_entities = reinterpret_cast<__u64>(entity_array.data());
    topology.ptr_pads = reinterpret_cast<__u64>(pad_array.data());
    topology.ptr_links = reinterpret_cast<__u64>(link_array.data());

    if (ioctl(fd, MEDIA_IOC_G_TOPOLOGY, &topology) < 0) {
        throw std::runtime_error("Failed to get topology: " + std::string(strerror(errno)));
    }

    entities.clear();
    links.clear();

    // Process entities and their pads
    for (const auto& entity : entity_array) {
        EntityInfo entity_info = {entity.id, std::string(entity.name), {}};
        for (const auto& pad : pad_array) {
            if (pad.entity_id == entity.id) {
                entity_info.pads.push_back(pad);
            }
        }
        entities[entity.id] = entity_info;
    }

    // Process links
    for (const auto& link : link_array) {
        links.push_back({link.id, link.source_id, link.sink_id, link.flags});
    }
}

void MediaController::resetLinks() {
    for (auto& link : links) {
        struct media_link_desc link_desc = {0};
        link_desc.source.entity = link.source_id;
        link_desc.sink.entity = link.sink_id;
        link_desc.flags = 0;  // Disable the link

        // Find the correct pad indices
        auto& source_entity = entities[link.source_id];
        auto& sink_entity = entities[link.sink_id];

        auto source_pad = std::find_if(source_entity.pads.begin(), source_entity.pads.end(),
            [](const media_v2_pad& pad) { return pad.flags & MEDIA_PAD_FL_SOURCE; });
        auto sink_pad = std::find_if(sink_entity.pads.begin(), sink_entity.pads.end(),
            [](const media_v2_pad& pad) { return pad.flags & MEDIA_PAD_FL_SINK; });

        if (source_pad != source_entity.pads.end() && sink_pad != sink_entity.pads.end()) {
            link_desc.source.index = source_pad->index;
            link_desc.sink.index = sink_pad->index;

            if (ioctl(fd, MEDIA_IOC_SETUP_LINK, &link_desc) < 0) {
                std::cerr << "Warning: Failed to disable link from entity " 
                          << link.source_id << " to entity " << link.sink_id 
                          << ": " << strerror(errno) << std::endl;
            }
        } else {
            std::cerr << "Warning: Unable to find appropriate source or sink pad for link" << std::endl;
        }
    }
    std::cout << "All links have been reset.\n";
}

void MediaController::setupLink(const std::string& source, const std::string& sink, int flags) {
    __u32 source_entity_id = findEntityId(source);
    __u32 sink_entity_id = findEntityId(sink);

    auto link_it = std::find_if(links.begin(), links.end(),
        [source_entity_id, sink_entity_id](const LinkInfo& link) {
            return link.source_id == source_entity_id && link.sink_id == sink_entity_id;
        });

    if (link_it == links.end()) {
        throw std::runtime_error("Link not found");
    }

    struct media_link_desc link_desc = {0};
    link_desc.source.entity = link_it->source_id;
    link_desc.sink.entity = link_it->sink_id;
    link_desc.flags = flags;

    // We need to find the correct pad indices
    auto& source_entity = entities[link_it->source_id];
    auto& sink_entity = entities[link_it->sink_id];

    auto source_pad = std::find_if(source_entity.pads.begin(), source_entity.pads.end(),
        [](const media_v2_pad& pad) { return pad.flags & MEDIA_PAD_FL_SOURCE; });
    auto sink_pad = std::find_if(sink_entity.pads.begin(), sink_entity.pads.end(),
        [](const media_v2_pad& pad) { return pad.flags & MEDIA_PAD_FL_SINK; });

    if (source_pad == source_entity.pads.end() || sink_pad == sink_entity.pads.end()) {
        throw std::runtime_error("Unable to find appropriate source or sink pad");
    }

    link_desc.source.index = source_pad->index;
    link_desc.sink.index = sink_pad->index;

    if (ioctl(fd, MEDIA_IOC_SETUP_LINK, &link_desc) < 0) {
        throw std::runtime_error("Failed to setup link: " + std::string(strerror(errno)));
    }
    std::cout << "Link setup successful.\n";
}

void MediaController::setFormat(const std::string& entity, int padIndex, const std::string& format, int width, int height) {
    __u32 entity_id = findEntityId(entity);

    // Check if the specified pad index exists for this entity
    auto entity_it = entities.find(entity_id);
    if (entity_it == entities.end() || 
        padIndex >= static_cast<int>(entity_it->second.pads.size())) {
        throw std::runtime_error("Invalid entity or pad index");
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
        throw std::runtime_error("Unsupported format: " + format);
    }

    std::string subdev_path = "/dev/v4l-subdev" + std::to_string(entity_id);
    int subdev_fd = open(subdev_path.c_str(), O_RDWR);
    if (subdev_fd < 0) {
        throw std::runtime_error("Failed to open subdevice: " + std::string(strerror(errno)));
    }

    if (ioctl(subdev_fd, VIDIOC_SUBDEV_S_FMT, &fmt) < 0) {
        close(subdev_fd);
        throw std::runtime_error("Failed to set format: " + std::string(strerror(errno)));
    }

    close(subdev_fd);
    std::cout << "Format set successfully for entity " << entity << " pad " << padIndex << "\n";
}

void MediaController::printTopology() {
    std::cout << "Media Topology:\n";
    
    // Print Entities and their Pads
    std::cout << "Entities: " << entities.size() << "\n";
    for (const auto& [id, info] : entities) {
        std::cout << "  Entity " << id << ": name=" << info.name << "\n";
        std::cout << "    Pads:\n";
        for (const auto& pad : info.pads) {
            std::cout << "      Pad " << pad.index << ": ";
            if (pad.flags & MEDIA_PAD_FL_SINK) {
                std::cout << "Sink";
            } else if (pad.flags & MEDIA_PAD_FL_SOURCE) {
                std::cout << "Source";
            } else {
                std::cout << "Unknown type";
            }
            std::cout << "\n";
        }
    }

    // Print Links
    std::cout << "\nLinks: " << links.size() << "\n";
    for (const auto& link : links) {
        std::cout << "  Link " << link.id << ": "
                  << entities[link.source_id].name 
                  << " (Entity " << link.source_id << ") -> "
                  << entities[link.sink_id].name
                  << " (Entity " << link.sink_id << ")"
                  << " [Flags: " << link.flags << "]\n";
    }
}


// PRIVATE
__u32 MediaController::findEntityId(const std::string& name) {
    auto it = std::find_if(entities.begin(), entities.end(),
        [&name](const auto& pair) { return pair.second.name == name; });
    if (it != entities.end()) {
        return it->first;
    }

    throw std::runtime_error("Entity not found: " + name);
}