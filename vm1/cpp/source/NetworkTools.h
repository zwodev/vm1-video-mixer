#pragma once

#include <iostream>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>

namespace NetworkTools 
{
    static bool getIPAddress(const std::string& deviceName, std::string& ipAddress) {
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

}