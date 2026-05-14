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

* Qt 6.11.1 MSVC 2022 64-bit kit installed at `C:\Qt\6.11.1\msvc2022_64`
* Visual Studio 2022 or newer with Desktop development with C++
* Windows SDK with the resource and manifest tools
* CMake 3.24+
* Ninja

The local build path is intentionally close to the future GitHub Actions build:
CMake drives an MSVC x64 Ninja build, Qt is discovered from `QT_ROOT_DIR`, and
the output is deployed with `windeployqt`. The compiler and Qt kit must match;
the canonical build uses MSVC, not MinGW.

If Qt is installed elsewhere, pass its root with `-QtRoot`. The default is
`C:\Qt\6.11.1\msvc2022_64`.

Build from PowerShell:

1. Debug: `./scripts/build-windows.ps1 -Configuration Debug`
2. Release: `./scripts/build-windows.ps1 -Configuration Release`
3. Clean configure: add `-Fresh`

The script validates the Qt root, bootstraps an MSVC x64 developer environment,
sets `QT_ROOT_DIR`, prepends the Qt tool paths, configures the matching CMake
preset, and builds.
You can also call CMake directly after setting `QT_ROOT_DIR`:

1. Configure: `cmake --preset windows-msvc-debug` or `cmake --preset windows-msvc-release`
2. Build: `cmake --build --preset Debug` or `cmake --build --preset Release`

Output binaries are generated under `bin/Debug/` and `bin/Release/` at the repository root.

CMake build files and object/intermediate files are generated under `obj/windows-msvc-debug/` and `obj/windows-msvc-release/`.

MinGW builds are not the release-compatible path. They are supported only for
temporary local troubleshooting by passing `-Toolchain MinGW -QtRoot C:\Qt\6.11.1\mingw_64`.

Create a local release archive after a release build:

1. `./scripts/package-windows.ps1 -Configuration Release`

The package script verifies that the executable, Qt runtime files, platform
plugin, and license/compliance files are present before creating
`artifacts/FieldMouse-windows-x64.zip`.

If you plan to redistribute binaries that bundle Qt, use a shared-library Qt
build and deploy with `windeployqt` or an equivalent process. Release bundles
should include the license and notice files added in this repository and must
publish the exact Qt version used together with corresponding-source access for
that version.

## Repository Layout

* `CMakeLists.txt` - primary build definition
* `CMakePresets.json` - MSVC x64 Windows Debug/Release presets (builds in `obj/windows-msvc-*`)
* `src/` - Win32 entrypoint, resources, and app manifest
* `include/field_mouse/` - project headers
* `assets/icons/` - embedded application icon assets

Generated directories such as `.vs/`, `bin/`, `obj/`, `artifacts/`, `build/`, and `out/` are disposable local artifacts and should not be treated as source files.

## License

Field Mouse is licensed under the GNU Lesser General Public License,
version 3 or, at your option, any later version. See `LICENSE`.

This repository also includes Qt-related compliance materials for binary
distribution:

* `THIRD_PARTY_NOTICES.md`
* `LGPL_COMPLIANCE.md`
* `LICENSES/LGPL-3.0.txt`
* `LICENSES/GPL-3.0.txt`

The current build is intended to use Qt 6 as shared libraries. If you ship a
binary release, keep Qt dynamically linked, include the notice files, and make
the exact corresponding Qt source available for the version you distribute.
