#include "field_mouse/app_windows.h"

#include <windows.h>
#include <shellapi.h>
#include <string>

#include "field_mouse/app_constants.h"
#include "field_mouse/app_state.h"
#include "field_mouse/app_types.h"
#include "field_mouse/input.h"
#include "field_mouse/resource.h"
#include "field_mouse/settings.h"

namespace FieldMouse {
namespace {

void ClearInputTesterLights() {
  g_app.inputTesterLightStates.fill(false);
  if (!g_app.configWindow) {
    return;
  }

  for (size_t i = 0; i < g_app.inputTesterLights.size(); ++i) {
    if (g_app.inputTesterLights[i]) {
      InvalidateRect(g_app.inputTesterLights[i], nullptr, TRUE);
    }
  }
}

void ShowConfigWindow() {
  if (!g_app.configWindow) {
    return;
  }

  ShowWindow(g_app.configWindow, SW_SHOWNORMAL);
  SetWindowPos(
    g_app.configWindow,
    HWND_TOPMOST,
    0,
    0,
    0,
    0,
    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
  );
  SetWindowPos(
    g_app.configWindow,
    HWND_NOTOPMOST,
    0,
    0,
    0,
    0,
    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW
  );
  BringWindowToTop(g_app.configWindow);
  SetActiveWindow(g_app.configWindow);
  SetForegroundWindow(g_app.configWindow);
  SetFocus(g_app.configWindow);
}

void UpdateConfigWindowTitle() {
  if (!g_app.configWindow) {
    return;
  }

  std::wstring title = std::wstring(kAppTitle) + (g_app.settings.remapEnabled ? L" - Remap ON" : L" - Remap OFF");
  SetWindowTextW(g_app.configWindow, title.c_str());
}

void RemoveTrayIcon() {
  if (!g_app.mainWindow || !g_app.trayIconAdded) {
    return;
  }

  NOTIFYICONDATAW nid = {};
  nid.cbSize = sizeof(nid);
  nid.hWnd = g_app.mainWindow;
  nid.uID = kTrayIconId;
  if (Shell_NotifyIconW(NIM_DELETE, &nid)) {
    g_app.trayIconAdded = false;
  }
}

void AddTrayIcon() {
  if (!g_app.mainWindow) {
    return;
  }

  if (g_app.trayIconAdded) {
    NOTIFYICONDATAW nid = {};
    nid.cbSize = sizeof(nid);
    nid.hWnd = g_app.mainWindow;
    nid.uID = kTrayIconId;
    nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
    nid.uCallbackMessage = kTrayMessage;
    nid.hIcon = LoadIconW(g_app.instance, MAKEINTRESOURCEW(IDI_APPICON));
    wcscpy_s(nid.szTip, L"Field Mouse");
    Shell_NotifyIconW(NIM_MODIFY, &nid);
    return;
  }

  NOTIFYICONDATAW nid = {};
  nid.cbSize = sizeof(nid);
  nid.hWnd = g_app.mainWindow;
  nid.uID = kTrayIconId;
  nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
  nid.uCallbackMessage = kTrayMessage;
  nid.hIcon = LoadIconW(g_app.instance, MAKEINTRESOURCEW(IDI_APPICON));
  wcscpy_s(nid.szTip, L"Field Mouse");
  if (Shell_NotifyIconW(NIM_ADD, &nid)) {
    g_app.trayIconAdded = true;
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
  }
}

void ShowTrayMenu() {
  HMENU menu = CreatePopupMenu();
  if (!menu) {
    return;
  }

  AppendMenuW(menu, MF_STRING, kMenuOpenConfig, L"Open Field Mouse");
  AppendMenuW(
    menu,
    MF_STRING,
    kMenuToggleRemap,
    g_app.settings.remapEnabled ? L"Disable Remapping" : L"Enable Remapping"
  );
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, MF_STRING, kMenuExit, L"Exit");

  POINT cursorPos = {};
  GetCursorPos(&cursorPos);
  SetForegroundWindow(g_app.mainWindow);
  TrackPopupMenu(menu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, cursorPos.x, cursorPos.y, 0, g_app.mainWindow, nullptr);
  DestroyMenu(menu);
}

void SyncControlsFromSettings() {
  if (!g_app.configWindow) {
    return;
  }

  HWND toggle = GetDlgItem(g_app.configWindow, kControlRemapEnabled);
  if (toggle) {
    SendMessageW(toggle, BM_SETCHECK, g_app.settings.remapEnabled ? BST_CHECKED : BST_UNCHECKED, 0);
  }

  HWND autostartToggle = GetDlgItem(g_app.configWindow, kControlStartWithWindows);
  if (autostartToggle) {
    SendMessageW(autostartToggle, BM_SETCHECK, g_app.settings.startWithWindows ? BST_CHECKED : BST_UNCHECKED, 0);
  }

  for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
    auto combo = g_app.mapCombos[i];
    if (!combo) {
      continue;
    }
    SendMessageW(combo, CB_SETCURSEL, static_cast<WPARAM>(ToIndex(g_app.settings.mapping[i])), 0);
  }

