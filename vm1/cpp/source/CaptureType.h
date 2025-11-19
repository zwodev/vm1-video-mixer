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