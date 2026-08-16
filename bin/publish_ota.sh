#!/usr/bin/env bash
set -euo pipefail

SERVER_HOST="spartan"
SERVER_PORT="8080"
FIRMWARE_ROOT="firmware"   # relative to $SERVER_HOST's home directory

# Run from the ESP-IDF project directory (where sdkconfig lives).
if [[ ! -f sdkconfig ]]; then
    echo "Error: no sdkconfig here - run this from your project root (e.g. .../start)" >&2
    exit 1
fi

TARGET=$(grep '^CONFIG_IDF_TARGET=' sdkconfig | cut -d'"' -f2)
if [[ -z "$TARGET" ]]; then
    echo "Error: couldn't determine target from sdkconfig" >&2
    exit 1
fi

BIN_PATH=$(find build -maxdepth 1 -name '*.bin' ! -name 'bootloader.bin' ! -name 'partition-table.bin' ! -name 'ota_data_initial.bin')
if [[ -z "$BIN_PATH" ]]; then
    echo "Error: couldn't find app .bin in build/" >&2
    exit 1
fi

GIT_ROOT=$(git rev-parse --show-toplevel 2>/dev/null || true)
if [[ -n "$GIT_ROOT" ]]; then
    PROJECT_NAME=$(basename "$GIT_ROOT")
else
    PROJECT_NAME=$(basename "$BIN_PATH" .bin)
    echo "Warning: not inside a git repo - falling back to '$PROJECT_NAME'" >&2
fi

BIN_NAME=$(basename "$BIN_PATH")
REMOTE_DIR="${FIRMWARE_ROOT}/${PROJECT_NAME}/${TARGET}"

echo "Publishing $BIN_NAME ($TARGET) to ${SERVER_HOST}:${REMOTE_DIR}/"
ssh "$SERVER_HOST" "mkdir -p ~/${REMOTE_DIR}"
scp "$BIN_PATH" "${SERVER_HOST}:~/${REMOTE_DIR}/"

URL="http://${SERVER_HOST}:${SERVER_PORT}/${PROJECT_NAME}/${TARGET}/${BIN_NAME}"
echo "URL: $URL"

if curl -s -I "$URL" >/tmp/ota_check.$$ 2>/dev/null; then
    echo "--- Server response ---"
    cat /tmp/ota_check.$$
    rm -f /tmp/ota_check.$$
else
    echo "(Server not reachable at $URL - is http.server running on $SERVER_HOST?)"
fi