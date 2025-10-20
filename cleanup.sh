#!/bin/bash

CONFIG_FILE="ds/config.txt"
PROCESS_NAME="proj1"

echo "[*] Parsing config file: $CONFIG_FILE"

NUM_NODES=$(head -n 1 "$CONFIG_FILE" | awk '{print $1}')

for ((i=1; i<=NUM_NODES; i++)); do
    line=$(sed -n "$((i+1))p" "$CONFIG_FILE")
    clean_line=$(echo "$line" | cut -d'#' -f1 | xargs)
    NODE_ID=$(echo "$clean_line" | awk '{print $1}')
    HOST=$(echo "$clean_line" | awk '{print $2}')
    echo "[*] Cleaning up node $NODE_ID on $HOST..."
    ssh "lbl190001@$HOST" "pkill -f $PROCESS_NAME && echo '[✓] Node $NODE_ID cleaned.' || echo '[i] No process found on node $NODE_ID.'"
done

echo "[✓] Cleanup complete."
