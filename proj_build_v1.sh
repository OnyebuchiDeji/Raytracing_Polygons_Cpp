#!/bin/bash

echo Local Build Tool
echo Usage:
echo 	1. No argument: just call 'build' for a full build/rebuild.
echo 	2. --full: performs full build/rebuild.
echo 	3. --partial: performs rebuild for source file changes.
echo 	4. --clean: perforns clean full build, essentially a full rebuild.
echo

# exit 0 means success!

if [ $# -eq 0 ]; then
	echo Full Build
	cmake -S . -B build -G "Visual Studio 17 2022" -A x64
	cmake --build build
	exit 0
fi

# full-build: for first build --- involves downloading dependencies, etc.
# used when CMakeList itself is modified
if [[ "$1" == "--full" ]]; then 
	echo Full Build
	cmake -S . -B build -G "Visual Studio 17 2022" -A x64
	cmake --build build
	exit 0
fi

# partial-build/rebuild: intended for when CMake source files are changed (not Cmake itself)
if [[ "$1" == "--partial" ]]; then 
	echo Partial Build
	cmake --build build
	exit 0
fi

# clean-build: removes previous build and runs full build anew
# used when a clean build is needed
if [[ "$1" == "--clean" ]]; then
	echo Clean Build
	# conditional remove for clean build.
	if [ -d build ]; then
		rmdir -rf build
	fi
	cmake -S . -B build -G "Visual Studio 17 2022" -A x64
	cmake --build build
	exit 0
fi

# Handle unknown flags gracefully
echo "Error: Unknown argument '$1'"
exit 1