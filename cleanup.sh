#!/bin/bash
# cleanup across dcXX for proj1 processes
# - comment-aware config parsing (matches launcher.sh)
# - TERM -> wait -> KILL
# - auto-detects whether each node runs locally or remotely
# - uses correct variable expansion inside SSH commands
# - final summary verifies across all nodes

set -euo pipefail
IFS=$'\n\t'

CONFIG_FILE="ds/config.txt"
PROCESS_BASENAME="proj1"
EXECUTABLE_RE='[/]build/proj1'     # exact-match friendly for pgrep/pkill -f
REMOTE_DIR="$HOME/project"
SSH_USER="lbl190001"

# SSH options
SSH_OPTS=( -o BatchMode=yes -o ConnectTimeout=5 -o ServerAliveInterval=5 -o ServerAliveCountMax=2 -n )

# parse valid lines from config
valid_lines() {
  sed 's/#.*$//' "$CONFIG_FILE" \
  | sed 's/^[[:space:]]\+//; s/[[:space:]]\+$//' \
  | awk 'NF>0 && $1 ~ /^[0-9]+$/ { print }'
}

# kill local proj1 processes
kill_local() {
  local node_id="$1"
  echo "  - local (node $node_id): SIGTERM..."
  pkill -u "$USER" -f "$EXECUTABLE_RE" >/dev/null 2>&1 || true

  for _ in {1..10}; do
    if ! pgrep -u "$USER" -f "$EXECUTABLE_RE" >/dev/null 2>&1; then
      echo "    ! node $node_id: cleaned"
      return 0
    fi
    sleep 0.5
  done

  echo "    ! still running — SIGKILL..."
  pkill -9 -u "$USER" -f "$EXECUTABLE_RE" >/dev/null 2>&1 || true
  if pgrep -u "$USER" -f "$EXECUTABLE_RE" >/dev/null 2>&1; then
    echo "    x node $node_id: processes remain"
    return 1
  else
    echo "    ! node $node_id: cleaned"
    return 0
  fi
}

# kill remote proj1 processes
kill_remote() {
  local host="$1"
  local node_id="$2"
  echo "  - $host (node $node_id): SIGTERM..."

  # try normal kill, then escalate to -9 if needed
  if ! ssh "${SSH_OPTS[@]}" "${SSH_USER}@${host}" "bash -c \" \
    pkill -f '$EXECUTABLE_RE' >/dev/null 2>&1 || sudo pkill -f '$EXECUTABLE_RE' >/dev/null 2>&1 || true; \
    for i in {1..10}; do \
      if ! pgrep -f '$EXECUTABLE_RE' >/dev/null 2>&1; then \
        echo '    ! node $node_id: cleaned'; exit 0; fi; \
      sleep 0.5; \
    done; \
    echo '    ! still running — SIGKILL...'; \
    pkill -9 -f '$EXECUTABLE_RE' >/dev/null 2>&1 || sudo pkill -9 -f '$EXECUTABLE_RE' >/dev/null 2>&1 || true; \
    if pgrep -f '$EXECUTABLE_RE' >/dev/null 2>&1; then \
      echo '    x node $node_id: processes remain'; exit 2; \
    else \
      echo '    ! node $node_id: cleaned'; exit 0; fi\""
  then
    echo "    ! $host: SSH failed (skipping)"
    return 3
  fi
}

# parse config
echo "[-] parsing config file (comment-aware): $CONFIG_FILE"

mapfile -t VLINES < <( valid_lines )
if ((${#VLINES[@]} == 0)); then
  echo "[!] no valid config lines; aborting cleanup." >&2
  exit 1
fi

GLOBALS="${VLINES[0]}"
NUM_NODES=$(awk '{print $1}' <<< "$GLOBALS")
if ! [[ "$NUM_NODES" =~ ^[0-9]+$ ]]; then
  echo "[!] failed to parse n from globals: '$GLOBALS'" >&2
  exit 1
fi

declare -a NODE_IDS NODE_HOSTS
for ((i=0; i<NUM_NODES; i++)); do
  line="${VLINES[1+i]}"
  NODE_IDS[i]=$(awk '{print $1}' <<< "$line")
  NODE_HOSTS[i]=$(awk '{print $2}' <<< "$line")
done

local_host_short=$(hostname -s 2>/dev/null || echo "")
local_host_full=$(hostname -f 2>/dev/null || echo "")

is_local_host() {
  local h="$1"
  [[ "$h" == "$local_host_short" || "$h" == "$local_host_full" ]]
}

# cleanup all nodes (auto local/remote detection)
echo "[-] cleaning up all nodes..."
for ((i=0; i<NUM_NODES; i++)); do
  nid="${NODE_IDS[i]}"
  host="${NODE_HOSTS[i]}"
  if is_local_host "$host"; then
    kill_local "$nid" || true
  else
    kill_remote "$host" "$nid" || true
  fi
done

# verification
echo "[-] Verifying across all nodes..."
declare -a BAD

# local
if pgrep -u "$USER" -f "$EXECUTABLE_RE" >/dev/null 2>&1; then
  BAD+=("local")
fi

# remote
for ((i=0; i<NUM_NODES; i++)); do
  nid="${NODE_IDS[i]}"
  host="${NODE_HOSTS[i]}"
  if is_local_host "$host"; then continue; fi
  if ssh "${SSH_OPTS[@]}" "${SSH_USER}@${host}" "pgrep -u $SSH_USER -f '$EXECUTABLE_RE' >/dev/null 2>&1"; then
    BAD+=("$host(node $nid)")
  fi
done

if ((${#BAD[@]} == 0)); then
  echo "[+] all proj1 processes halted across all nodes."
  exit 0
else
  echo "[!] some nodes still have proj1 running: ${BAD[*]}"
  exit 2
fi
