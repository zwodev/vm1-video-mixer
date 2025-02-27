
#include "V4L2Controller.h"

V4L2Controller::V4L2Controller(const std::string& device) : device_path(device) {
    fd = open(device_path.c_str(), O_RDWR);
    if (fd < 0) {
        throw std::runtime_error("Failed to open device: " + device_path);
    }
}

V4L2Controller::~V4L2Controller() {
    if (fd >= 0) {
        close(fd);
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

void V4L2Controller::setEdid(const std::string& edidFile) {
    std::vector<unsigned char> edid_data(MAX_EDID_SIZE);
    struct v4l2_subdev_edid edid = {0};

    // Read EDID from file
    std::ifstream file(edidFile, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open EDID file: " + edidFile);
    }

    file.read(reinterpret_cast<char*>(edid_data.data()), MAX_EDID_SIZE);
    edid.blocks = file.gcount() / 128;
    file.close();

    if (edid.blocks == 0) {
        throw std::runtime_error("Failed to read EDID data");
    }

    // Fix EDID checksums
    for (int i = 0; i < edid.blocks; i++) {
        unsigned char sum = 0;
        for (int j = 0; j < 127; j++)
            sum += edid_data[i * 128 + j];
        edid_data[i * 128 + 127] = (unsigned char)(256 - sum);
    }

    // Set EDID
    edid.pad = 0;
    edid.start_block = 0;
    edid.edid = edid_data.data();

    if (ioctl(fd, VIDIOC_SUBDEV_S_EDID, &edid) < 0) {
        throw std::runtime_error("Failed to set EDID");
    }

    std::cout << "EDID set successfully\n";
}

v4l2_dv_timings V4L2Controller::queryDvTimings() {
    v4l2_dv_timings timings;

    if (ioctl(fd, VIDIOC_QUERY_DV_TIMINGS, &timings) < 0) {
        throw std::runtime_error("Failed to query DV timings");
    }

    return timings;
}

void V4L2Controller::setDvTimings() {
    v4l2_dv_timings timings = queryDvTimings();

    if (ioctl(fd, VIDIOC_S_DV_TIMINGS, &timings) < 0) {
        throw std::runtime_error("Failed to set DV timings");
    }

    std::cout << "DV timings set successfully\n";
}

void V4L2Controller::printDvTimings(const v4l2_dv_timings& timings) {
    std::cout << "DV Timings:\n";
    std::cout << "Type: " << timings.type << "\n";
    std::cout << "Width: " << timings.bt.width << "\n";
    std::cout << "Height: " << timings.bt.height << "\n";
    std::cout << "Interlaced: " << (timings.bt.interlaced ? "Yes" : "No") << "\n";
    std::cout << "Pixelclock: " << timings.bt.pixelclock << " Hz\n";
}
