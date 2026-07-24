#!/bin/bash

# Define usage instructions
usage() {
	echo "Local Build Tool"
	echo "Usage: $0 [FLAG] [BUILD_TYPE]"
	echo "Flags (choose one):"
	echo "	--full		: Performs a full configuration and build."
	echo "	--partial	: Performs a fast rebuild (skips configuration)."
	echo "	--clean		: Removes previous build directory and runs full build."
	echo "Build Types (choose one):"
	echo "	debug		: Configures a Debug build."
	echo "	release		: Configures a Release build."
	echo "Examples:"
	echo "	$0 --full debug"
	echo "	$0 --partial release"
	echo ""
}

# --- 1. Detect OS & Set Generator/Executable Extensions ---
OS_TYPE="Unknown"
CMAKE_GEN=""
EXE_EXT=""

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || -n "$COMSPEC" ]]; then
	OS_TYPE="Windows"
	CMAKE_GEN=-'G "Visual Studio 17 2022" -A x64'
	EXE_EXT=".exe"
elif [[ "$OSTYPE" == "linux-gnu" ]]; then
	OS_TYPE="Linux"
	# Linux typically defaults to Unix Makefiles or Ninja
	CMAKE_GEN='-G "Unix Makefiles"'
	EXE_EXT=""
else
	echo "Error: Unsupported operating system ($OSTYPE)."
	exit 1
fi

echo "Detected OS: $OS_TYPE"

# --- 2. Parse & Validate Arguments ---
FLAG=""
BUILD_TYPE="debug" # Default fallback

# Check if help requested or too many arguments
if [[ "$1" == "-h" || "$1" == "--help" || $# -gt 2 ]]; then
	usage
	exit 0
fi

# Extract options based on argument count
if [$# -eq 0 ]; then
	FLAG="--full"
elif [ $# -eq 1 ]; then
	# If only one argument, check if it's a flag or a build type
	if [[ "$1" == "--full" || "$1" == "--partial" || "$1" == "--clean" ]]; then
		FLAG="$1"
	elif [[ "$1" == "debug" || "$1" == "release" ]]; then
		FLAG="--full" # Assume full build if flag omitted
		BUILD_TYPE="$1"
	else
		echo "Error: Invalid argument '$1'"
		usage
		exit 1
	fi
elif [ $# -eq 2 ]; then
	FLAG="$1"
	BUILD_TYPE="$2"
fi

# Validate flag syntax
if [[ "$FLAG" != "--full" && "$FLAG" != "--partial" && "$FLAG" != "--clean" ]]; then
	echo "Error: Unknown flag '$FLAG'"
	usage
	exit 1
fi

# Validate build type syntax (and convert to proper CMake casing)
if [[ "${BUILD_TYPE,,}" == "release" ]]; then
	CONFIG_TYPE="Release"
elif [[ "${BUILD_TYPE,,}" == "debug" ]]; then
	CONFIG_TYPE="Debug"
else
	echo "Error: Invalid build type '$BUILD_TYPE'. Use 'debug' or 'release'."
	usage
	exit 1
fi

echo "Action: $FLAG"
echo "Configuration: $CONFIG_TYPE"
echo

# --- 3. Execute Build Operations ---

# Handle clean flag
if [[ "$FLAG" == "--clean" ]]; then
	echo "Clean Build: Removing old directory..."
	if [-d "build" ]; then
		rm -rf build
	fi
fi

# Handle full/clean configuration step

if [[ "$FLAG" == "full" || "$FLAG" == "clean" ]]; then
	echo "Configuring CMake..."
	# Evaluate variables properly to pass the generator string correctly
	eval "cmake -S . -B build $CMAKE_GEN -DCMAKE_BUILD_TYPE=$CONFIG_TYPE"
	if [ $? -ne 0 ]; then
		echo "Error: CMake configuration failed."
		exit 1
	fi
fi

# Build compilation step (Applicable to full, clean, and partial)
echo "Compiling project..."
cmake --build build --config "$CONFIG_TYPE"
if [ $? -ne 0 ]; then
	echo "Error: Build compilation failed."
	exit 1
fi

echo "Build completed successfully!"
echo

# --- 4. Locate and Run the Execuatable ---
echo "Locating and running application..."

# Note: Adjust 'my_project' to match your actual executable target name
# Extract last part of current working directory
CURRENT_DIR = "$PWD" 
TARGET_NAME = "${CURRENT_DIR##*/}"

# Multi-configuration generators (like Visual Studio) drop binaries inside subfolders
if [ "$OS_TYPE" == "Windows" ]; then
	EXE_PATH="build/${CONFIG_TYPE}/${TARGET_NAME}${EXE_EXT}"
else
	EXE_PATH="build/${TARGET_NAME}${EXE_EXT}"
fi

if [ -f "$EXE_PATH" ]; then
	echo "Running: $EXE_PATH"
	echo "_________________________________________"
	"$EXE_PATH"
	exit $?
else
	echo "Error: Executable not found at $EXE_PATH"
	echo "Please ensure TARGET_NAME in this script matches your CMakeLists.txt target."
	exit 1
fi