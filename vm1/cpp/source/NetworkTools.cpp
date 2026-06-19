/*
 * Copyright (c) 2023-2026 Nils Zweiling & Julian Jungel
 *
 * This file is part of VM-1 which is released under the MIT license.
 * See file LICENSE or go to https://github.com/zwodev/vm1-video-mixer/tree/master/LICENSE
 * for full license details.
 */

#include <iostream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <array>
#include <memory>
#include <sstream> 
#include "NetworkTools.h"

bool NetworkTools::getIPAddress(const std::string& deviceName, std::string& ipAddress) {
    bool success = false;
    struct ifaddrs *ifaddr, *ifa;
    char ip[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        return success;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) { // IPv4
            if (std::strcmp(ifa->ifa_name, deviceName.c_str()) == 0) {
                void* addr_ptr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
                inet_ntop(AF_INET, addr_ptr, ip, INET_ADDRSTRLEN);
                ipAddress = std::string(ip);
                success = true;
            }
        }
    }

    freeifaddrs(ifaddr);
    return success;
}

std::string NetworkTools::runCommand(const std::string& cmd) {
    std::array<char, 256> buf;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) throw std::runtime_error("popen failed");
    while (fgets(buf.data(), buf.size(), pipe.get()))
        result += buf.data();
    // strip trailing newline
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}


std::string NetworkTools::findAPConnection() {
    // Get all wifi connection names
    std::string wifiConns = runCommand(
        "nmcli -t -f NAME,TYPE connection show"
        " | awk -F: '$2==\"802-11-wireless\" {print $1}'"
    );
    if (wifiConns.empty()) return {};

    // For each, check if mode=ap
    std::istringstream stream(wifiConns);
    std::string name;
    while (std::getline(stream, name)) {
        std::string mode = runCommand(
            "nmcli -t -f 802-11-wireless.mode connection show \"" + name + "\""
            " | cut -d: -f2"
        );
        if (mode == "ap") return name;
    }
    return {};
}    

NetworkTools::APCredentials NetworkTools::getAPCredentials() {
    std::string conn = findAPConnection();
    if (conn.empty()) throw std::runtime_error("No AP profile found");

    APCredentials creds;
    creds.ssid = runCommand(
        "nmcli -t -f 802-11-wireless.ssid connection show \"" + conn + "\""
        " | cut -d: -f2"
    );
    creds.psk = runCommand(
        "sudo nmcli -s -t -f 802-11-wireless-security.psk connection show \"" + conn + "\""
        " | cut -d: -f2"
    );
    return creds;
}

