#include <arpa/inet.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <random>
#include <string>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

#include "socket_tools.h"



int main() {
    const char *port = "2025";
    int sfd = create_dgram_socket(nullptr, port, nullptr);

    if (sfd == -1) {
        printf("Cannot create socket\n");
        return 1;
    }
    printf("Listening on port %s!\n", port);

    while (true) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sfd, &readSet);

        timeval timeout = {0, 100000}; // 100 ms
        select(sfd + 1, &readSet, NULL, NULL, &timeout);

        if (FD_ISSET(sfd, &readSet)) {
            char buffer[1000];
            sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            ssize_t numBytes = recvfrom(sfd, buffer, sizeof(buffer)-1, 0, (sockaddr*)&clientAddr, &clientAddrLen);

            if (numBytes > 0) {
                buffer[numBytes] = '\0';
                printf("Received from %s:%d: %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);

                // Send "pong" response
                const char *response = "pong";
                sendto(sfd, response, strlen(response), 0, (sockaddr*)&clientAddr, clientAddrLen);
            }
        }
    }

    close(sfd);
    return 0;
}

