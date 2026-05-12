#ifndef FIELD_MOUSE_APP_CONSTANTS_H
#define FIELD_MOUSE_APP_CONSTANTS_H

#include <windows.h>

#include "resource.h"

namespace FieldMouse {

inline constexpr wchar_t kMainWindowClass[] = L"FieldMouseHiddenWindow";
inline constexpr wchar_t kConfigWindowClass[] = L"FieldMouseConfigWindow";
inline constexpr wchar_t kAppTitle[] = L"Field Mouse";

inline constexpr UINT kTrayIconId = 1;
inline constexpr UINT kTrayMessage = WM_APP + 1;
inline constexpr UINT kMsgEmitButton = WM_APP + 2;
inline constexpr UINT kMsgInputEvent = WM_APP + 3;

inline constexpr UINT kMenuOpenConfig = 1001;
inline constexpr UINT kMenuToggleRemap = 1002;
inline constexpr UINT kMenuExit = 1003;

inline constexpr int kControlRemapEnabled = 2001;
inline constexpr int kControlStartWithWindows = IDC_START_WITH_WINDOWS;
inline constexpr int kControlFirstCombo = 2100;
inline constexpr int kControlInputTesterLog = IDC_INPUT_TESTER_LOG;
inline constexpr int kControlInputTesterFirstLight = IDC_INPUT_TESTER_FIRST_LIGHT;
inline constexpr UINT kTesterLightClearTimer = 1;
inline constexpr ULONG_PTR kInjectedSentinel = 0xFE1D4000;
inline constexpr wchar_t kRunKeyPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
inline constexpr wchar_t kRunValueName[] = L"FieldMouse";

} // namespace FieldMouse

#endif
