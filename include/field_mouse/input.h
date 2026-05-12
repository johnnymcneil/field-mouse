#ifndef FIELD_MOUSE_INPUT_H
#define FIELD_MOUSE_INPUT_H

#include <windows.h>

#include "field_mouse/app_types.h"

namespace FieldMouse {

void HandleInputTesterEvent(const MouseInputEvent& event);
void PostInputTesterEvent(const MSLLHOOKSTRUCT* info, UINT message);
void EmitMappedButton(MouseButton target, bool down);
LRESULT CALLBACK MouseHookProc(int code, WPARAM wParam, LPARAM lParam);
void HookMouse();
void UnhookMouse();

} // namespace FieldMouse

#endif
