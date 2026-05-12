#ifndef FIELD_MOUSE_SETTINGS_H
#define FIELD_MOUSE_SETTINGS_H

#include <windows.h>

namespace FieldMouse {

bool IsStartHiddenCommandLine(PWSTR commandLine);
bool SetAutostartEnabledInRegistry(bool enabled);
void SaveSettings();
void LoadSettings();

} // namespace FieldMouse

#endif