  UpdateConfigWindowTitle();
}

void BuildConfigControls() {
  CreateWindowW(
    L"BUTTON",
    L"Enable remapping",
    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
    20,
    16,
    200,
    24,
    g_app.configWindow,
    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlRemapEnabled)),
    g_app.instance,
    nullptr
  );

  CreateWindowW(
    L"BUTTON",
    L"Start with Windows",
    WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
    240,
    16,
    200,
    24,
    g_app.configWindow,
    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlStartWithWindows)),
    g_app.instance,
    nullptr
  );

  int y = 64;
  for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
    std::wstring label = std::wstring(ButtonDisplayName(static_cast<MouseButton>(i))) + L" ->";
    CreateWindowW(
      L"STATIC",
      label.c_str(),
      WS_CHILD | WS_VISIBLE,
      20,
      y + 4,
      120,
      20,
      g_app.configWindow,
      nullptr,
      g_app.instance,
      nullptr
    );

    int comboId = kControlFirstCombo + static_cast<int>(i);
    HWND combo = CreateWindowW(
      L"COMBOBOX",
      nullptr,
      WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
      150,
      y,
      200,
      140,
      g_app.configWindow,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(comboId)),
      g_app.instance,
      nullptr
    );

    for (size_t j = 0; j < static_cast<size_t>(MouseButton::Count); ++j) {
      SendMessageW(
        combo,
        CB_ADDSTRING,
        0,
        reinterpret_cast<LPARAM>(ButtonDisplayName(static_cast<MouseButton>(j)))
      );
    }
    g_app.mapCombos[i] = combo;
    y += 30;
  }

  y += 10;
  CreateWindowW(
    L"STATIC",
    L"Input Tester",
    WS_CHILD | WS_VISIBLE,
    20,
    y,
    140,
    20,
    g_app.configWindow,
    nullptr,
    g_app.instance,
    nullptr
  );

  y += 26;
  const int lightWidth = 76;
  const int lightHeight = 24;
  const int lightGap = 8;
  for (size_t i = 0; i < static_cast<size_t>(InputTesterLight::Count); ++i) {
    int column = static_cast<int>(i % 4);
    int row = static_cast<int>(i / 4);
    int x = 20 + column * (lightWidth + lightGap);
    int lightY = y + row * (lightHeight + 8);
    HWND light = CreateWindowW(
      L"STATIC",
      InputTesterLightName(static_cast<InputTesterLight>(i)),
      WS_CHILD | WS_VISIBLE | WS_BORDER | SS_CENTER | SS_CENTERIMAGE,
      x,
      lightY,
      lightWidth,
      lightHeight,
      g_app.configWindow,
      reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlInputTesterFirstLight + static_cast<int>(i))),
      g_app.instance,
      nullptr
    );
    g_app.inputTesterLights[i] = light;
  }

  y += 66;
  g_app.inputTesterLog = CreateWindowExW(
    WS_EX_CLIENTEDGE,
    L"EDIT",
    L"Awaiting input...",
    WS_CHILD | WS_VISIBLE | ES_READONLY | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
    20,
    y,
    490,
    70,
    g_app.configWindow,
    reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlInputTesterLog)),
    g_app.instance,
    nullptr
  );
}

