#ifndef FIELD_MOUSE_APP_WINDOWS_H
#define FIELD_MOUSE_APP_WINDOWS_H

class QApplication;

namespace FieldMouse {

bool RegisterWindowClasses();
bool CreateAppWindows();
int RunMessageLoop(QApplication& app);
bool HideConfigWindowToTray();
void SyncUiFromSettings();

} // namespace FieldMouse

#endif
