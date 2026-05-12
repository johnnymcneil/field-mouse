#include <windows.h>

#include "field_mouse/app_state.h"
#include "field_mouse/app_windows.h"
#include "field_mouse/input.h"
#include "field_mouse/settings.h"

using namespace FieldMouse;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
  g_app.instance = hInstance;
  g_app.startHidden = IsStartHiddenCommandLine(GetCommandLineW());
  g_app.taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");
  g_app.inputTesterLightOnBrush = CreateSolidBrush(RGB(96, 220, 120));
  g_app.inputTesterLightOffBrush = CreateSolidBrush(RGB(80, 80, 80));

  LoadSettings();

  if (!RegisterWindowClasses()) {
    return 1;
  }

  if (!CreateAppWindows()) {
    return 1;
  }

  HookMouse();
  return RunMessageLoop();
}
