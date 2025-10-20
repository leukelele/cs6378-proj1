#!/bin/bash
set -euo pipefail

CONFIG_FILE="ds/config.txt"
EXECUTABLE="build/proj1"
REMOTE_DIR="$HOME/project"       # adjust if your repo path differs
LOG_DIR="$REMOTE_DIR/logs"       # logs are written under project/logs

echo "[*] Parsing config file: $CONFIG_FILE"
NUM_NODES=$(head -n 1 "$CONFIG_FILE" | awk '{print $1}')

# Ensure local logs dir exists (NFS-shared homes usually mirror to others)
mkdir -p "logs"

launch_remote() {
  local host="$1"
  local node_id="$2"

  # -n: redirect stdin from /dev/null
  # -f: ssh backgrounds itself after auth
  # We use bash -lc to expand $HOME and ensure a login shell environment.
  ssh -n -f "lbl190001@$host" "bash -lc '
    cd \"$REMOTE_DIR\" &&
    mkdir -p logs &&
    # Fully detach using setsid + nohup; close all stdio.
    # Note: the final \"&\" ensures the process is in background before ssh exits.
    nohup setsid \"$EXECUTABLE\" $node_id \
      > logs/stdout-$node_id.log \
      2> logs/stderr-$node_id.log < /dev/null &
    disown || true
  '"
}

echo "[*] Launching nodes..."

# Launch node 0 *locally* (no ssh to dc01)
{
  cd "$REMOTE_DIR"
  mkdir -p logs
  nohup setsid "$EXECUTABLE" 0 \
    > logs/stdout-0.log \
    2> logs/stderr-0.log < /dev/null &
} &>/dev/null
echo "  - node 0 on dc01.utdallas.edu [local]"

# Launch nodes 1..N-1 via ssh
for ((i=1; i<=NUM_NODES-1; i++)); do
  line=$(sed -n "$((i+1))p" "$CONFIG_FILE")
  clean_line=$(echo "$line" | cut -d'#' -f1 | xargs)
  NODE_ID=$(echo "$clean_line" | awk '{print $1}')
  HOST=$(echo "$clean_line" | awk '{print $2}')
  echo "  - node $NODE_ID on $HOST"
  launch_remote "$HOST" "$NODE_ID"
done

echo "[✓] All nodes launched."
