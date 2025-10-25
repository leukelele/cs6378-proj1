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

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netdb.h>
#include <netinet/sctp.h>
#include <unistd.h>

// initialize socket in a safe "empty" state (no valid fd, cleared address)
// - prevents accidental use of uninitialized descriptor
// - makes close() idempotent even if create() fails
SCTPSocket::SCTPSocket() : sockfd(-1) {
    // 'addr' is a sockaddr_in structure; it holds address info like family,
    // IP, and port; calling memset fills the entire structure with zero bytes
    std::memset(&addr, 0, sizeof(addr));
} // SCTPSocket()

// ensures that any open socket is properly closed when the object goes
// out of scope
SCTPSocket::~SCTPSocket() {
    close();
} // ~SCTPSocket()

bool SCTPSocket::create() {
    // creates a stream-oriented (SOCK_STREAM) socket bc the internet said to
    // do so. The scoket is configured for IPv4 addresses (AF_INET), as there
    // is no need to worry about anything beyond IPv4 for the dc0X servers;
    // while IPPROTO_SCTP sets SCTP for the transport layer protocol bc the 
    // prof. suggested this protocol makes design easier but is preferable bc
    // it support multiple streams.
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sockfd < 0) {
        std::cerr << "[!] failed to create SCTP socket\n";
        return false;
    }
    // this allows me to restart the program quickly on the same port
    // without waiting 30–60 seconds for the OS to free it
    int yes = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    // applies SCTP-specific tuning and ensures that every new SCTP socket is
    // created with:
    // - predictable behavior (1 outgoing / incoming stream),
    // - minimal startup delay, and 
    // - efficient message delivery.
    //
    // *internet supplied LOL
    set_defaults();
    return true;
} // create()

/**
 * @brief sets SCTP scocket with reasonable defaults.
 */
void SCTPSocket::set_defaults() {
    // sctp_initmsg is a struct defined by the SCTP API (<netinet/sctp.h>).
    // It controls association setup parameters aka how the SCTP connection
    // behaves when first established. memset() clears the struct to all
    // zeros to keep behaviors consistent.
    struct sctp_initmsg init{};
    std::memset(&init, 0, sizeof(init));

    init.sinit_num_ostreams = 1;  // num of outbound streams socket can send
    init.sinit_max_instreams = 1; // max num of inbound the sock can accept
    init.sinit_max_attempts = 4;  // max num SCTP will retransmit INIT chunk
                                  // aka handshake retry count

    // applies params to socket
    if (::setsockopt(sockfd, IPPROTO_SCTP, SCTP_INITMSG,
                     &init, sizeof(init)) < 0)
        std::perror("setsockopt(SCTP_INITMSG)");

    // disable Nagle’s algorithm for low-latency control messages, which is a
    // TCP/SCTP feature that batches small packets together before sending
    // them. Normally, Nagle’s algorithm helps network efficiency by combining
    // small writes but it adds latency because it waits to accumulate more
    // data before sending. This is no needed for this project.
    int one = 1;
    if (::setsockopt(sockfd, IPPROTO_SCTP, SCTP_NODELAY,
                     &one, sizeof(one)) < 0)
        std::perror("setsockopt(SCTP_NODELAY)");
} // set_defaults()

bool SCTPSocket::bind(int port) {
    // config 'addr' to accept all IPv4 connections
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;

    // htons() = Host TO Network Short, which converts the integer from the
    // host’s byte order to network byte order (big endian); sets 'addr' port
    addr.sin_port = htons(port);

    // the ::bind() system call associates the socket descriptor (sockfd) with
    // the given address and port. This essentially “claims” that (IP, port)
    // on the system for that process.
    //
    // If it fails, it means:
    // - port is already in use (another process bound it),
    // - socket wasn’t created properly,
    // - lack of permissions for that port (<1024 without root),
    // - SCTP support is missing
    if (::bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[!] failed to bind SCTP socket\n";
        return false;
    }
    return true;
} // bind()

bool SCTPSocket::listen(int backlog) {
    // it tells the kernel: “I’ve bound this socket to a local port. Now 
    // listen for incoming connection requests.” and places the socket in a 
    // passive/listening state.
    //
    // the backlog paramater means the kernel can hold up to 5 pending
    // connection requests before rejecting new ones with an
    // error (ECONNREFUSED).
    if (::listen(sockfd, backlog) < 0) {
        std::cerr << "[!] failed to listen on SCTP socket\n";
        return false;
    }
    return true;
} // listen()

