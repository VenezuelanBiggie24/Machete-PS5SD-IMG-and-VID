// Detects the console's real LAN IP address: UDP routing lookup with getifaddrs fallback.
//
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <cstdlib>
#include <string>

namespace ps5sd {

inline std::string detect_console_ip() {
    // 1. Check for manual override in environment (env.txt)
    const char * env_ip = std::getenv("PS5SD_IP");
    if (env_ip == nullptr || env_ip[0] == '\0') {
        env_ip = std::getenv("PS5SD_ADDRESS");
    }
    if (env_ip != nullptr && env_ip[0] != '\0') {
        return std::string(env_ip);
    }

    // 2. Try UDP routing socket to default gateway.
    // Connecting a SOCK_DGRAM socket does not send any packets over the wire; it simply
    // asks the kernel routing table for the local interface IP chosen for outbound traffic.
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock >= 0) {
        struct sockaddr_in dest = {};
        dest.sin_family = AF_INET;
        dest.sin_port   = htons(53);
        inet_pton(AF_INET, "8.8.8.8", &dest.sin_addr);
        if (connect(sock, reinterpret_cast<const struct sockaddr *>(&dest), sizeof(dest)) == 0) {
            struct sockaddr_in local = {};
            socklen_t          len   = sizeof(local);
            if (getsockname(sock, reinterpret_cast<struct sockaddr *>(&local), &len) == 0) {
                char buf[INET_ADDRSTRLEN] = {};
                if (inet_ntop(AF_INET, &local.sin_addr, buf, sizeof(buf))) {
                    std::string ip(buf);
                    close(sock);
                    if (!ip.empty() && ip.rfind("127.", 0) != 0 && ip != "0.0.0.0") {
                        return ip;
                    }
                }
            }
        }
        close(sock);
    }

    // 3. Fallback: query ifaddrs (all active non-loopback IPv4 interfaces)
    struct ifaddrs * ifap = nullptr;
    if (getifaddrs(&ifap) == 0 && ifap != nullptr) {
        for (struct ifaddrs * ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
            if (ifa->ifa_addr == nullptr || ifa->ifa_addr->sa_family != AF_INET) continue;
            if (ifa->ifa_flags & IFF_LOOPBACK) continue;
            if (!(ifa->ifa_flags & IFF_UP)) continue;
            auto * sa = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
            char buf[INET_ADDRSTRLEN] = {};
            if (inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf))) {
                std::string ip(buf);
                if (!ip.empty() && ip.rfind("127.", 0) != 0 && ip != "0.0.0.0") {
                    freeifaddrs(ifap);
                    return ip;
                }
            }
        }
        freeifaddrs(ifap);
    }
    return "";
}

}  // namespace ps5sd
