#!/usr/bin/env bash

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CDK_ROOT="${REPO_ROOT}/../CDK"

usage() {
	cat <<'EOF'
Usage: ./build.sh [options]

Options:
  --config Debug|Release
  --platform macOS|Linux
  --arch x86_64|arm64|aarch64
  --help

If an option is omitted, the script asks interactively.
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

require_file() {
	local file_path="$1"
	if [[ ! -f "$file_path" ]]; then
		echo "Required file not found: $file_path" >&2
		exit 1
	fi
}

detect_toolchain() {
	local system_name
	system_name="$(uname -s)"
	local machine_name
	machine_name="$(uname -m)"
	local host_platform_dir
	local host_arch_dir

	case "$system_name" in
		Darwin)
			host_platform_dir="macOS"
			;;
		Linux)
			host_platform_dir="Linux"
			;;
		*)
			echo "Unsupported host OS: $system_name" >&2
			exit 1
			;;
	esac

	case "$machine_name" in
		x86_64|amd64)
			host_arch_dir="x86_64"
			;;
		arm64|aarch64)
			if [[ "$host_platform_dir" == "Linux" ]]; then
				host_arch_dir="aarch64"
			else
				host_arch_dir="arm64"
			fi
			;;
		*)
			echo "Unsupported host architecture: $machine_name" >&2
			exit 1
			;;
	esac

	if [[ -z "$TARGET_PLATFORM" ]]; then
		TARGET_PLATFORM="$host_platform_dir"
	fi
	if [[ -z "$TARGET_ARCH" ]]; then
		if [[ -n "$TARGET_ARCH_INPUT" ]]; then
			TARGET_ARCH="$(normalize_target_arch "$TARGET_PLATFORM" "$TARGET_ARCH_INPUT")" || {
				echo "Invalid --arch value for $TARGET_PLATFORM: $TARGET_ARCH_INPUT" >&2
				exit 1
			}
		else
			TARGET_ARCH="$(normalize_target_arch "$TARGET_PLATFORM" "$machine_name")"
		fi
	fi

	PLATFORM_DIR="$TARGET_PLATFORM"
	ARCH_DIR="$TARGET_ARCH"

	case "$PLATFORM_DIR" in
		macOS)
			TOOLCHAIN_PREFIX="macos"
			;;
		Linux)
			TOOLCHAIN_PREFIX="linux"
			;;
		*)
			echo "Unsupported target platform: $PLATFORM_DIR" >&2
			exit 1
			;;
	esac

	HOST_SYSROOT="${CDK_ROOT}/targets/${host_platform_dir}/${host_arch_dir}"
	CMAKE_BIN="${HOST_SYSROOT}/usr/bin/cmake"
	NINJA_BIN="${HOST_SYSROOT}/usr/bin/ninja"
	TARGET_SYSROOT="${CDK_ROOT}/targets/${PLATFORM_DIR}/${ARCH_DIR}"
	TOOLCHAIN_FILE="${CDK_ROOT}/toolchains/${TOOLCHAIN_PREFIX}.${ARCH_DIR}.toolchain.cmake"

	require_file "$CMAKE_BIN"
	require_file "$NINJA_BIN"
	[[ -d "$TARGET_SYSROOT" ]] || { echo "Required target sysroot not found: $TARGET_SYSROOT" >&2; exit 1; }
	require_file "$TOOLCHAIN_FILE"
}

configure_project() {
	local source_dir="$1"
	local build_dir="$2"
	shift 2
	"$CMAKE_BIN" -S "$source_dir" -B "$build_dir" -G Ninja "$@"
}

copy_app_artifact() {
	local build_dir="$1"
	local artifact_dir="$2"
	local binary_path
	rm -rf "$artifact_dir"
	mkdir -p "$artifact_dir"
	binary_path="$(find "$build_dir" -type f -name 'SailPalServer' | head -n 1)"
	if [[ -z "$binary_path" ]]; then
		echo "Built application binary was not found in $build_dir" >&2
		exit 1
	fi
	cp "$binary_path" "$artifact_dir/"
}

BUILD_TYPE=""
TARGET_PLATFORM=""
TARGET_ARCH_INPUT=""
TARGET_ARCH=""

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

detect_toolchain

if [[ -z "$BUILD_TYPE" ]]; then
	BUILD_TYPE="$(prompt_choice 'Choose build type:' 'Debug' 'Release')"
fi

BUILD_TYPE_DIR="$(lowercase "$BUILD_TYPE")"
BUILD_DIR="${REPO_ROOT}/_out/build/application/${BUILD_TYPE_DIR}-${PLATFORM_DIR}-${ARCH_DIR}"
ARTIFACT_DIR="${REPO_ROOT}/_out/artifacts/application/${BUILD_TYPE_DIR}-${PLATFORM_DIR}-${ARCH_DIR}"

COMMON_CMAKE_ARGS=(
	-DCMAKE_BUILD_TYPE="$BUILD_TYPE"
	-DGUI_ENABLED=OFF
	-DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
	-DCMAKE_MAKE_PROGRAM="$NINJA_BIN"
	-DQt6_DIR="${TARGET_SYSROOT}/usr/lib/cmake/Qt6"
	-DQT_HOST_PATH="${HOST_SYSROOT}/usr"
	-DQT_HOST_PATH_CMAKE_DIR="${HOST_SYSROOT}/usr/lib/cmake"
	-DNEXUS_QT_HOST_PATH="${HOST_SYSROOT}/usr"
	-DNEXUS_QT_HOST_PATH_CMAKE_DIR="${HOST_SYSROOT}/usr/lib/cmake"
)

mkdir -p "$BUILD_DIR"
configure_project "$REPO_ROOT" "$BUILD_DIR" "${COMMON_CMAKE_ARGS[@]}"
"$CMAKE_BIN" --build "$BUILD_DIR" --target SailPalServer
copy_app_artifact "$BUILD_DIR" "$ARTIFACT_DIR"

echo
echo "Application build completed."
echo "Build directory: $BUILD_DIR"
echo "Artifacts: $ARTIFACT_DIR"