LRESULT CALLBACK ConfigWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED) {
      ShowWindow(hwnd, SW_HIDE);
      return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  case WM_TIMER:
    if (wParam == kTesterLightClearTimer) {
      KillTimer(hwnd, kTesterLightClearTimer);
      ClearInputTesterLights();
      return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  case WM_CTLCOLORSTATIC: {
    HWND control = reinterpret_cast<HWND>(lParam);
    for (size_t i = 0; i < g_app.inputTesterLights.size(); ++i) {
      if (control == g_app.inputTesterLights[i]) {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        SetBkMode(hdc, OPAQUE);
        SetTextColor(hdc, RGB(20, 20, 20));
        SetBkColor(hdc, g_app.inputTesterLightStates[i] ? RGB(96, 220, 120) : RGB(80, 80, 80));
        return reinterpret_cast<LRESULT>(g_app.inputTesterLightStates[i] ? g_app.inputTesterLightOnBrush : g_app.inputTesterLightOffBrush);
      }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
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
    if (LOWORD(wParam) == kControlRemapEnabled && HIWORD(wParam) == BN_CLICKED) {
      g_app.settings.remapEnabled = (SendMessageW(GetDlgItem(hwnd, kControlRemapEnabled), BM_GETCHECK, 0, 0) == BST_CHECKED);
      UpdateConfigWindowTitle();
      SaveSettings();
      return 0;
    }

    if (LOWORD(wParam) == kControlStartWithWindows && HIWORD(wParam) == BN_CLICKED) {
      bool enabled = (SendMessageW(GetDlgItem(hwnd, kControlStartWithWindows), BM_GETCHECK, 0, 0) == BST_CHECKED);
      if (!SetAutostartEnabledInRegistry(enabled)) {
        MessageBoxW(hwnd, L"Failed to update startup setting.", L"Field Mouse", MB_OK | MB_ICONERROR);
        SendMessageW(GetDlgItem(hwnd, kControlStartWithWindows), BM_SETCHECK, g_app.settings.startWithWindows ? BST_CHECKED : BST_UNCHECKED, 0);
        return 0;
      }

      g_app.settings.startWithWindows = enabled;
      SaveSettings();
      return 0;
    }

    if (LOWORD(wParam) >= kControlFirstCombo && LOWORD(wParam) < kControlFirstCombo + static_cast<int>(MouseButton::Count)
      && HIWORD(wParam) == CBN_SELCHANGE) {
      int idx = LOWORD(wParam) - kControlFirstCombo;
      auto combo = reinterpret_cast<HWND>(lParam);
      LRESULT selected = SendMessageW(combo, CB_GETCURSEL, 0, 0);
      if (selected >= 0 && selected < static_cast<LRESULT>(MouseButton::Count)) {
        g_app.settings.mapping[static_cast<size_t>(idx)] = static_cast<MouseButton>(selected);
        SaveSettings();
      }
      return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);

  case WM_CLOSE:
    if (g_app.mainWindow) {
      DestroyWindow(g_app.mainWindow);
    } else {
      DestroyWindow(hwnd);
    }
    return 0;
  case WM_DESTROY:
    KillTimer(hwnd, kTesterLightClearTimer);
    g_app.configWindow = nullptr;
    return 0;
  default:
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == g_app.taskbarCreatedMessage) {
    AddTrayIcon();
    return 0;
  }

  switch (msg) {
  case WM_CREATE:
    AddTrayIcon();
    return 0;

  case kMsgEmitButton: {
    auto btn = static_cast<MouseButton>(wParam);
    if (btn < MouseButton::Count) {
      EmitMappedButton(btn, lParam != 0);
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
      SyncControlsFromSettings();
      SaveSettings();
      return 0;
    case kMenuExit:
      DestroyWindow(hwnd);
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

  case kTrayMessage:
    if (LOWORD(lParam) == WM_LBUTTONUP || LOWORD(lParam) == WM_LBUTTONDBLCLK) {
      ShowConfigWindow();
      return 0;
    }
    if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_CONTEXTMENU) {
      ShowTrayMenu();
      return 0;
    }
    return 0;

  case WM_DESTROY:
    SaveSettings();
    RemoveTrayIcon();
    UnhookMouse();
    if (g_app.inputTesterLightOnBrush) {
      DeleteObject(g_app.inputTesterLightOnBrush);
      g_app.inputTesterLightOnBrush = nullptr;
    }
    if (g_app.inputTesterLightOffBrush) {
      DeleteObject(g_app.inputTesterLightOffBrush);
      g_app.inputTesterLightOffBrush = nullptr;
    }
    PostQuitMessage(0);
    return 0;

  default:
    return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

} // namespace

bool RegisterWindowClasses() {
  WNDCLASSW mainClass = {};
  mainClass.lpfnWndProc = MainWndProc;
  mainClass.hInstance = g_app.instance;
  mainClass.lpszClassName = kMainWindowClass;

  if (!RegisterClassW(&mainClass)) {
    return false;
  }

  WNDCLASSW configClass = {};
  configClass.lpfnWndProc = ConfigWndProc;
  configClass.hInstance = g_app.instance;
  configClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  configClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
  configClass.lpszClassName = kConfigWindowClass;

  return RegisterClassW(&configClass) != 0;
}

bool CreateAppWindows() {
  g_app.mainWindow = CreateWindowExW(
    0,
    kMainWindowClass,
    kAppTitle,
    WS_OVERLAPPEDWINDOW,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    300,
    200,
    nullptr,
    nullptr,
    g_app.instance,
    nullptr
  );
  if (!g_app.mainWindow) {
    return false;
  }

  g_app.configWindow = CreateWindowExW(
    0,
    kConfigWindowClass,
    kAppTitle,
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    560,
    470,
    nullptr,
    nullptr,
    g_app.instance,
    nullptr
  );
  if (!g_app.configWindow) {
    return false;
  }

  BuildConfigControls();
  SyncControlsFromSettings();

  ShowWindow(g_app.mainWindow, SW_HIDE);
  ShowWindow(g_app.configWindow, g_app.startHidden ? SW_HIDE : SW_SHOW);
  AddTrayIcon();
  if (!g_app.startHidden) {
    SetForegroundWindow(g_app.configWindow);
  }
  return true;
}

int RunMessageLoop() {
  MSG msg = {};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}

} // namespace FieldMouse
