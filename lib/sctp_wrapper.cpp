/****************************************************************************
 * file: sctp_wrapper.cpp
 * author: luke le
 * description:
 *     implements a thin C++ wrapper around the SCTP socket API to simplify
 *     creation, connection, and communication over SCTP sockets.
 * notes:
 *     this class encapsulates common operations such as socket creation,
 *     binding, listening, accepting, connecting, sending, and receiving.
 *     It uses the 1-to-1 SCTP model, which closely resembles the TCP socket
 *     API, while enabling SCTP features like reliable message delivery.
 ****************************************************************************/
#include "sctp_wrapper.hpp"

#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <cstring>

// **************************************************************************
// constructor / destructor
// **************************************************************************
/**
 * @brief Default constructor
 *
 * Initializes the socket file descriptor to -1 and zeroes out the address
 * structure. The actual socket is not created until create() is called.
 */
SCTPSocket::SCTPSocket() : sockfd(-1) {
    std::memset(&addr, 0, sizeof(addr));
} // SCTPSocket()

/**
 * @brief Destructor
 *
 * Ensures that any open socket is properly closed when the object goes
 * out of scope.
 */
SCTPSocket::~SCTPSocket() {
    close();
} // ~SCTPSocket()

// **************************************************************************
// socket setup
// **************************************************************************
/**
 * @brief create an SCTP socket; calls socket() with AF_INET, SOCK_STREAM, and
 * IPPROTO_SCTP to create an SCTP socket in the 1-to-1 style (similar to TCP).
 *
 * @return true if socket creation succeeded, false otherwise
 */
// lib/sctp_wrapper.cpp
bool SCTPSocket::create() {
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0) {
        std::cerr << "[!] Failed to create SCTP socket\n";
        return false;
    }
    int yes = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    return true;
}

/**
 * @brief bind the SCTP socket to a local port configures the sockaddr_in
 * structure with INADDR_ANY to accept connections from any interface, and
 * binds it to the given port.
 *
 * @param port port number to bind to
 * @return true if binding succeeded, false otherwise
 */
bool SCTPSocket::bind(int port) {
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[!] Failed to bind SCTP socket\n";
        return false;
    }
    return true;
} // bind()

/**
 * @brief mark the socket as passive to accept incoming connections; wraps the
 * standard listen() call. SCTP uses the same function as TCP in the 1-to-1
 * model.
 *
 * @param backlog max number of pending connections
 * @return true if listen succeeded, false otherwise
 */
bool SCTPSocket::listen(int backlog) {
    if (::listen(sockfd, backlog) < 0) {
        std::cerr << "[!] Failed to listen on SCTP socket\n";
        return false;
    }
    return true;
} // listen()

// **************************************************************************
// connection management
// **************************************************************************
/**
 * @brief accept an incoming SCTP connection; waits for an incoming connection
 * on the bound SCTP socket. On success, initializes the provided SCTPSocket
 * object with the accepted file descriptor and client address.
 *
 * @param clientSocket reference to an SCTPSocket to hold the accepted connection
 * @return true if accept succeeded, false otherwise
 */
bool SCTPSocket::accept(SCTPSocket &clientSocket) {
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);
    int clientFd = ::accept(sockfd, (sockaddr*)&clientAddr, &len);
    if (clientFd < 0) {
        std::cerr << "[!] Failed to accept SCTP connection\n";
        return false;
    }
    clientSocket.sockfd = clientFd;
    clientSocket.addr = clientAddr;
    return true;
} // accept()

/**
 * @brief Connect to a remote SCTP server; resolves the hostname to an IP
 * address using gethostbyname(), then attempts to connect to the specified
 * port on that host.
 *
 * @param host hostname or IP address to connect to
 * @param port remote port number
 * @return true if connection succeeded, false otherwise
 */
bool SCTPSocket::connect(const std::string &host, int port) {
    hostent *server = gethostbyname(host.c_str());
    if (!server) {
        std::cerr << "[!] No such host: " << host << "\n";
        return false;
    }

    addr.sin_family = AF_INET;
    std::memcpy(&addr.sin_addr.s_addr, server->h_addr, server->h_length);
    addr.sin_port = htons(port);

    if (::connect(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[!] Failed to connect to " << host << ":" << port << "\n";
        return false;
    }
    return true;
} // connect()

// **************************************************************************
// data transfer
// **************************************************************************
/**
 * @brief send a message over SCTP; Uses sctp_sendmsg() to transmit a message
 * on the established SCTP socket. Flags and stream identifiers are set to
 * zero for simplicity.
 *
 * @param message data to send
 * @return true if the message was sent successfully, false otherwise
 */
bool SCTPSocket::send(const std::string &message) {
    int flags = 0;
    int ret = sctp_sendmsg(sockfd, message.c_str(), message.size(),
                           nullptr, 0, 0, 0, 0, 0, flags);
    if (ret < 0) {
        std::cerr << "[!] Failed to send SCTP message\n";
        return false;
    }
    return true;
} // send()

/**
 * @brief Receive a message over SCTP; uses sctp_recvmsg() to receive data
 * from the SCTP socket. This function blocks until data is available. The
 * received bytes are stored in the provided string.
 *
 * @param message 0utput string to hold the received data
 * @return true if message received successfully, false otherwise
 */
bool SCTPSocket::receive(std::string &message) {
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));
    int flags = 0;
    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);

    int ret = sctp_recvmsg(sockfd, buffer, sizeof(buffer), 
                           (sockaddr*)&peerAddr, &len, nullptr, &flags);
    if (ret < 0) {
        std::cerr << "[!] Failed to receive SCTP message\n";
        return false;
    }

    message = std::string(buffer, ret);
    return true;
} // receive()

sockaddr_in SCTPSocket::get_peer_addr() const {
    return addr;
}

// **************************************************************************
// cleanup
// **************************************************************************
/**
 * @brief close the SCTP socket if open; closes the underlying file descriptor
 * and resets it to -1.
 */
void SCTPSocket::close() {
    if (sockfd >= 0) {
        ::close(sockfd);
        sockfd = -1;
    }
} // close()
