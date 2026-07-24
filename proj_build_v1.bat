@echo off

echo Local Build Tool
echo Usage:
echo 	1. No argument: just call 'build' for a full build/rebuild.
echo 	2. --full: performs full build rebuild.
echo 	3. --partial: performs rebuild for source file changes.
echo 	4. --clean: perforns clean full build, essentially a full rebuild.
echo

::  count arguments
Set /A argcount = 0

for %%x in (%*) do Set/A argcount += 1

:: exit code 0 indicates success

:: no arguments> run a full build
if %argcount% == 0 (
	echo Full Build
	cmake -S . -B build -G "Visual Studio 17 2022" -A x64
	cmake --build build 
	exit /b 0
)

:: using /I prevents crashes from blank arguments
:: and makes it work regardless of capitalization
:: of input, i.e, --Full and --FULL work wihtout failure

:: full-build: for first build --- involves downloading dependencies, etc.
:: used when CMakeList itself is modified
if /I "%1" == "--full" (
	echo Full Build
	cmake -S . -B build -G "Visual Studio 17 2022" -A x64
	cmake --build build 
	exit /b 0
)

:: partial-build/rebuild: intended for when CMake source files are changed (not Cmake itself)
if /I "%1" == "--partial" (
	echo Partial Build
	cmake --build build
	exit /b 0
)

:: clean-build: ::oves previous build and runs full build anew
:: used when a clean build is needed
if /I "%1" == "--clean" (
	echo Clean Build
	:: using rmdir seems superior to the blow rm -rf build
	:: conditional remove for clean build.
	if exist build rmdir /s /q build
	cmake -S . -B build -G "Visual Studio 17 2022" -A x64
	cmake --build build
	exit /b 0
)

:: Handle unknown flags gracefully
echo Error: Unknown argument "%1"
exit /b 1