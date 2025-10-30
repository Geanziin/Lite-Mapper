#pragma once
#include <windows.h>

// Anti-debugging utilities
// Start a lightweight watchdog that periodically checks for debuggers
void AntiDebug_Start();

// One-shot check (returns true if a debugger is detected)
bool AntiDebug_IsDebuggerPresent();


