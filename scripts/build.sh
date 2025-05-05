#!/bin/bash

# build.sh - Universal build script for CMake projects
#
# Usage:
#   ./build.sh [generator]
#
# Arguments:
#   generator - Optional build system to use (default: ninja)
#               Supported values: ninja, makefile, make
#
# Examples:
#   ./build.sh            # Uses Ninja build system (default)
#   ./build.sh ninja      # Uses Ninja build system explicitly
#   ./build.sh makefile   # Uses Unix Makefiles build system

set -e  # Exit immediately if a command exits with a non-zero status

# Default configuration
DEFAULT_GENERATOR="ninja"
BUILD_TYPE="Release"

# Process command line arguments
GENERATOR="${1:-$DEFAULT_GENERATOR}"
GENERATOR=$(echo "$GENERATOR" | tr '[:upper:]' '[:lower:]')  # Convert to lowercase

# Define a single associative array with all properties encoded
# Format: "CMake Generator|build command|build directory"
declare -A GENERATOR_PROPS=(
    ["ninja"]="Ninja|ninja|build-ninja"
    ["makefile"]="Unix Makefiles|make|build-makefile"
    ["make"]="Unix Makefiles|make|build-makefile"
)

# Function to get a specific property
get_prop() {
    local generator="$1"
    local prop_index="$2"
    echo "${GENERATOR_PROPS[$generator]}" | cut -d'|' -f"$prop_index"
}

# Validate the generator
if [[ ! -v GENERATOR_PROPS[$GENERATOR] ]]; then
    echo "Error: Unsupported generator '$GENERATOR'"
    echo "Supported generators: ${!GENERATOR_PROPS[@]}"
    exit 1
fi

# Set variables based on the selected generator
CMAKE_GENERATOR=$(get_prop "$GENERATOR" 1)
BUILD_COMMAND=$(get_prop "$GENERATOR" 2)
BUILD_DIR=$(get_prop "$GENERATOR" 3)

echo "Building with generator: $CMAKE_GENERATOR"
echo "Build directory: $BUILD_DIR"
echo "Build type: $BUILD_TYPE"

# Create build directory if it doesn't exist
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || { echo "Error: Failed to change directory to $BUILD_DIR"; exit 1; }

# Run CMake
echo "Running CMake..."
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -G "$CMAKE_GENERATOR" ..

# Build the project
echo "Building project with $BUILD_COMMAND..."
$BUILD_COMMAND

echo "Build completed successfully!"