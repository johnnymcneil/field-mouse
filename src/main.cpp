#include <QApplication>
#include <windows.h>

#include "field_mouse/app_state.h"
#include "field_mouse/app_windows.h"
#include "field_mouse/input.h"
#include "field_mouse/settings.h"

using namespace FieldMouse;

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  app.setQuitOnLastWindowClosed(false);

  g_app.instance = GetModuleHandleW(nullptr);
  g_app.startHidden = IsStartHiddenCommandLine(GetCommandLineW());

  LoadSettings();

  if (!RegisterWindowClasses()) {
    return 1;
  }

  if (!CreateAppWindows()) {
    return 1;
  }

  HookMouse();
  return RunMessageLoop(app);
}
