#include <iostream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <sys/socket.h>
#include <algorithm>

#define MAX_IP_ADDR_LEN 15
#define MAX_QUERY_LEN 1024
#define TOTAL_CLIENT_PORTS 6
#define TRACKER_ADDRESS "127.0.0.1"
#define MAX_NAME_LEN 50
#define MAX_CONTACTS 10
#define MAX_PORT_LENGTH 5
#define MAX_USERS 10
#define SCHAR sizeof(char)

std::vector<int> available_ports = {8080, 8081}, client_available_ports = {8082, 8083, 8084, 8086, 8087, 8088};

struct peer_t
{
    int peer_id;       // Unused
    char peer_ipaddr[MAX_IP_ADDR_LEN];       // Unused
    char peer_name[MAX_NAME_LEN];       // Unused
    char* contacts[MAX_CONTACTS];
    char* contacts_ipaddr[MAX_CONTACTS];
    int contacts_port[MAX_CONTACTS];
};

struct tracker_t
{
    char* directory[MAX_USERS];
    char* peers_ipaddr[MAX_USERS];
    char* port[MAX_USERS];
};

void get_current_ip(char ip_addr[]) {
    struct ifaddrs *ifAddrStruct = nullptr;
    struct ifaddrs *ifa = nullptr;
    void *tmpAddrPtr = nullptr;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) {
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, tmpAddrPtr, ip_addr, 16);
            if (strcmp(ifa->ifa_name, "lo") != 0) {
                if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
                std::cout << "Current IPv4 Address: " << ip_addr << std::endl;
                return;
            }
        }
    }
    if (ifAddrStruct != nullptr) freeifaddrs(ifAddrStruct);
    return;
}
