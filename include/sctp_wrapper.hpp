#ifndef SCTP_WRAPPER_HPP
#define SCTP_WRAPPER_HPP

#include <string>
#include <sys/socket.h> // for socklen_t
#include <netinet/in.h>

int     create_listen_socket (int port, int backlog = 5);
int     connect_to_peer      (const std::string &host, int port);
int     accept_connection    (int listen_fd, sockaddr_storage &peer_addr,
                                socklen_t &peer_len);
ssize_t send_all             (int fd, const void *buf, size_t len);
ssize_t recv_all             (int fd, void *buf, size_t len);
void    close_socket         (int fd);

#endif // SCTP_WRAPPER_HPP
