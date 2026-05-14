#include "field_mouse/input.h"

#include "field_mouse/app_constants.h"
#include "field_mouse/app_state.h"
#include "field_mouse/app_types.h"
#include "field_mouse/config_window.h"

namespace FieldMouse {
namespace {

MouseButton SourceFromMessage(WPARAM msg, const MSLLHOOKSTRUCT* info) {
  switch (msg) {
  case WM_LBUTTONDOWN:
  case WM_LBUTTONUP:
    return MouseButton::Left;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONUP:
    return MouseButton::Right;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONUP:
    return MouseButton::Middle;
  case WM_XBUTTONDOWN:
  case WM_XBUTTONUP:
    return HIWORD(info->mouseData) == XBUTTON2 ? MouseButton::X2 : MouseButton::X1;
  default:
    return MouseButton::Count;
  }
}

bool IsButtonDownMessage(WPARAM msg) {
  return msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_XBUTTONDOWN;
}

bool IsButtonUpMessage(WPARAM msg) {
  return msg == WM_LBUTTONUP || msg == WM_RBUTTONUP || msg == WM_MBUTTONUP || msg == WM_XBUTTONUP;
}

} // namespace

void HandleInputTesterEvent(const MouseInputEvent& event) {
  if (g_app.configWindow) {
    g_app.configWindow->handleInputTesterEvent(event);
  }
}

void PostInputTesterEvent(const MSLLHOOKSTRUCT* info, UINT message) {
  HWND target = g_app.mainWindow;
  if (!target) {
    return;
  }

  auto* event = new MouseInputEvent{};
  event->message = message;
  event->flags = info->flags;
  event->mouseData = info->mouseData;
  event->x = info->pt.x;
  event->y = info->pt.y;
  event->extraInfo = info->dwExtraInfo;
  event->time = info->time;

  if (!PostMessageW(target, kMsgInputEvent, 0, reinterpret_cast<LPARAM>(event))) {
    delete event;
  }
}

void EmitMappedButton(MouseButton target, bool down) {
  INPUT input = {};
  input.type = INPUT_MOUSE;

  switch (target) {
  case MouseButton::Left:
    input.mi.dwFlags = down ? MOUSEEVENTF_LEFTDOWN : MOUSEEVENTF_LEFTUP;
    break;
  case MouseButton::Right:
    input.mi.dwFlags = down ? MOUSEEVENTF_RIGHTDOWN : MOUSEEVENTF_RIGHTUP;
    break;
  case MouseButton::Middle:
    input.mi.dwFlags = down ? MOUSEEVENTF_MIDDLEDOWN : MOUSEEVENTF_MIDDLEUP;
    break;
  case MouseButton::X1:
    input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    input.mi.mouseData = XBUTTON1;
    break;
  case MouseButton::X2:
    input.mi.dwFlags = down ? MOUSEEVENTF_XDOWN : MOUSEEVENTF_XUP;
    input.mi.mouseData = XBUTTON2;
    break;
  default:
    return;
  }

  input.mi.dwExtraInfo = kInjectedSentinel;

  if (SendInput(1, &input, sizeof(INPUT)) == 0) {
    OutputDebugStringW(L"FieldMouse: SendInput failed\n");
  }
}

LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code < 0) {
    return CallNextHookEx(g_app.mouseHook, code, wParam, lParam);
  }

  auto* info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
  PostInputTesterEvent(info, static_cast<UINT>(wParam));

  if (!g_app.settings.remapEnabled) {
    return CallNextHookEx(g_app.mouseHook, code, wParam, lParam);
  }

  if (info->dwExtraInfo == kInjectedSentinel) {
    return CallNextHookEx(g_app.mouseHook, code, wParam, lParam);
  }

  MouseButton source = SourceFromMessage(wParam, info);
  if (source == MouseButton::Count) {
    return CallNextHookEx(g_app.mouseHook, code, wParam, lParam);
  }

  MouseButton target = g_app.settings.mapping[ToIndex(source)];
  if (target == source) {
    return CallNextHookEx(g_app.mouseHook, code, wParam, lParam);
  }

  if (IsButtonDownMessage(wParam)) {
    PostMessageW(g_app.mainWindow, kMsgEmitButton, static_cast<WPARAM>(ToIndex(target)), 1);
    return 1;
  }
  if (IsButtonUpMessage(wParam)) {
    PostMessageW(g_app.mainWindow, kMsgEmitButton, static_cast<WPARAM>(ToIndex(target)), 0);
    return 1;
  }

  return CallNextHookEx(g_app.mouseHook, code, wParam, lParam);
}

void HookMouse() {
  if (g_app.mouseHook) {
    return;
  }

  g_app.mouseHook = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, g_app.instance, 0);
  if (!g_app.mouseHook) {
    OutputDebugStringW(L"FieldMouse: SetWindowsHookExW failed\n");
  }
}

void UnhookMouse() {
  if (!g_app.mouseHook) {
    return;
  }

  UnhookWindowsHookEx(g_app.mouseHook);
  g_app.mouseHook = nullptr;
}

} // namespace FieldMouse
