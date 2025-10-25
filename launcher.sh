#!/bin/bash
set -euo pipefail
IFS=$'\n\t'

CONFIG_FILE="ds/config.txt"
EXECUTABLE="build/proj1"
REMOTE_DIR="$HOME/project"       # adjust if paths differ
LOG_DIR="$REMOTE_DIR/logs"
SSH_USER="lbl190001"

echo "[-] parsing config file (comment-aware): $CONFIG_FILE"

# extract valid, non-comment lines that start with a digit
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

# parse valid lines
mapfile -t VLINES < <( valid_lines )
if ((${#VLINES[@]} == 0)); then
  echo "[!] no valid lines found in $CONFIG_FILE" >&2
  exit 1
fi

GLOBALS="${VLINES[0]}"
NUM_NODES=$(awk '{print $1}' <<< "$GLOBALS")
if ! [[ "$NUM_NODES" =~ ^[0-9]+$ ]]; then
  echo "[!] could not parse node count from globals: '$GLOBALS'" >&2
  exit 1
fi

# verify enough lines
if ((${#VLINES[@]} < 1 + NUM_NODES)); then
  echo "[!] config too short: expected >= $((1+NUM_NODES)) valid lines, got ${#VLINES[@]}" >&2
  exit 1
fi

# parse node entries
declare -a NODE_IDS NODE_HOSTS NODE_PORTS
for ((i=0; i<NUM_NODES; i++)); do
  line="${VLINES[1+i]}"
  nid=$(awk '{print $1}' <<< "$line")
  host_raw=$(awk '{print $2}' <<< "$line")
  port=$(awk '{print $3}' <<< "$line")
  if [[ -z "$nid" || -z "$host_raw" || -z "$port" ]]; then
    echo "[!] invalid node line at index $i: '$line'" >&2
    exit 1
  fi
  NODE_IDS[i]="$nid"
  NODE_HOSTS[i]=$(normalize_host "$host_raw")
  NODE_PORTS[i]="$port"
done

# identify this host and helper functions
local_host_short=$(hostname -s 2>/dev/null || echo "")
local_host_full=$(hostname -f 2>/dev/null || echo "")

is_local_host() {
  local h="$1"
  [[ "$h" == "$local_host_short" || "$h" == "$local_host_full" ]]
}

# create logs directory (shared NFS)
mkdir -p "$LOG_DIR"

launch_local() {
  local node_id="$1"
  echo "  - launching node $node_id on $(hostname -f) [local]"
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
  echo "  - launching node $node_id on $host [remote]"
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

# determine driver vs participant
participates=false
for ((i=0; i<NUM_NODES; i++)); do
  if is_local_host "${NODE_HOSTS[i]}"; then
    participates=true
    break
  fi
done

if ! $participates; then
  echo "[-] this host ($(hostname -f)) is acting as DRIVER ONLY (no local node)."
else
  echo "[-] this host participates as a NODE in the topology."
fi

# launch all nodes
echo "[-] launching nodes..."
for ((i=0; i<NUM_NODES; i++)); do
  nid="${NODE_IDS[i]}"
  host="${NODE_HOSTS[i]}"
  if is_local_host "$host"; then
    launch_local "$nid"
  else
    launch_remote "$host" "$nid"
  fi
done

echo "[+] All nodes launched."
