cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake -S . -B build -DBUILD_TESTS=OFF
cmake --build build
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
cmake --build build --target tests
ctest --test-dir build



# MAP Protocol Distributed System

This project implements **Part 1** of the MAP protocol for a distributed
system. Each node communicates with its neighbors over reliable FIFO channels
(TCP sockets). Nodes alternate between **active** and **passive** states
according to the MAP rules:

- At least one node starts **active** (node 0 by default).
- Active nodes send a random number of messages between
  `minPerActive` and `maxPerActive`.
- Each message is sent to a uniformly random neighbor.
- Nodes wait at least `minSendDelay` between consecutive messages within the
  same active interval.
- Nodes become passive after finishing an interval, and can become active
  again when receiving a message (until they reach `maxNumber` total messages
  sent).

Logs are generated in real-time for each node. Snapshots (`.out` files) will be
added in later parts.

## Prerequisites

- Linux or macOS with:
  - `g++` supporting C++11 or higher
  - `make`
  - `bash`
- Ensure `launcher.sh` and `cleanup.sh` are executable:
  ```bash
  chmod +x launcher.sh cleanup.sh
  ```

## File Structure
* `node.cpp` — implementation of the MAP protocol logic for each node.
* `Makefile` — build and utility commands (`make`, `make run`, `make clean`).
* `launcher.sh` — spawns all nodes locally in the background.
* `cleanup.sh` — terminates all running node processes.
* `config.txt` — configuration file describing topology and parameters.
* `logs/` — runtime logs for each node (`.log` and `.stdout.log`).
* `outputs/` — reserved for snapshot outputs (future parts).

## How to Use
### Building
In order to compile the program, run:
```bash
make
```

This produces a `node` binary.

### Running
Aftwards, launch the distributed system locally:
```bash
make run
```

This calls `launcher.sh`, which:
- parses `config.txt` to determine the number of nodes
- starts one `node` process per node ID in the background
- directs runtime logs to `logs/<config>-<id>.log`
- directs stdout/stderr of each process to `logs/<config>-<id>.stdout.log`
- reserves `outputs/` for snapshot output files, which have not yet been
  implemented in Part 1).

### Stopping
To clean up all running node processes and remove build artifacts:
```bash
make clean
```

This:
- runs `cleanup.sh` to kill all running node processes.
- deletes the `node` binary.
- removes the `logs/` and `outputs/` directories.


## Example
```bash
# compile
make

# launch nodes with config.txt
make run

# monitor logs
tail -f logs/config-0.log

# stop everything and clean
make clean
```

## Notes
- By default, node 0 starts active, all others start passive.
- All nodes are run on `localhost`, using ports specified in `config.txt`, due
  to the dcXX servers being down.
