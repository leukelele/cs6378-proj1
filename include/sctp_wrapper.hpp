/****************************************************************************
 * file: sctp_wrapper.hpp
 * author: luke le
 * description:
 *     declares class functions for socket management
 ****************************************************************************/
#ifndef SCTP_WRAPPER_HPP
#define SCTP_WRAPPER_HPP

#include <netinet/in.h>
#include <string>

class SCTPSocket {
public:
    SCTPSocket();
    ~SCTPSocket();

    bool create();
    bool bind(int port);
    bool listen(int backlog = 5);
    bool accept(SCTPSocket &clientSocket);
    bool connect(const std::string &host, int port);
    bool send(const std::string &message);
    bool receive(std::string &message);
    sockaddr_in get_peer_addr() const;

    void close();

private:
    int sockfd;
    sockaddr_in addr;
};

#endif // SCTP_WRAPPER_HPP
