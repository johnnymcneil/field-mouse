#include "field_mouse/settings.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "field_mouse/app_constants.h"
#include "field_mouse/app_state.h"
#include "field_mouse/app_types.h"

namespace FieldMouse {
namespace {

std::wstring BuildAutostartCommandLine() {
  wchar_t pathBuffer[MAX_PATH] = {};
  DWORD len = GetModuleFileNameW(nullptr, pathBuffer, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return {};
  }

  std::wstring command = L"\"";
  command += pathBuffer;
  command += L"\" --background";
  return command;
}

bool IsAutostartEnabledInRegistry() {
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKeyPath, 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
    return false;
  }

  DWORD type = 0;
  LONG result = RegQueryValueExW(key, kRunValueName, nullptr, &type, nullptr, nullptr);
  RegCloseKey(key);
  return result == ERROR_SUCCESS && type == REG_SZ;
}

std::string ExtractJsonString(const std::string& json, const std::string& key) {
  std::string token = "\"" + key + "\"";
  auto pos = json.find(token);
  if (pos == std::string::npos) {
    return {};
  }

  pos = json.find(':', pos + token.size());
  if (pos == std::string::npos) {
    return {};
  }

  pos = json.find('"', pos + 1);
  if (pos == std::string::npos) {
    return {};
  }

  auto end = json.find('"', pos + 1);
  if (end == std::string::npos || end <= pos + 1) {
    return {};
  }

  return json.substr(pos + 1, end - pos - 1);
}

std::filesystem::path GetExecutableDirectory() {
  wchar_t pathBuffer[MAX_PATH] = {};
  DWORD len = GetModuleFileNameW(nullptr, pathBuffer, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) {
    return std::filesystem::current_path();
  }
  return std::filesystem::path(pathBuffer).parent_path();
}

std::filesystem::path ResolvePortableSettingsPath() {
  auto exeDir = GetExecutableDirectory();
  auto preferred = exeDir / "field-mouse-settings.json";

  std::ofstream test(preferred, std::ios::app);
  if (test.good()) {
    return preferred;
  }

  auto fallbackDir = std::filesystem::current_path() / "field-mouse-data";
  std::error_code ec;
  std::filesystem::create_directories(fallbackDir, ec);
  return fallbackDir / "settings.json";
}

} // namespace

bool IsStartHiddenCommandLine(PWSTR commandLine) {
  if (!commandLine) {
    return false;
  }

  std::wstring cmd = commandLine;
  return cmd.find(L"--background") != std::wstring::npos
    || cmd.find(L"/background") != std::wstring::npos
    || cmd.find(L"--hidden") != std::wstring::npos
    || cmd.find(L"/hidden") != std::wstring::npos;
}

bool SetAutostartEnabledInRegistry(bool enabled) {
  HKEY key = nullptr;
  LONG openResult = RegCreateKeyExW(
    HKEY_CURRENT_USER,
    kRunKeyPath,
    0,
    nullptr,
    REG_OPTION_NON_VOLATILE,
    KEY_SET_VALUE,
    nullptr,
    &key,
    nullptr
  );
  if (openResult != ERROR_SUCCESS) {
    return false;
  }

  LONG result = ERROR_SUCCESS;
  if (enabled) {
    std::wstring command = BuildAutostartCommandLine();
    if (command.empty()) {
      RegCloseKey(key);
      return false;
    }

    result = RegSetValueExW(
      key,
      kRunValueName,
      0,
      REG_SZ,
      reinterpret_cast<const BYTE*>(command.c_str()),
      static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t))
    );
  } else {
    result = RegDeleteValueW(key, kRunValueName);
    if (result == ERROR_FILE_NOT_FOUND) {
      result = ERROR_SUCCESS;
    }
  }

  RegCloseKey(key);
  return result == ERROR_SUCCESS;
}

void SaveSettings() {
  if (g_app.settingsPath.empty()) {
    return;
  }

  std::ofstream out(g_app.settingsPath, std::ios::trunc);
  if (!out.good()) {
    return;
  }

  out << "{\n";
  out << "  \"schemaVersion\": 1,\n";
  out << "  \"remapEnabled\": " << (g_app.settings.remapEnabled ? "true" : "false") << ",\n";
  out << "  \"startWithWindows\": " << (g_app.settings.startWithWindows ? "true" : "false") << ",\n";
  out << "  \"mapping\": {\n";

  for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
    auto source = static_cast<MouseButton>(i);
    auto target = g_app.settings.mapping[i];
    out << "    \"" << ButtonKeyName(source) << "\": \"" << ButtonKeyName(target) << "\"";
    out << (i + 1 == static_cast<size_t>(MouseButton::Count) ? "\n" : ",\n");
  }

  out << "  }\n";
  out << "}\n";
}

void LoadSettings() {
  g_app.settingsPath = ResolvePortableSettingsPath();
  std::ifstream in(g_app.settingsPath);
  if (!in.good()) {
    SaveSettings();
    return;
  }

  std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (content.find("\"remapEnabled\": false") != std::string::npos) {
    g_app.settings.remapEnabled = false;
  } else if (content.find("\"remapEnabled\": true") != std::string::npos) {
    g_app.settings.remapEnabled = true;
  } else {
    SaveSettings();
  }

  if (content.find("\"startWithWindows\": true") != std::string::npos) {
    g_app.settings.startWithWindows = true;
  } else if (content.find("\"startWithWindows\": false") != std::string::npos) {
    g_app.settings.startWithWindows = false;
  }

  for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i) {
    auto source = static_cast<MouseButton>(i);
    auto key = ExtractJsonString(content, ButtonKeyName(source));
    if (!key.empty()) {
      g_app.settings.mapping[i] = ParseButtonKey(key, source);
    }
  }

  g_app.settings.startWithWindows = IsAutostartEnabledInRegistry();
}

} // namespace FieldMouse
