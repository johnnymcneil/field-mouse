#include "field_mouse/app_windows.h"

#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QStyle>
#include <QSystemTrayIcon>
#include <windows.h>

#include "field_mouse/app_constants.h"
#include "field_mouse/app_state.h"
#include "field_mouse/config_window.h"
#include "field_mouse/app_types.h"
#include "field_mouse/input.h"
#include "field_mouse/resource.h"
#include "field_mouse/settings.h"

namespace FieldMouse {
namespace {

QString ToggleRemapActionText() {
  return g_app.settings.remapEnabled ? QStringLiteral("Disable Remapping") : QStringLiteral("Enable Remapping");
}

QIcon AppIcon() {
  QIcon icon(QStringLiteral(":/icons/field-mouse.ico"));
  if (icon.isNull()) {
    icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
  }
  return icon;
}

void ShowConfigWindow() {
  if (!g_app.configWindow) {
    return;
  }

  g_app.configWindow->showAndActivate();
}

void DestroyAppWindow() {
  if (g_app.mainWindow) {
    DestroyWindow(g_app.mainWindow);
    g_app.mainWindow = nullptr;
  }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case kMsgEmitButton: {
    auto btn = static_cast<MouseButton>(wParam);
    if (btn < MouseButton::Count) {
      EmitMappedButton(btn, lParam != 0);
    }
    return 0;
  }

  case kMsgInputEvent: {
    auto* event = reinterpret_cast<MouseInputEvent*>(lParam);
    if (event) {
      HandleInputTesterEvent(*event);
      delete event;
    }
    return 0;
  }

  case WM_COMMAND:
    switch (LOWORD(wParam)) {
    case kMenuOpenConfig:
      ShowConfigWindow();
      return 0;
    case kMenuToggleRemap:
      g_app.settings.remapEnabled = !g_app.settings.remapEnabled;
      SaveSettings();
      SyncUiFromSettings();
      return 0;
    case kMenuExit:
      DestroyAppWindow();
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

  case WM_DESTROY:
    SaveSettings();
    UnhookMouse();

    if (g_app.trayIcon) {
      g_app.trayIcon->hide();
      delete g_app.trayIcon;
      g_app.trayIcon = nullptr;
    }
    g_app.toggleRemapAction = nullptr;

    if (g_app.trayMenu) {
      delete g_app.trayMenu;
      g_app.trayMenu = nullptr;
    }

    if (g_app.configWindow) {
      delete g_app.configWindow;
      g_app.configWindow = nullptr;
    }

    QApplication::quit();
    return 0;

  default:
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

} // namespace

void SyncUiFromSettings() {
  if (g_app.configWindow) {
    g_app.configWindow->syncFromSettings();
  }

  if (g_app.toggleRemapAction) {
    g_app.toggleRemapAction->setText(ToggleRemapActionText());
  }
}

bool RegisterWindowClasses() {
  WNDCLASSW mainClass = {};
  mainClass.lpfnWndProc = MainWndProc;
  mainClass.hInstance = g_app.instance;
  mainClass.lpszClassName = kMainWindowClass;

  return RegisterClassW(&mainClass) != 0;
}

bool CreateAppWindows() {
  g_app.mainWindow = CreateWindowExW(
    0,
    kMainWindowClass,
    kAppTitle,
    0,
    0,
    0,
    0,
    0,
    HWND_MESSAGE,
    nullptr,
    g_app.instance,
    nullptr
  );
  if (!g_app.mainWindow) {
    return false;
  }

  const QIcon appIcon = AppIcon();
  QApplication::setWindowIcon(appIcon);

  g_app.configWindow = new ConfigWindow();
  g_app.configWindow->setWindowIcon(appIcon);

  g_app.trayMenu = new QMenu(g_app.configWindow);
  QAction* openAction = g_app.trayMenu->addAction(QStringLiteral("Open Field Mouse"));
  g_app.toggleRemapAction = g_app.trayMenu->addAction(ToggleRemapActionText());
  g_app.trayMenu->addSeparator();
  QAction* exitAction = g_app.trayMenu->addAction(QStringLiteral("Exit"));

  QObject::connect(openAction, &QAction::triggered, []() {
    ShowConfigWindow();
  });
  QObject::connect(g_app.toggleRemapAction, &QAction::triggered, []() {
    g_app.settings.remapEnabled = !g_app.settings.remapEnabled;
    SaveSettings();
    SyncUiFromSettings();
  });
  QObject::connect(exitAction, &QAction::triggered, []() {
    DestroyAppWindow();
  });

  g_app.trayIcon = new QSystemTrayIcon(appIcon, g_app.configWindow);
  g_app.trayIcon->setToolTip(QStringLiteral("Field Mouse"));
  g_app.trayIcon->setContextMenu(g_app.trayMenu);
  QObject::connect(
    g_app.trayIcon,
    &QSystemTrayIcon::activated,
    [](QSystemTrayIcon::ActivationReason reason) {
      if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        ShowConfigWindow();
      }
    }
  );
  HideConfigWindowToTray();

  SyncUiFromSettings();
  if (!g_app.startHidden || !g_app.trayIcon->isVisible()) {
    ShowConfigWindow();
  }
  return true;
}

bool HideConfigWindowToTray() {
  if (!g_app.configWindow || !g_app.trayIcon || !QSystemTrayIcon::isSystemTrayAvailable()) {
    if (g_app.configWindow) {
      g_app.configWindow->showNormal();
      g_app.configWindow->raise();
      g_app.configWindow->activateWindow();
    }
    return false;
  }

  if (!g_app.trayIcon->isVisible()) {
    g_app.trayIcon->show();
  }

  if (!g_app.trayIcon->isVisible()) {
    g_app.configWindow->showNormal();
    g_app.configWindow->raise();
    g_app.configWindow->activateWindow();
    return false;
  }

  g_app.configWindow->hide();
  return true;
}

int RunMessageLoop(QApplication& app) {
  return app.exec();
}

} // namespace FieldMouse
