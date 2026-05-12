# Field Mouse

![Field Mouse Icon](assets/icons/field-mouse.ico)

> Win32 application for remapping mouse buttons, e.g. use Mouse4 as MouseMiddle, or use Mouse5 as MouseRight

## Features

* Autostart with Windows
* Starts minimized to the System Tray
* Runs as a background process by default with the ability to launch a GUI
    * Double-click Field Mouse icon in system tray to launch Field Mouse Config GUI
    * Field Mouse GUI is a window that has:
        * a list of available mouse inputs, with a dropdown next to each input name for selecting another input that the physical mouse input should send instead
        * a toggle for turning the remapping functionality on or off without exiting the application
        * a toggle for enabling/disabling "start with windows"
        * a button to exit and close the application

## Architecture

* C++
* Builds exclusively target Windows 11
* CMake is the primary build configuration
* No logs; Field Mouse is a privacy-focused tool used by professionals in high-security environments
* The software is designed in such a way that it poses no threats to antivirus software

## MVP Implementation Status

Current implementation includes:

* Win32 background application entrypoint
* Startup behavior for MVP:
    * launches with the config window visible
    * minimizing the config window hides it to the system tray
    * closing the config window hides it to tray (does not exit)
* System tray icon with:
    * double-click to open a basic config window
    * right-click menu for open, remap on/off, and exit
* Config window controls:
    * remap enable/disable checkbox
    * per-button mapping dropdowns for Left, Right, Middle, X1, X2
* Low-level mouse hook remap engine:
    * remaps all standard mouse buttons based on selected mapping
    * filters injected input events to prevent synthetic recursion loops
* Portable settings persistence using JSON
    * preferred: `field-mouse-settings.json` beside executable
    * fallback: `field-mouse-data/settings.json` in current working directory

Implemented:

* Autostart with Windows
* Application icon resource embedding from `.ico`

## Build (Windows 11, x64 only)

Prerequisites:

* Visual Studio Insiders 2026 or a compatible Visual Studio installation with Desktop development with C++
* Windows 11 SDK
* CMake 3.24+

The repository does not vendor the Windows toolchain. Build dependencies are the compiler toolset, Windows SDK, and CMake installation provided by the machine or CI image. The included presets use the Visual Studio generator so CMake can resolve MSVC and SDK components without relying on `Program Files` paths committed into the repo or an already-initialized Developer PowerShell session.

From PowerShell or the VS Code CMake Tools extension:

1. Configure: `cmake --preset windows-x64-debug` or `cmake --preset windows-x64-release`
2. Build: `cmake --build --preset debug` or `cmake --build --preset release`

Output binaries are generated under `bin/Debug/` and `bin/Release/` at the repository root.

CMake build files and object/intermediate files are generated under `obj/`.

## Repository Layout

* `CMakeLists.txt` - primary build definition
* `CMakePresets.json` - x64 Windows Debug/Release presets (builds in `obj/`)
* `src/` - Win32 entrypoint, resources, and app manifest
* `include/field_mouse/` - project headers
* `assets/icons/` - embedded application icon assets

Generated directories such as `.vs/`, `bin/`, `obj/`, `build/`, and `out/` are disposable local artifacts and should not be treated as source files.
