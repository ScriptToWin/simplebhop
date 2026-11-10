#!/usr/bin/env bash
# Compile and run the standalone CS2 bhop using a temporary binary
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$DIR/bhop.cpp"

# Create a secure temporary file for the binary
BIN="$(mktemp)"

# Ensure the temporary binary is deleted when the script exits
trap 'rm -f "$BIN"' EXIT

echo "[*] Compiling bhop.cpp..."
g++ -std=c++17 -O2 -Wall -Wextra -o "$BIN" "$SRC"

echo "[*] Starting bhop (root needed for /dev/uinput and /dev/input)..."
echo "    1. Bind Mouse Wheel Up → +jump in CS2"
echo "    2. Start CS2"
echo "    3. Press Mouse4 (side button) to toggle bhop ON/OFF"
echo "    4. Ctrl+C to quit"
echo

exec sudo "$BIN"
