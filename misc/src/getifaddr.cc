#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include "getifaddr.h"
bool getFirstNonLoopbackAddress (std::string& saddr) {
    struct ifaddrs *ifs = NULL;
    int status = getifaddrs(&ifs);
    if (status != 0) {
        return false;
    }
    struct ifaddrs *current = ifs;
    bool success = false;
    while (current) {
        if (current->ifa_addr->sa_family == AF_INET && !(current->ifa_flags & IFF_LOOPBACK)) {
            struct sockaddr_in *inet4addr  = (struct sockaddr_in*) current->ifa_addr;
            char addrSpace[INET_ADDRSTRLEN];
            const char *addr = inet_ntop(inet4addr->sin_family, &inet4addr->sin_addr, addrSpace, INET_ADDRSTRLEN);
            if (addr) {
                saddr = addr; 
                success = true;
                break;
            } else {
                success = false;
                break;
            }
        }         
        current = current->ifa_next;
    };
    freeifaddrs(ifs);
    return success;
}
