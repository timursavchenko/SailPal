#!/usr/bin/env bash

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

usage() {
	cat <<'EOF'
Usage: ./server.sh [options]

Build options:
  --config Debug|Release
  --platform macOS|Linux
  --arch x86_64|arm64|aarch64

Server options:
  --storage /absolute/path/to/storage     (required)
  --host 127.0.0.1                        (default)
  --port 80                               (default)
  --password <value>
  --tls-certificate /absolute/path/to/cert.pem
  --tls-private-key /absolute/path/to/key.pem

Flow options:
  --build-only      Build server binary only
  --run-only        Run previously built binary only
  --help

If --run-only is not set, the script builds first and then runs the server.
EOF
}

lowercase() {
	printf '%s' "$1" | tr '[:upper:]' '[:lower:]'
}

uppercase() {
	printf '%s' "$1" | tr '[:lower:]' '[:upper:]'
}

normalize_config() {
	case "$(uppercase "$1")" in
		DEBUG)
			printf '%s' 'Debug'
			;;
		RELEASE)
			printf '%s' 'Release'
			;;
		*)
			return 1
			;;
	esac
}

normalize_platform() {
	case "$(uppercase "$1")" in
		DARWIN|MACOS)
			printf '%s' 'macOS'
			;;
		LINUX)
			printf '%s' 'Linux'
			;;
		*)
			return 1
			;;
	esac
}

normalize_target_arch() {
	local platform="$1"
	local arch
	arch="$(lowercase "$2")"

	case "$arch" in
		x86_64|amd64)
			printf '%s' 'x86_64'
			;;
		arm64|aarch64)
			if [[ "$platform" == "Linux" ]]; then
				printf '%s' 'aarch64'
			else
				printf '%s' 'arm64'
			fi
			;;
		*)
			return 1
			;;
	esac
}

prompt_choice() {
	local prompt="$1"
	shift
	local options=("$@")
	local selected=""

	echo >&2
	echo "$prompt" >&2
	select selected in "${options[@]}"; do
		if [[ -n "$selected" ]]; then
			printf '%s' "$selected"
			return 0
		fi
		echo "Invalid choice. Try again." >&2
	done
}

BUILD_TYPE=""
TARGET_PLATFORM=""
TARGET_ARCH_INPUT=""
TARGET_ARCH=""
SERVER_HOST="127.0.0.1"
SERVER_PORT="80"
STORAGE_PATH=""
SERVER_PASSWORD=""
TLS_CERTIFICATE_PATH=""
TLS_PRIVATE_KEY_PATH=""
BUILD_ONLY="NO"
RUN_ONLY="NO"

while [[ $# -gt 0 ]]; do
	case "$1" in
		--config)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --config" >&2; exit 1; }
			BUILD_TYPE="$(normalize_config "$1")" || { echo "Invalid --config value: $1" >&2; exit 1; }
			;;
		--platform)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --platform" >&2; exit 1; }
			TARGET_PLATFORM="$(normalize_platform "$1")" || { echo "Invalid --platform value: $1" >&2; exit 1; }
			;;
		--arch)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --arch" >&2; exit 1; }
			TARGET_ARCH_INPUT="$1"
			;;
		--storage)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --storage" >&2; exit 1; }
			STORAGE_PATH="$1"
			;;
		--host)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --host" >&2; exit 1; }
			SERVER_HOST="$1"
			;;
		--port)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --port" >&2; exit 1; }
			SERVER_PORT="$1"
			;;
		--password)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --password" >&2; exit 1; }
			SERVER_PASSWORD="$1"
			;;
		--tls-certificate)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --tls-certificate" >&2; exit 1; }
			TLS_CERTIFICATE_PATH="$1"
			;;
		--tls-private-key)
			shift
			[[ $# -gt 0 ]] || { echo "Missing value for --tls-private-key" >&2; exit 1; }
			TLS_PRIVATE_KEY_PATH="$1"
			;;
		--build-only)
			BUILD_ONLY="YES"
			;;
		--run-only)
			RUN_ONLY="YES"
			;;
		--help|-h)
			usage
			exit 0
			;;
		*)
			echo "Unknown argument: $1" >&2
			usage >&2
			exit 1
			;;
	esac
	shift
done

if [[ "$BUILD_ONLY" == "YES" && "$RUN_ONLY" == "YES" ]]; then
	echo "--build-only and --run-only cannot be used together." >&2
	exit 1
fi

if [[ -z "$TARGET_PLATFORM" ]]; then
	case "$(uname -s)" in
		Darwin) TARGET_PLATFORM="macOS" ;;
		Linux) TARGET_PLATFORM="Linux" ;;
		*)
			echo "Unsupported host OS: $(uname -s)" >&2
			exit 1
			;;
	esac
fi

if [[ -n "$TARGET_ARCH_INPUT" ]]; then
	TARGET_ARCH="$(normalize_target_arch "$TARGET_PLATFORM" "$TARGET_ARCH_INPUT")" || {
		echo "Invalid --arch value for $TARGET_PLATFORM: $TARGET_ARCH_INPUT" >&2
		exit 1
	}
else
	TARGET_ARCH="$(normalize_target_arch "$TARGET_PLATFORM" "$(uname -m)")"
fi

if [[ -z "$BUILD_TYPE" ]]; then
	BUILD_TYPE="$(prompt_choice 'Choose build type:' 'Debug' 'Release')"
fi

BUILD_TYPE_DIR="$(lowercase "$BUILD_TYPE")"
ARTIFACT_DIR="${REPO_ROOT}/_out/artifacts/application/${BUILD_TYPE_DIR}-${TARGET_PLATFORM}-${TARGET_ARCH}"
SERVER_BINARY="${ARTIFACT_DIR}/SailPalServer"

if [[ "$RUN_ONLY" != "YES" ]]; then
	"${REPO_ROOT}/build.sh" --config "$BUILD_TYPE" --platform "$TARGET_PLATFORM" --arch "$TARGET_ARCH"
fi

if [[ "$BUILD_ONLY" == "YES" ]]; then
	exit 0
fi

if [[ ! -x "$SERVER_BINARY" ]]; then
	echo "Server binary not found: $SERVER_BINARY" >&2
	echo "Run without --run-only or build first using ./build.sh." >&2
	exit 1
fi

if [[ -z "$STORAGE_PATH" ]]; then
	echo "--storage is required for server start." >&2
	exit 1
fi

if [[ -n "$TLS_CERTIFICATE_PATH" && -z "$TLS_PRIVATE_KEY_PATH" ]]; then
	echo "Both --tls-certificate and --tls-private-key must be provided together." >&2
	exit 1
fi
if [[ -z "$TLS_CERTIFICATE_PATH" && -n "$TLS_PRIVATE_KEY_PATH" ]]; then
	echo "Both --tls-certificate and --tls-private-key must be provided together." >&2
	exit 1
fi

RUN_ARGS=(
	--storage "$STORAGE_PATH"
	--host "$SERVER_HOST"
	--port "$SERVER_PORT"
)

if [[ -n "$SERVER_PASSWORD" ]]; then
	RUN_ARGS+=(--password "$SERVER_PASSWORD")
fi
if [[ -n "$TLS_CERTIFICATE_PATH" ]]; then
	RUN_ARGS+=(--tls-certificate "$TLS_CERTIFICATE_PATH")
	RUN_ARGS+=(--tls-private-key "$TLS_PRIVATE_KEY_PATH")
fi

echo "Starting SailPalServer from: $SERVER_BINARY"
exec "$SERVER_BINARY" "${RUN_ARGS[@]}"