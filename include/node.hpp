#ifndef NODE_HPP
#define NODE_HPP

#include <string>

/**
 * @brief represents the identifying information for a single node.
 *
 * Each NodeInfo structure stores the minimal information required to
 * establish a network connection to a node in the distributed system:
 *   - a unique node identifier (ID),
 *   - the hostname or IP address where the node is running,
 *   - and the port number used for communication.
 *
 * @param id   unique integer identifier assigned to the node.
 * @param host hostname or IP address of the node.
 * @param port TCP port number on which the node listens for incoming
 *        messages.
 */
struct NodeInfo {
    int id;
    std::string host;
    int port;
};

#endif
