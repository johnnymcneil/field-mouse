#ifndef FIELD_MOUSE_APP_STATE_H
#define FIELD_MOUSE_APP_STATE_H

#include <array>
#include <filesystem>
#include <windows.h>

#include "field_mouse/app_types.h"

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace FieldMouse {

class ConfigWindow;

struct AppState {
  HINSTANCE instance = nullptr;
  HWND mainWindow = nullptr;
  HHOOK mouseHook = nullptr;
  ConfigWindow* configWindow = nullptr;
  QSystemTrayIcon* trayIcon = nullptr;
  QMenu* trayMenu = nullptr;
  QAction* toggleRemapAction = nullptr;
  bool startHidden = false;
  std::filesystem::path settingsPath;
  AppSettings settings;
};

extern AppState g_app;

} // namespace FieldMouse

#endif
