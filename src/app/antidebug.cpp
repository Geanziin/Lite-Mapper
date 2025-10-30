#include "antidebug.h"
#include <tlhelp32.h>

static HANDLE g_watchdog = NULL;
static DWORD WINAPI WatchdogThread(LPVOID) {
    for (;;) {
        if (AntiDebug_IsDebuggerPresent()) {
            // If debugging detected, make it noisier/harder to step
            // Option: random sleep jitter
            Sleep(250 + (GetTickCount() % 250));
        }
        Sleep(500);
    }
    return 0;
}

bool AntiDebug_IsDebuggerPresent() {
    // Layer 1: Windows API checks
    if (IsDebuggerPresent()) return true;
    BOOL remote = FALSE;
    if (CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote) return true;

    // Layer 2: PEB BeingDebugged flag via NtGlobalFlag heuristic (best-effort)
    __try {
        #ifdef _M_X64
        PBYTE pPeb = (PBYTE)__readgsqword(0x60);
        #else
        PBYTE pPeb = (PBYTE)__readfsdword(0x30);
        #endif
        if (pPeb && pPeb[2]) return true; // BeingDebugged
    } __except(EXCEPTION_EXECUTE_HANDLER) {}

    return false;
}

void AntiDebug_Start() {
    if (!g_watchdog) {
        g_watchdog = CreateThread(NULL, 0, WatchdogThread, NULL, 0, NULL);
    }
}


