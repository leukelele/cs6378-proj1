Earlier send_all/recv_all called send()/recv() once and returned whatever the
kernel managed to move at that moment. On stream transports (TCP and SCTP with
SOCK_STREAM), that’s unsafe because:
- Partial writes/reads are normal. The kernel may accept/deliver only a prefix
  of your buffer due to congestion window, socket buffer pressure, or
  MTU/segmentation—even on blocking sockets.
- Signals can interrupt I/O. send/recv can fail with EINTR (e.g., SIGCHLD,
  SIGALRM) before any bytes, or after some bytes, are transferred.
- The function name implied “all,” but it could return fewer bytes without the
  caller realizing it—leading to truncated frames, deserialization errors, or
  subtle protocol bugs.

The new implementation loops until every byte is written or a hard error
occurs. If the call is interrupted by a signal, the function retries without
surfacing a spurious error to the caller.


Why this matters specifically for SCTP/TCP stream sockets
- With SOCK_STREAM (TCP or SCTP in stream mode), the transport is a byte
  stream: there are no message boundaries. The kernel is free to deliver your
  application “message” in any chunking it sees fit.
- By looping and maintaining your own boundaries (length prefix), you
  reassemble the exact application message on the other side—ensuring
  FIFO + integrity for each logical message on a single connection.
- Even with SCTP’s message‑oriented flavor (SOCK_SEQPACKET), applications
  often still guard against partial transfers and interruptions for robustness
  and portability. You’re using SOCK_STREAM, so the loop is essential.
