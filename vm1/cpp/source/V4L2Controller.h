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
    V4L2Controller(const std::string& device);
    ~V4L2Controller();

    static void listDevices();
    void setEdid(const std::string& edid_file);
    v4l2_dv_timings queryDvTimings();
    void setDvTimings();
    void printDvTimings(const v4l2_dv_timings& timings);
};