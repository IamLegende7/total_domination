#!/usr/bin/env bash
set -Eeuo pipefail

PYTHON_VERSION="$1"
INSTALL_DIR="$2"

TARGET="x86_64-unknown-linux-gnu"
ARCHIVE_TYPE="install_only_stripped"

REPO_API="https://api.github.com/repos/astral-sh/python-build-standalone/releases/latest"
TMP_DIR="$(mktemp -d)"

cleanup() {
    rm -rf "$TMP_DIR"
}
trap cleanup EXIT

die() {
    echo "Error: $*" >&2
    exit 1
}

for command in curl jq tar sha256sum; do
    command -v "$command" >/dev/null ||
        die "$command is required"
done

echo "Fetching available Python standalone builds..."

RELEASE_JSON="$TMP_DIR/release.json"

curl \
    --fail \
    --silent \
    --show-error \
    --location \
    --retry 3 \
    "$REPO_API" \
    -o "$RELEASE_JSON"

ASSET_NAME="$(
    jq -r \
        --arg version "$PYTHON_VERSION" \
        --arg target "$TARGET" \
        --arg archive_type "$ARCHIVE_TYPE" '
        .assets[].name
        | select(
            startswith("cpython-" + $version + "+")
            and contains("-" + $target + "-" + $archive_type + ".tar.gz")
        )
    ' "$RELEASE_JSON" | head -n 1
)"

[ -n "$ASSET_NAME" ] ||
    die "Could not find a build for Python $PYTHON_VERSION"

DOWNLOAD_URL="$(
    jq -r \
        --arg name "$ASSET_NAME" '
        .assets[]
        | select(.name == $name)
        | .browser_download_url
    ' "$RELEASE_JSON"
)"

EXPECTED_SHA256="$(
    jq -r \
        --arg name "$ASSET_NAME" '
        .assets[]
        | select(.name == $name)
        | .digest
        | sub("^sha256:"; "")
    ' "$RELEASE_JSON"
)"

[ -n "$DOWNLOAD_URL" ] ||
    die "Could not determine download URL"

ARCHIVE="$TMP_DIR/$ASSET_NAME"

echo "Downloading:"
echo "  $ASSET_NAME"

curl \
    --fail \
    --silent \
    --show-error \
    --location \
    --retry 3 \
    "$DOWNLOAD_URL" \
    -o "$ARCHIVE"

if [ -n "$EXPECTED_SHA256" ] && [ "$EXPECTED_SHA256" != "null" ]; then
    echo "$EXPECTED_SHA256  $ARCHIVE" | sha256sum --check --strict
fi

rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

tar \
    --extract \
    --gzip \
    --file "$ARCHIVE" \
    --directory "$INSTALL_DIR" \
    --strip-components=1

PYTHON="$INSTALL_DIR/bin/python3.14"

[ -x "$PYTHON" ] ||
    die "Python executable was not found at $PYTHON"

echo
echo "Installed successfully:"
"$PYTHON" --version

echo
echo "Runtime test:"
"$PYTHON" -c '
import sys
import ssl
import zlib

print(sys.executable)
print("standard library: OK")
'