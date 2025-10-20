#!/bin/bash
# Robust cleanup across dc01..dc07 for proj1 processes.
# - Kills only your user’s processes
# - TERM -> wait -> KILL
# - Handles node 0 locally, others via SSH
# - Uses SSH options as an array to avoid quoting issues

set -u  # keep -u for typos; avoid -e so one failure doesn't abort all
IFS=$'\n\t'

CONFIG_FILE="ds/config.txt"
PROCESS_BASENAME="proj1"
REMOTE_DIR="$HOME/project"

# IMPORTANT: options as an array; NO quotes around the whole string
SSH_OPTS=( -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveInterval=5 -o ServerAliveCountMax=2 -n )

echo "[*] Parsing config file: $CONFIG_FILE"
NUM_NODES=$(head -n 1 "$CONFIG_FILE" | awk '{print $1}')

kill_local() {
  local node_id="$1"
  echo "  - local (node $node_id): SIGTERM..."
  pkill -u "$USER" -f "$PROCESS_BASENAME" >/dev/null 2>&1 || true

  for _ in {1..10}; do
    if ! pgrep -u "$USER" -f "[/]build/${PROCESS_BASENAME}" >/dev/null 2>&1; then
      echo "    ✓ node $node_id: cleaned"
      return 0
    fi
    sleep 0.5
  done

  echo "    ! still running — SIGKILL..."
  pkill -9 -u "$USER" -f "$PROCESS_BASENAME" >/dev/null 2>&1 || true
  if pgrep -u "$USER" -f "[/]build/${PROCESS_BASENAME}" >/dev/null 2>&1; then
    echo "    ✗ node $node_id: processes remain"
  else
    echo "    ✓ node $node_id: cleaned"
  fi
}

kill_remote() {
  local host="$1"
  local node_id="$2"
  echo "  - $host (node $node_id): SIGTERM..."

  # Single SSH invocation that performs TERM->wait->KILL remotely
  ssh "${SSH_OPTS[@]}" "lbl190001@$host" "bash -lc '
    pkill -u lbl190001 -f $PROCESS_BASENAME >/dev/null 2>&1 || true
    for i in {1..10}; do
      if ! pgrep -u lbl190001 -f \"[/]build/$PROCESS_BASENAME\" >/dev/null 2>&1; then
        echo \"    ✓ node $node_id: cleaned\"
        exit 0
      fi
      sleep 0.5
    done
    echo \"    ! still running — SIGKILL...\"
    pkill -9 -u lbl190001 -f $PROCESS_BASENAME >/dev/null 2>&1 || true
    if pgrep -u lbl190001 -f \"[/]build/$PROCESS_BASENAME\" >/dev/null 2>&1; then
      echo \"    ✗ node $node_id: processes remain\"
    else
      echo \"    ✓ node $node_id: cleaned\"
    fi
  '" || echo "    ! $host: SSH failed (skipping)"
}

echo "[*] Cleaning up all nodes..."

# Node 0 handled locally
FIRST_LINE=$(sed -n "2p" "$CONFIG_FILE")
NODE0_ID=$(echo "$FIRST_LINE" | awk '{print $1}')
kill_local "$NODE0_ID"

# Nodes 1..N-1 via SSH
for ((i=2; i<=NUM_NODES; i++)); do
  line=$(sed -n "$((i+1))p" "$CONFIG_FILE")
  clean_line=$(echo "$line" | cut -d'#' -f1 | xargs)
  NODE_ID=$(echo "$clean_line" | awk '{print $1}')
  HOST=$(echo "$clean_line" | awk '{print $2}')
  kill_remote "$HOST" "$NODE_ID"
done

echo "[✓] Cleanup complete."
