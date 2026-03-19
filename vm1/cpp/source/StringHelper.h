#pragma once

#include <sstream>
#include <iomanip> 
#include <filesystem>

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
};