bool SCTPSocket::accept(SCTPSocket &clientSocket) {
    sockaddr_in clientAddr;
    socklen_t len = sizeof(clientAddr);

    // ::accept() system call waits (blocks) until a connection request
    // arrives on the listening socket (sockfd). When a connection request is
    // received, the kernel:
    // - creates a new socket specifically for this client connection
    // - copies the client’s addr info into 'clientAddr'
    // - leaves the og sockfd in the listening state so it can accept more
    //   connections later
    int clientFd = ::accept(sockfd, (sockaddr*)&clientAddr, &len);
    if (clientFd < 0) {
        std::cerr << "[!] failed to accept SCTP connection\n";
        return false;
    }

    // copy the newly accepted connection’s file descriptor and address info 
    // into another SCTPSocket object. This is the one passed by reference
    // (clientSocket).
    //
    //The design allows the func to return the connected socket through an
    //existing object rather than constructing a new one.
    clientSocket.sockfd = clientFd;
    clientSocket.addr = clientAddr;
    return true;
} // accept()

bool SCTPSocket::connect(const std::string &host, int port) {
    // addrinfo is a POSIX structure used for host name resolution
    // (used by getaddrinfo()). 'hints' tells resolver what kind of address
    // to look for. 'res' will hold the results returned by getaddrinfo()
    struct addrinfo hints{}, *res = nullptr;
    std::memset(&hints, 0, sizeof(hints));

    // basically, “I want IPv4 addresses suitable for a stream-based socket.” 
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // SCTP 1-to-1 behaves like stream sock

    // converts human-readable hostname into a usable sockaddr_in struct
    int rc = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (rc != 0 || res == nullptr) {
        std::cerr << "[!] failed to resolve host: " << host << "\n";
        return false;
    }
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)res->ai_addr;

    // sets `addr` struct to the destination host and port, which is converted
    // to network byte order with htons() so it’s consistent across arch
    addr.sin_family = AF_INET;
    addr.sin_addr = ipv4->sin_addr;
    addr.sin_port = htons(port);

    // performs the SCTP association handshake with kernel connect()
    bool success = (::connect(sockfd, (sockaddr*)&addr, sizeof(addr)) == 0);
    if (!success)
        std::cerr << "[!] failed to connect to " << host << ":" << port << "\n";

    freeaddrinfo(res);  // frees memory allocated by getaddrinfo()
    return success;
} // connect()

bool SCTPSocket::send(const std::string &message) {
    // even though SCTP is message-oriented, it’s still possible for a single
    // send call to transmit only part of the data. This loop ensures that the
    // entire message is eventually sent.
    ssize_t totalSent = 0;
    const char *data = message.c_str();
    size_t len = message.size();
    while (totalSent < static_cast<ssize_t>(len)) {

        // actual sctp send call
        int ret = sctp_sendmsg(sockfd, data + totalSent, len - totalSent,
                               nullptr, 0, 0, 0, 0, 0, 0);
        if (ret <= 0) {
            std::perror("sctp_sendmsg");
            return false;
        }
        totalSent += ret;
    }
    return true;
} // send()

bool SCTPSocket::receive(std::string &message) {
    // prevents leftover data from showing up in short reads
    char buffer[1024];
    std::memset(buffer, 0, sizeof(buffer));

    int flags = 0;  // receive message status info

    // will store the address of the sender
    struct sockaddr_in peerAddr;
    socklen_t len = sizeof(peerAddr);

    // receive data from the socket
    int ret = sctp_recvmsg(sockfd, buffer, sizeof(buffer), 
                           (sockaddr*)&peerAddr, &len, nullptr, &flags);
    if (ret < 0) {
        std::cerr << "[!] failed to receive SCTP message\n";
        return false;
    }
    if (ret == 0) {
        std::cerr << "[!] connection closed by peer\n";
        return false;
    }
    message = std::string(buffer, ret);
    return true;
} // receive()

sockaddr_in SCTPSocket::get_peer_addr() const {
    return addr;
} // get_peer_addr()

void SCTPSocket::close() {
    // closees the SCTP socket if open; closes the underlying file descriptor
    // and resets it to -1.
    if (sockfd >= 0) {
        ::close(sockfd);
        sockfd = -1;
    }
} // close()
