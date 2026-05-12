#ifndef FIELD_MOUSE_APP_TYPES_H
#define FIELD_MOUSE_APP_TYPES_H

#include <array>
#include <string>
#include <windows.h>

namespace FieldMouse {

enum class MouseButton : int {
  Left = 0,
  Right = 1,
  Middle = 2,
  X1 = 3,
  X2 = 4,
  Count = 5
};

enum class InputTesterLight : size_t {
  Any = 0,
  Move = 1,
  Wheel = 2,
  Left = 3,
  Right = 4,
  Middle = 5,
  X1 = 6,
  X2 = 7,
  Count = 8
};

struct MouseInputEvent {
  UINT message = 0;
  DWORD flags = 0;
  DWORD mouseData = 0;
  LONG x = 0;
  LONG y = 0;
  ULONG_PTR extraInfo = 0;
  DWORD time = 0;
};

struct AppSettings {
  bool remapEnabled = true;
  bool startWithWindows = false;
  std::array<MouseButton, static_cast<size_t>(MouseButton::Count)> mapping = {
    MouseButton::Left,
    MouseButton::Right,
    MouseButton::Middle,
    MouseButton::X1,
    MouseButton::X2
  };
};

inline constexpr const wchar_t* kButtonNames[] = {
  L"Left",
  L"Right",
  L"Middle",
  L"X1 (Mouse4)",
  L"X2 (Mouse5)"
};

inline constexpr const char* kButtonKeys[] = {
  "left",
  "right",
  "middle",
  "x1",
  "x2"
};

inline constexpr const wchar_t* kInputTesterLightNames[] = {
  L"Any",
  L"Move",
  L"Wheel",
  L"Left",
  L"Right",
  L"Middle",
  L"X1",
  L"X2"
};

inline size_t ToIndex(MouseButton button) {
  return static_cast<size_t>(button);
}

inline const wchar_t* ButtonDisplayName(MouseButton button) {
  return kButtonNames[ToIndex(button)];
}

inline const char* ButtonKeyName(MouseButton button) {
  return kButtonKeys[ToIndex(button)];
}

inline MouseButton ParseButtonKey(const std::string& key, MouseButton fallback) {
  for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
    if (key == kButtonKeys[i]) {
      return static_cast<MouseButton>(i);
    }
  }
  return fallback;
}

inline size_t ToIndex(InputTesterLight light) {
  return static_cast<size_t>(light);
}

inline const wchar_t* InputTesterLightName(InputTesterLight light) {
  return kInputTesterLightNames[ToIndex(light)];
}

} // namespace FieldMouse

#endif
