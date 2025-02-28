
#include "V4L2Controller.h"

V4L2Controller::V4L2Controller() {

}

V4L2Controller::~V4L2Controller() {
    if (fd >= 0) {
        close(fd);
    }
}

bool V4L2Controller::openDevice(const std::string& devicePath) {
    fd = open(devicePath.c_str(), O_RDWR);
    if (fd < 0) {
        std::cout << "Failed to open device: " << devicePath << std::endl;
        return false;
    }

    return true;
}

void V4L2Controller::closeDevice() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

void V4L2Controller::listDevices() {
    // List video devices
    for (int i = 0; i < 64; i++) {
        char device_path[20];
        snprintf(device_path, sizeof(device_path), "/dev/video%d", i);
        
        int fd = open(device_path, O_RDWR);
        if (fd == -1) continue;

        struct v4l2_capability cap;
        if (ioctl(fd, VIDIOC_QUERYCAP, &cap) == 0) {
            if (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) {
                printf("Device path: %s", device_path);
                printf("Found device: %s\n", cap.card);
            }
        }
        
        close(fd);
    }

}

bool V4L2Controller::setEdid(const std::vector<unsigned char>& edid) {
    struct v4l2_edid v4l2_edid = {};
    v4l2_edid.pad = 0;
    v4l2_edid.start_block = 0;
    v4l2_edid.blocks = edid.size() / 128;
    v4l2_edid.edid = const_cast<unsigned char*>(edid.data());

    if (ioctl(fd, VIDIOC_S_EDID, &v4l2_edid) < 0) {
        close(fd);
        std::cout << "Failed to set EDID data" << std::endl;
        return false;
    }

    std::cout << "EDID set successfully" << std::endl;
    return true;
}

bool V4L2Controller::setEdid(const std::string& edidFile) {
    std::vector<unsigned char> edid_data(MAX_EDID_SIZE);
    struct v4l2_subdev_edid edid = {0};

    // Read EDID from file
    std::ifstream file(edidFile, std::ios::binary);
    if (!file) {
        std::cout << "Failed to open EDID file: " << edidFile;
        return false;
    }

    file.read(reinterpret_cast<char*>(edid_data.data()), MAX_EDID_SIZE);
    edid.blocks = file.gcount() / 128;
    file.close();

    if (edid.blocks == 0) {
        std::cout << "Failed to read EDID data" << std::endl;
        return false;
    }

    // // Fix EDID checksums
    // for (int i = 0; i < edid.blocks; i++) {
    //     unsigned char sum = 0;
    //     for (int j = 0; j < 127; j++)
    //         sum += edid_data[i * 128 + j];
    //     edid_data[i * 128 + 127] = (unsigned char)(256 - sum);
    // }

    // Set EDID
    edid.pad = 0;
    edid.start_block = 0;
    edid.edid = edid_data.data();

    if (ioctl(fd, VIDIOC_S_EDID, &edid) < 0) {
        std::cout << "Failed to set EDID" << std::endl;
        return false;
    }

    std::cout << "EDID set successfully\n";
    return true;
}

v4l2_dv_timings V4L2Controller::queryDvTimings() {
    v4l2_dv_timings timings = {0};

    if (ioctl(fd, VIDIOC_QUERY_DV_TIMINGS, &timings) < 0) {
        std::cout << "Failed to query DV timings" << std::endl;
        return timings;
    }

    return timings;
}

bool V4L2Controller::setDvTimings() {
    v4l2_dv_timings timings = queryDvTimings();

    if (ioctl(fd, VIDIOC_S_DV_TIMINGS, &timings) < 0) {
        std::cout << "Failed to set DV timings" << std::endl;
        return false;
    }

    std::cout << "DV timings set successfully" << std::endl;
    return true;
}

void V4L2Controller::printDvTimings(const v4l2_dv_timings& timings) {
    std::cout << "DV Timings:" << std::endl;
    std::cout << "Type: " << timings.type << std::endl;
    std::cout << "Width: " << timings.bt.width << std::endl;
    std::cout << "Height: " << timings.bt.height << std::endl;
    std::cout << "Interlaced: " << (timings.bt.interlaced ? "Yes" : "No") << std::endl;
    std::cout << "Pixelclock: " << timings.bt.pixelclock << " Hz" << std::endl;
}
