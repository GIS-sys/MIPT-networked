#include "common.h"


ssize_t CommonCS::_send(const std::string& msg, int sfd, sockaddr_in client_sockaddr) const {
    std::cout << "(sending message: " << msg << ")" << std::endl;
    ssize_t res = sendto(sfd, msg.c_str(), msg.size(), 0, (sockaddr*)&client_sockaddr, sizeof(client_sockaddr));
    if (res == -1) std::cout << "(error while sending message: " << strerror(errno) << ")" << std::endl;
    return res;
}

ReadResult CommonCS::_read(int sfd) const {
    // fd_set containing only sfd
    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(sfd, &readFds);

    // Timeout
    timeval tv = {0, SERVER_RECEIVE_TIMEOUT_US};

    // See if there are updates for any fd <= sfd
    int ready = select(sfd + 1, &readFds, nullptr, nullptr, &tv);

    // Process edge cases
    if (ready < 0) {
        // perror("select error");
        return ReadResult{ .is_error = true, .is_empty = true };
    }
    if (ready == 0) {
        // std::cout << "no response from server?" << std::endl;
        return ReadResult{ .is_error = false, .is_empty = true };
    }

    // Read message
    char buffer[MESSAGE_BUFFER_SIZE];
    sockaddr_in responderAddr;
    socklen_t addrLen = sizeof(responderAddr);
    ssize_t numBytes = recvfrom(sfd, buffer, sizeof(buffer), 0, (sockaddr*)&responderAddr, &addrLen);

    if (numBytes > 0) {
        buffer[numBytes] = '\0';
    }
    char* responder_addr = inet_ntoa(responderAddr.sin_addr);
    int responder_port = ntohs(responderAddr.sin_port);
    std::cout << "(got message from client " << responder_addr << ":" << responder_port << ": " << buffer << ")" << std::endl;
    return ReadResult{ .msg = buffer, .responder_sockaddr = responderAddr, .is_error = false, .is_empty = false };
}

