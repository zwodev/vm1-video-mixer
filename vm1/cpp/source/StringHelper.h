#pragma once

#include <sstream>
#include <iomanip> 

namespace strhlpr {
    std::string formatFloat(double value, int decimals) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(decimals) << value;
        return oss.str();
    }
}