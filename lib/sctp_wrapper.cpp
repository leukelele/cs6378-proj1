#include "sctp_wrapper.hpp"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/sctp.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

using namespace std;

namespace {

    /**
     * @brief creates an SCTP socket using IPv4; the func wraps the `socket()`
     * system call to create a stream-oriented
     * SCTP socket (AF_INET, SOCK_STREAM, IPPROTO_SCTP)
     *
     * @return file descriptor on success, or -1 on failure
     */
    int create_sctp_socket() {
        int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
        if (fd < 0) perror("socket");
        return fd;
    }  // create_sctp_socket()

    /**
     * @brief enables SO_REUSEADDR on a socket; allows the socket to be
     * rebound to the same address immediately after the program terminates,
     * which is useful during testing or rapid restarts
     *
     * @param fd socket file descriptor
     * @return true on success; false on failure
     */
    bool set_reuse_addr(int fd) {
        int on = 1;
        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
            perror("setsockopt SO_REUSEADDR");
            return false;
        }
        return true;
    } // set_reuse_addr()

    /**
     * @brief binds a socket to a local port on all interfaces; this sets up
     * the socket to receive connections or packets sent to the specified port
     * on any local network interface.
     *
     * @param fd socket file descriptor
     * @param port port number to bind to
     * @return true on success; false on failure
     */
    bool bind_socket(int fd, int port) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);
        if (bind(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("bind");
            return false;
        }
        return true;
    } // bind_socket()

    /**
     * @brief marks a socket as a passive (listening) socket; this preps the
     * socket to accept incoming connection requests.
     *
     * @param fd Socket file descriptor.
     * @param backlog Maximum number of pending connections in the queue.
     * @return true on success; false on failure (with error logged).
     */
    bool listen_socket(int fd, int backlog) {
        if (listen(fd, backlog) < 0) {
            perror("listen");
            return false;
        }
        return true;
    } // listen_socket()

    /**
     * @brief resolves a hostname and port to a network address using
     * getaddrinfo; this helper wraps `getaddrinfo` to create an `addrinfo`
     * linked list suitable for establishing SCTP connections. The caller must
     * free the returned result with `freeaddrinfo`.
     *
     * @param host hostname or IP address of the peer
     * @param port port number to connect to (in host byte order)
     * @return Pointer to addrinfo on success; nullptr on failure
     */
    addrinfo* resolve_peer(const std::string &host, int port) {
        addrinfo hints{}, *res = nullptr;
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_SCTP;
    
        int rc = getaddrinfo(host.c_str(), std::to_string(port).c_str(),
                                &hints, &res);
        if (rc != 0) {
            std::cerr << "getaddrinfo: " << gai_strerror(rc) << "\n";
            return nullptr;
        }
        return res;
    } // resolve_peer()

    /**
     * @brief Creates an SCTP socket from an addrinfo structure; this is
     * similar to `create_sctp_socket`, but uses the protocol family, socket
     * type, and protocol contained in the given `addrinfo`.
     *
     * @param res pointer to a valid addrinfo structure
     * @return file descriptor on success, or -1 on failure
     */
    int create_sctp_socket_from_addr(addrinfo *res) {
        int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (fd < 0) {
            perror("socket");
        }
        return fd;
    } // create_sctp_socket_from_addr()

    /**
     * @brief establishes an SCTP connection to a resolved peer address; this
     * function wraps the `connect` system call using information from
     * a previously resolved `addrinfo`. It blocks until the connection
     * is established or fails.
     *
     * @param fd socket file descriptor
     * @param res pointer to addrinfo of the peer
     * @return true on success; false on failure
     */
    bool connect_socket(int fd, addrinfo *res) {
        if (connect(fd, res->ai_addr, res->ai_addrlen) < 0) {
            perror("connect");
            return false;
        }
        return true;
    } // connect_socket()
} // end anonymous namespace

 /**
  * @brief: creates an SCTP listening socket on the specified port
  *
  * @param port the port number on which the server will listen (1-65535)
  * @param backlog max number of pending connections to queue
  * @return socket file descriptor; -1 on failure
  */
int create_listen_socket(int port, int backlog) {
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number: " << port << "\n";
        return -1;
    }

    int fd = create_sctp_socket();
    if (fd < 0) return -1;

    if (!set_reuse_addr(fd) || !bind_socket(fd, port) || 
            !listen_socket(fd, backlog)) {
        close(fd);
        return -1;
    }

    return fd;
}

/**
 * @brief connects to a remote peer using SCTP (blocking call)
 *
 * @param host the hostname or IP address of the peer to connect to
 * @param port the port number of the peer (1-65535)
 * @return socket file descriptor on success; -1 on failure
 */
int connect_to_peer(const std::string &host, int port) {
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port number: " << port << "\n";
        return -1;
    }

    addrinfo *res = resolve_peer(host, port);
    if (!res) return -1;

    int fd = create_sctp_socket_from_addr(res);
    if (fd < 0) {
        freeaddrinfo(res);
        return -1;
    }

    if (!connect_socket(fd, res)) {
        close(fd);
        freeaddrinfo(res);
        return -1;
    }

    freeaddrinfo(res);
    return fd;
}

/**
 * @brief accepts an incoming SCTP connection. This function wraps the
 * `accept()` system call. It blocks until a peer connects to the listening
 * socket. On success, it fills in the peer's address information.
 *
 * @param listen_fd file descriptor of the listening socket.
 * @param peer_addr reference to a sockaddr_storage structure to hold peer
 *  address.
 * @param peer_len reference to a variable that will receive the size of 
 *  peer_addr.
 * @return new socket file descriptor for the accepted connection, or -1 on
 * failure
 */
int accept_connection(int listen_fd, sockaddr_storage &peer_addr, socklen_t &peer_len) {
    peer_len = sizeof(peer_addr);
    int fd = accept(listen_fd, (sockaddr*)&peer_addr, &peer_len);
    if (fd < 0) perror("accept");
    return fd;
}

/**
 * @brief sends data over an SCTP connection; this function wraps the `send()`
 * system call. It attempts to send the entire buffer in one call.
 *
 * It does not retry on partial sends, so the caller should check the return
 * value and handle short writes if needed.
 *
 * @param fd connected socket file descriptor
 * @param buf pointer to the buffer containing the data to send
 * @param len number of bytes to send
 * @return Number of bytes actually sent, or -1 on failure
 */
ssize_t send_all(int fd, const void *buf, size_t len) {
    ssize_t sent = send(fd, buf, len, 0);
    return sent;
}

/**
 * @brief receives data from an SCTP connection. This function wraps the
 * `recv()` system call. It attempts to read up to `len` bytes into the
 * provided buffer. It may return fewer bytes than requested if less data is
 * available.
 *
 * @param fd connected socket file descriptor
 * @param buf pointer to the buffer where received data will be stored
 * @param len maximum number of bytes to receive
 * @return Number of bytes received, 0 if the connection was closed by the
 * peer, or -1 on failure
 */
ssize_t recv_all(int fd, void *buf, size_t len) {
    ssize_t r = recv(fd, buf, len, 0);
    return r;
}

/**
 * @brief Closes a socket file descriptor safely; checks whether the
 * descriptor is valid (non-negative) before calling `close()`, preventing
 * accidental errors when closing invalid sockets.
 *
 * @param fd Socket file descriptor to close.
 */
void close_socket(int fd) {
    if (fd >= 0) close(fd);
}
