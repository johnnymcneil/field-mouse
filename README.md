# Field Mouse

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
* Visual Studio solution and MSBuild project are the primary build configuration
* No logs; Field Mouse is a privacy-focused tool used by professionals in high-security environments
* The software is designed in such a way that it poses no threats to antivirus software

## Resources

`Resources/images/field-mouse-color.png` is the source artwork for the application icon.

The checked-in Visual Studio project embeds `app/assets/icons/field-mouse.ico` into the executable at build time.

![field-mouse-color.png](images/field-mouse.png)

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

## Build (Windows 11)

Prerequisites:

* Visual Studio Insiders 2026 or a compatible Visual Studio installation with Desktop development with C++
* Windows 11 SDK

From Visual Studio:

1. Open `field-mouse.sln`.
2. Select `Debug|x64` or `Release|x64`.
3. Build the solution.
4. Run `field-mouse` from Visual Studio.

Output binaries are generated under `build/Debug/` and `build/Release/` at the repository root.

## Repository Layout

* `field-mouse.sln` - checked-in Visual Studio solution
* `FieldMouse/` - native Visual Studio C++ project directory
* `FieldMouse/src/` - Win32 entrypoint, resources, and app manifest
* `FieldMouse/include/field_mouse/` - project headers
* `FieldMouse/assets/icons/` - embedded application icon assets

Generated directories such as `.vs/`, `x64/`, `Debug/`, `Release/`, `build/`, and `out/` are disposable local artifacts and should not be treated as source files.
