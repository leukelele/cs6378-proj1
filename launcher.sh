#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

CONFIG_FILE="ds/config.txt"
EXECUTABLE="build/proj1"
REMOTE_DIR="$HOME/project"       # adjust if paths differs
LOG_DIR="$REMOTE_DIR/logs"       # logs are written under project/logs
SSH_USER="lbl190001"

echo "[*] Parsing config file (comment-aware): $CONFIG_FILE"

# keep only valid (non-empty, non-comment) lines that START WITH A DIGIT
valid_lines() {
  sed 's/#.*$//' "$CONFIG_FILE" \
  | sed 's/^[[:space:]]\+//; s/[[:space:]]\+$//' \
  | awk 'NF>0 && $1 ~ /^[0-9]+$/ { print }'
}

# normalize short hostnames (dc02 -> dc02.utdallas.edu)
DOMAIN_SUFFIX=".utdallas.edu"
normalize_host() {
  local h="$1"
  if [[ "$h" != *.* ]]; then
    printf "%s%s" "$h" "$DOMAIN_SUFFIX"
  else
    printf "%s" "$h"
  fi
}

# read valid lines
mapfile -t VLINES < <( valid_lines )
if ((${#VLINES[@]} == 0)); then
  echo "[!] No valid lines found in $CONFIG_FILE" >&2
  exit 1
fi

# globals and n
GLOBALS="${VLINES[0]}"
NUM_NODES=$(awk '{print $1}' <<< "$GLOBALS")
if ! [[ "$NUM_NODES" =~ ^[0-9]+$ ]]; then
  echo "[!] Could not parse node count from globals: '$GLOBALS'" >&2
  exit 1
fi

# verification; needs at least globals + n node lines
if ((${#VLINES[@]} < 1 + NUM_NODES)); then
  echo "[!] Config too short: expected >= $((1+NUM_NODES)) valid lines, got ${#VLINES[@]}" >&2
  exit 1
fi

# parse nodes (next NUM_NODES lines)
declare -a NODE_IDS NODE_HOSTS NODE_PORTS
for ((i=0; i<NUM_NODES; i++)); do
  line="${VLINES[1+i]}"
  nid=$(awk '{print $1}' <<< "$line")
  host_raw=$(awk '{print $2}' <<< "$line")
  port=$(awk '{print $3}' <<< "$line")
  if [[ -z "$nid" || -z "$host_raw" || -z "$port" ]]; then
    echo "[!] Invalid node line at index $i: '$line'" >&2
    exit 1
  fi
  NODE_IDS[i]="$nid"
  NODE_HOSTS[i]=$(normalize_host "$host_raw")
  NODE_PORTS[i]="$port"
done

# determine if a host is this machine (supports both short and FQDN)
local_host_short=$(hostname -s 2>/dev/null || echo "")
local_host_full=$(hostname -f 2>/dev/null || echo "")
is_local_host() {
  local h="$1"
  [[ "$h" == "$local_host_short" || "$h" == "$local_host_full" ]]
}

# ensure local logs dir exists (NFS-shared across dcXX)
mkdir -p "logs"

launch_local() {
  local node_id="$1"
  echo "  - node $node_id on $(hostname -f) [local]"
  (
    cd "$REMOTE_DIR"
    mkdir -p logs
    nohup setsid "$EXECUTABLE" "$node_id" \
      > "logs/stdout-$node_id.log" \
      2> "logs/stderr-$node_id.log" < /dev/null &
  ) &>/dev/null
}

launch_remote() {
  local host="$1"
  local node_id="$2"
  echo "  - node $node_id on $host"
  ssh -n -f "${SSH_USER}@${host}" "bash -lc '
    set -e
    cd \"$REMOTE_DIR\"
    mkdir -p logs
    nohup setsid \"$EXECUTABLE\" $node_id \
      > logs/stdout-$node_id.log \
      2> logs/stderr-$node_id.log < /dev/null &
    disown || true
  '"
}

echo "[*] Launching nodes..."
for ((i=0; i<NUM_NODES; i++)); do
  nid="${NODE_IDS[i]}"
  host="${NODE_HOSTS[i]}"
  if is_local_host "$host"; then
    launch_local "$nid"
  else
    launch_remote "$host" "$nid"
  fi
done

echo "[!] All nodes launched."
