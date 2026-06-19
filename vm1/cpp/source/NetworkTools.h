/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#pragma once

#include <iostream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

class NetworkTools {
public:

    NetworkTools() = delete;
    ~NetworkTools() = delete;
    
    struct APCredentials {
        std::string ssid;
        std::string psk;
    };

    static bool getIPAddress(const std::string& deviceName, std::string& ipAddress);
    static std::string runCommand(const std::string& cmd);
    static std::string findAPConnection();
    static APCredentials getAPCredentials();
};