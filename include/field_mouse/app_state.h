#ifndef FIELD_MOUSE_APP_STATE_H
#define FIELD_MOUSE_APP_STATE_H

#include <array>
#include <filesystem>
#include <windows.h>

#include "field_mouse/app_types.h"

namespace FieldMouse {

struct AppState {
  HINSTANCE instance = nullptr;
  HWND mainWindow = nullptr;
  HWND configWindow = nullptr;
  HHOOK mouseHook = nullptr;
  UINT taskbarCreatedMessage = 0;
  bool trayIconAdded = false;
  HWND inputTesterLog = nullptr;
  std::array<HWND, static_cast<size_t>(InputTesterLight::Count)> inputTesterLights = {};
  std::array<bool, static_cast<size_t>(InputTesterLight::Count)> inputTesterLightStates = {};
  HBRUSH inputTesterLightOnBrush = nullptr;
  HBRUSH inputTesterLightOffBrush = nullptr;
  bool startHidden = false;
  std::filesystem::path settingsPath;
  AppSettings settings;
  std::array<HWND, static_cast<size_t>(MouseButton::Count)> mapCombos = {};
};

extern AppState g_app;

} // namespace FieldMouse

#endif
