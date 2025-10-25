/****************************************************************************
 * file: sctp_wrapper.hpp
 * author: luke le
 * description:
 *     declares a thin C++ wrapper class around the SCTP socket API to simplify
 *     creation, connection, and communication over SCTP sockets.
 * notes:
 *     this class provides a clean abstraction for creating reliable,
 *     bidirectional, FIFO communication channels between nodes in the
 *     distributed system. it follows the SCTP 1-to-1 model, which behaves
 *     similarly to TCP, but offers SCTP-specific advantages such as
 *     message-based delivery and multi-streaming support.
 ****************************************************************************/
#ifndef SCTP_WRAPPER_HPP
#define SCTP_WRAPPER_HPP

#include <netinet/in.h>
#include <string>

/**
 * @class SCTPSocket
 * @brief a lightweight wrapper for managing SCTP sockets.
 *
 * This class encapsulates the essential operations required to establish and
 * manage SCTP-based communication channels between nodes in a distributed
 * system. It simplifies socket creation, binding, listening, accepting,
 * connecting, sending, and receiving while ensuring proper resource cleanup.
 *
 * The class uses the 1-to-1 SCTP model (`SOCK_STREAM`, `IPPROTO_SCTP`),
 * providing semantics similar to TCP sockets. Each instance of SCTPSocket
 * represents one endpoint of a reliable, bidirectional communication channel.
 *
 * Typical usage:
 * @code
 *   SCTPSocket listener;
 *   listener.create();
 *   listener.bind(5000);
 *   listener.listen();
 *
 *   SCTPSocket peer;
 *   listener.accept(peer);
 *
 *   std::string msg;
 *   peer.receive(msg);
 *   peer.send("ack");
 * @endcode
 */
class SCTPSocket {
public:
    /**
     * @brief construct a new SCTPSocket object.
     *
     * Initializes the socket descriptor to -1 and clears the internal
     * address structure. The actual socket is not created until create()
     * is called.
     */
    SCTPSocket();

    /**
     * @brief destroy the SCTPSocket object.
     *
     * Automatically closes the socket if it is still open, ensuring that
     * system resources are released when the object goes out of scope.
     */
    ~SCTPSocket();

    /**
     * @brief create a new SCTP socket in 1-to-1 mode.
     *
     * Calls socket() with parameters AF_INET, SOCK_STREAM, and IPPROTO_SCTP.
     * The socket is configured for address reuse and default SCTP parameters
     * (via set_defaults()).
     *
     * @return true if socket creation succeeded, false otherwise.
     */
    bool create();

    /**
     * @brief bind the SCTP socket to a local port.
     *
     * Configures the socket address structure with AF_INET and INADDR_ANY,
     * then binds it to the specified port number.
     *
     * @param port local port to bind the socket to.
     * @return true if binding succeeded, false otherwise.
     */
    bool bind(int port);

    /**
     * @brief mark the socket as passive to accept incoming connections.
     *
     * Wraps the standard listen() system call. Used on the server side to
     * wait for incoming connection requests.
     *
     * @param backlog maximum number of pending connections (default: 5).
     * @return true if listen succeeded, false otherwise.
     */
    bool listen(int backlog = 5);

    /**
     * @brief accept an incoming SCTP connection.
     *
     * Waits for a new connection on the bound and listening socket. On
     * success, initializes the provided SCTPSocket instance with the accepted
     * connection's file descriptor and client address.
     *
     * @param clientSocket reference to an SCTPSocket object to hold the
     *        accepted connection.
     * @return true if the accept succeeded, false otherwise.
     */
    bool accept(SCTPSocket &clientSocket);

    /**
     * @brief connect to a remote SCTP endpoint.
     *
     * Resolves the given hostname to an IP address and attempts to connect to
     * the specified remote port.
     *
     * @param host hostname or IP address of the remote peer.
     * @param port remote port number to connect to.
     * @return true if the connection succeeded, false otherwise.
     */
    bool connect(const std::string &host, int port);

    /**
     * @brief send a message over an established SCTP connection.
     *
     * Uses sctp_sendmsg() to transmit the provided message reliably. The
     * function retries sending until all bytes are sent or an error occurs.
     *
     * @param message the data to send.
     * @return true if the message was sent successfully, false otherwise.
     */
    bool send(const std::string &message);

    /**
     * @brief receive a message from the SCTP socket.
     *
     * Blocks until data is available, then uses sctp_recvmsg() to read up to
     * 1024 bytes. The received bytes are stored in the provided string.
     *
     * @param message output string to hold the received message.
     * @return true if data was received successfully, false otherwise.
     */
    bool receive(std::string &message);

    /**
     * @brief get the peer address associated with the socket.
     *
     * @return sockaddr_in structure containing the peer’s IP and port.
     */
    sockaddr_in get_peer_addr() const;

    /**
     * @brief close the SCTP socket if open.
     *
     * Closes the underlying file descriptor and resets it to -1 to prevent
     * accidental reuse.
     */
    void close();

private:
    int sockfd;         // file descriptor for the SCTP socket
    sockaddr_in addr;   // local or peer address associated with this socket

    /**
     * @brief apply default SCTP options for low-latency and reliable
     *        operation.
     *
     * Configures association setup parameters (SCTP_INITMSG) and disables
     * Nagle’s algorithm (SCTP_NODELAY) to reduce latency for small control
     * messages.
     */
    void set_defaults();
}; // SCTPSocket class

#endif // SCTP_WRAPPER_HPP
