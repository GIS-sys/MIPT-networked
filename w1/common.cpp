#include "common.h"
#include <netinet/in.h>


ssize_t CommonCS::_send(const std::string& msg, int sfd, sockaddr_in client_sockaddr) const {
    std::cout << "(sending message to "
              << ReadResult::calc_responder_addr(client_sockaddr) << ":" << ReadResult::calc_responder_port(client_sockaddr) << ": "
              << msg << ")" << std::endl;
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
    sockaddr_in responder_sockaddr;
    socklen_t addrLen = sizeof(responder_sockaddr);
    ssize_t numBytes = recvfrom(sfd, buffer, sizeof(buffer), 0, (sockaddr*)&responder_sockaddr, &addrLen);

    if (numBytes > 0) {
        buffer[numBytes] = '\0';
    }
    ReadResult result{ .msg = buffer, .responder_sockaddr = responder_sockaddr, .is_error = false, .is_empty = false };
    std::cout << "(got message from "
              << result.get_responder_addr() << ":" << result.get_responder_port() << ": "
              << buffer << ")" << std::endl;
    return result;
}

