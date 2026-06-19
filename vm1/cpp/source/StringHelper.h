/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <sstream>
#include <iomanip> 
#include <filesystem>
#include <cmath>

namespace strhlpr {
    inline std::string formatFloat(double value, int decimals) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(decimals) << value;
        return oss.str();
    }

    // void removeAllWhitespaces(std::string& s) {
    //     // Remove ALL whitespace (including internal spaces/tabs/etc.)
    //     s.erase(std::remove_if(s.begin(), s.end(), 
    //         [](unsigned char ch) { return std::isspace(ch); }), s.end());
    // }

    inline void searchAndReplace(std::string& str, const std::string& toReplace, const std::string& replaceWith) {
        std::string result = str;
        size_t pos = 0;

        while ((pos = result.find(toReplace)) != std::string::npos) {
            result.replace(pos, toReplace.size(), replaceWith);
        }

        str = result;
    }

    inline bool isFile(const std::string& path) {
        std::filesystem::path filePath(path);
        return std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath);
    }

    inline std::string secondsToSmpte(double seconds, double fps) {
        if (fps <= 0.0) return "00.00";
        int nominalFps = (int)round(fps);
        int64_t totalFrames = (int64_t)round(seconds * fps);
        int secs   = (int)(totalFrames / nominalFps);
        int frames = (int)(totalFrames % nominalFps);
        return std::to_string(secs) + "." + (frames < 10 ? "0" : "") + std::to_string(frames);
    }

    inline double secondsToNormalized(double seconds, double totalDuration) {
        if (totalDuration <= 0.0) return 0.0;
        return seconds / totalDuration;
    }

};