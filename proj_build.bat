@echo off
setlocal enabledelayedexpansion

:: Define usage instructions
goto :init

:usage
echo Local Build Tool
echo Usage: %~0 [FLAG] [BUILD_TYPE]
echo Flags (choose one):
echo 	--full		: Performs a full configuration and build.
echo	--partial	: Performs a fast rebuild (skips configuration).
echo	--clean		: Removes previous build directory and runs full build.
echo Build Types (choose one):
echo 	debug		: Configures a Debug build.
echo 	release		: Configures a Release build.
echo Examples
echo	%~0 --full debug
echo 	%~0 --partial release
echo.
goto :eof

:init
:: --- 1. Detect OS & Set Generator/Executable Extensions ---
:: Windows batch scripts can technically run on Linux via Wine, so we do a quick validation
if not "%OS%"=="Windows_NT" (
	echo Error: This script must be run natively inside a Windows environment.
	exit /b 1
)

echo Detected OS: Windows
set "CMAKE_GEN=-G "Visual Studio 17 2022" -A x64"
set "EXE_EXT=.exe"

:: --- 2. Count and Parse Arguments Safely ---
set /A argcount=0
for %%x in (%*) do set /A argcount+=1

set "FLAG="
set "BUILD_TYPE=debug"


:: Check for help flags or excess parameters
if /I "%~1"=="-h" goto :show_usage
if /I "%~1"=="--help" goto :show_usage
if %argcount% GTR 2 (
	echo Error: Too many arguments provided.
	goto :show_usage
)

:: Extract options based on argument count
if %argcount%==0 (
	set "FLAG=--full"
)

if %argcount%==1 (
	if /I "%~1"=="--full" set "FLAG=--full"
	if /I "%~1"=="--partial" set "FLAG=--partial"
	if /I "%~1"=="--clean" set "FLAG=--clean"

	if /I "%~1"=="debug" (
		set "FLAG=--full"
		set "BUILD_TYPE=debug"
	)

	if /I "%~1"=="release" (
		set "FLAG=--full"
		set "BUILD_TYPE=release"
	)

	if "!FLAG!"=="" (
		echo Error: Invalid argument "%~1"
		goto :show_usage
	)
)

if %argcount%==2 (
	set "FLAG=%~1"
	set "BUILD_TYPE=%~2"
)

:: Final validation on Flag syntax
if not "!FLAG!"=="--full" if not "!FLAG!"=="--partial" if not "!FLAG!"=="--clean" (
	echo Error: Unknown flag "!FLAG!"
	goto :show_usage
)

::Validate build type syntax and enforce casting for paths
if /I "!BUILD_TYPE!"=="release" (
	set "CONFIG_TYPE=Release"
) else if /I "!BUILD_TYPE!"=="debug" (
	set "CONFIG_TYPE=Debug"
) else (
	echo Error: Invalid build type "!BUILD_TYPE!". Use 'debug' or 'release'.
	goto :show_usage
)

echo Action: !FLAG!
echo Configuration: !CONFIG_TYPE!
echo.


:: --- 3. Execute Build Operations ---

:: Handle clean flag
if /I "!FLAG!"=="--clean" (
	echo Clean Build: Removing old directory...
	if exist build rmdir /s /q build
)

:: Handle full/clean configuration step
if /I "!FLAG!"=="--full" goto :configure
if /I "!FLAG!"=="--clean" goto :configure
goto :compile

:configure
echo Configuring CMake...
cmake -S . -B build %CMAKE_GEN% -DCMAKE_BUILD_TYPE=!CONFIG_TYPE!
if %ERRORLEVEL% NEQ 0 (
	echo Error: CMake configuration failed.
	exit /b 1
)

:compile
echo Compiling project...
cmake --build build --config !CONFIG_TYPE!
if %ERRORLEVEL% NEQ 0 (
	echo Error: Build compilation failed.
	exit /b 1
)


echo Build completed successfully!
echo.


:: --- 4. Locate and Run the Executable ---
echo Locating and running application...

:: Note: Adjust 'my_project' to match the actual executable target name
:: 1. Assign current working directory to a variable
set "CURRENT_DIR=%CD%"

:: 2. Extract the last part using a FOR loop modifier
for %%I in ("%CURRENT_DIR%") do set "LAST_PART=%%~nxI"
set "TARGET_NAME="%LAST_PART%"

:: Visual Studio generator drops binaries inside a subfolder matching the configuration
set "EXE_PATH=build\!CONFIG_TYPE!\!TARGET_NAME!!EXE_EXT!"

if exist "!EXE_PATH!" (
	echo Running: !EXE_PATH!
	echo --------------------------------------
	"!EXE_PATH!"
	exit /b %ERRORLEVEL%
) else (
	echo Error: Executable not found at !EXE_PATH!
	echo Please ensure TARGET_NAME in this script matches your CMakeLists.txt target.
	exit /b 1
)

:show_usage
call :usage
exit /b 1