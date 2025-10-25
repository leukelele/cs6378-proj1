# distributed systems project 1

This project implements a distributed MAP protocol using SCTP sockets and
vector clocks. Each node establishes bidirectional connections with its
neighbors, sends and receives messages, and records snapshots of its vector
clock.

## features
- bidirectional SCTP connections
- vector clock synchronization
- initial and ongoing snapshot recording
- passive/active node behavior
- robust connection setup with retries

## requirements
- C++11 compiler
- CMake >= 3.26
- SCTP development libraries

## build instructions

1. configure the build:
   ```bash
   cmake -S . -B build -DBUILD_TESTS=OFF
   ```

2. build the project:
   ```bash
   cmake --build build
   ```

3. make launcher and cleanup scripts executable:
   ```bash
   chmod +x launcher.sh cleanup.sh
   ```

## Execution

1. launch the distributed system:
   ```bash
   ./launcher.sh
   ```

2. logs and snapshot files will be written to the `logs/` directory.

3. to terminate all running node processes:
   ```bash
   ./cleanup.sh
   ```

## output
- Each node writes its vector clock snapshots to `logs/config-<node_id>.out`
- Standard output and error logs are stored in `logs/stdout-<node_id>.log` and `logs/stderr-<node_id>.log`
