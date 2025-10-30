#include "keyhook.h"
#include <windows.h>
#include <psapi.h>
#include <atomic>
#include <string>
#include <cstring>
#include <cstdlib>
#include "app/protection.h"

static HHOOK g_keyboardHook = nullptr;
static HHOOK g_mouseHook = nullptr;
static HANDLE g_thread = nullptr;
static DWORD g_threadId = 0;
static std::atomic<int> g_isActive{0};
static std::atomic<int> g_keysHeld{0};
static std::atomic<int> g_hold2PhysDown{0};
static std::atomic<int> g_pendingHold{0};

// Stored VKs
static int VK_JUMP_PHYS = 0;
static int VK_JUMP_VIRT = 0;
static int VK_CROUCH_PHYS = 0;
static int VK_CROUCH_VIRT = 0;
static int VK_TRIGGER = 0;
static int VK_HOLD1 = 0; // W
static int VK_HOLD2 = 0; // user key

// W temporary disable state (ms)
static std::atomic<int> g_wDisabled{0};
static DWORD g_wDisableStart = 0;
static const DWORD W_DISABLE_MS = 850;

// Mouse hold detection
static std::atomic<int> g_mouseDown{0};
static DWORD g_mouseDownTick = 0;
static std::atomic<int> g_isJumping{0}; // Estado de pulo
static DWORD g_jumpStart = 0; // Início do estado de pulo
static const DWORD JUMP_DURATION_MS = 875; // Duração do estado de pulo
static const DWORD JUMP_MOUSE_HOLD_MS = 500; // Tempo para segurar mouse durante pulo

// Timer for W re-enable check
static UINT_PTR g_wTimer = 0;
static const UINT WM_TIMER_W_REENABLE = WM_APP + 100;

// Timer for mouse hold check
static UINT_PTR g_mouseTimer = 0;
static const UINT WM_TIMER_MOUSE_HOLD = WM_APP + 101;

// Timer for hold2 key check (50ms)
static UINT_PTR g_holdTimer = 0;
static UINT_PTR g_jumpTimer = 0;

static int toLower(int c) { return (c >= 'A' && c <= 'Z') ? (c - 'A' + 'a') : c; }

// Retorna se o delay de pulo ainda está ativo
static bool is_jump_delay_active() {
	return g_isJumping.load() && (GetTickCount() - g_jumpStart) < JUMP_DURATION_MS;
}

// Forward declarations for key press/release helpers used below
static void press_vk(int vk);
static void release_vk(int vk);
static bool is_mouse_clicking();

static void force_enable_hold() {
    // Ativa hold imediatamente, sem verificações
    if (!g_keysHeld.load()) {
        if (VK_HOLD1 && !g_wDisabled.load()) press_vk(VK_HOLD1);
        if (VK_HOLD2) press_vk(VK_HOLD2);
        g_keysHeld.store(1);
        if (!g_holdTimer) { g_holdTimer = SetTimer(NULL, 0, 50, NULL); }
    }
}

static void attempt_enable_hold() {
    // Não ativar hold se o mouse estiver sendo clicado
    if (is_mouse_clicking()) {
        return; // Mantém pendente, será tentado novamente quando mouse for solto
    }
    force_enable_hold();
}

// Retorna se qualquer botão do mouse está clicado/segurando no momento
static bool is_mouse_clicking() {
	SHORT l = GetAsyncKeyState(VK_LBUTTON);
	SHORT r = GetAsyncKeyState(VK_RBUTTON);
	SHORT x1 = GetAsyncKeyState(VK_XBUTTON1);
	SHORT x2 = GetAsyncKeyState(VK_XBUTTON2);
	return ((l | r | x1 | x2) & 0x8000) != 0 || g_mouseDown.load();
}

static int get_vk_code(const char* key) {
	if (!key || !*key) return 0;
	
	// Verificação de proteção
	if (ANTI_DEBUG_CHECK()) {
		JUNK_CODE();
		return 0;
	}
	
	std::string s(key);
	for (auto &ch : s) ch = static_cast<char>(toLower(static_cast<unsigned char>(ch)));
	
	// Debug: verificar se as teclas especiais estão sendo interpretadas
	#ifdef _DEBUG
	OutputDebugStringA(("get_vk_code called with: " + s + "\n").c_str());
	#endif
	
	// Gerar código lixo para ofuscar
	JUNK_CODE();
	
	if (s.size() == 1 && s[0] >= 'a' && s[0] <= 'z') return 'A' + (s[0] - 'a');
	if (s.size() == 1 && s[0] >= '0' && s[0] <= '9') return '0' + (s[0] - '0');
	if (s == "space") return VK_SPACE;
	if (s == "enter" || s == "return") return VK_RETURN;
	if (s == "tab") return VK_TAB;
	// Botões laterais do mouse
	if (s == "mouse4" || s == "xbutton1") return VK_XBUTTON1;
	if (s == "mouse5" || s == "xbutton2") return VK_XBUTTON2;
	if (s == "esc" || s == "escape") return VK_ESCAPE;
	if (s == "backspace" || s == "back") return VK_BACK;
	if (s == "delete" || s == "del") return VK_DELETE;
	if (s == "up") return VK_UP;
	if (s == "down") return VK_DOWN;
	if (s == "left") return VK_LEFT;
	if (s == "right") return VK_RIGHT;
	// Modificadores: usar variantes específicas para melhor compatibilidade
	if (s == "ctrl" || s == "control" || s == "lctrl") return VK_LCONTROL;
	if (s == "rctrl") return VK_RCONTROL;
	if (s == "shift" || s == "lshift") return VK_LSHIFT;
	if (s == "rshift") return VK_RSHIFT;
	if (s == "alt" || s == "menu" || s == "lalt") return VK_LMENU;
	if (s == "ralt") return VK_RMENU;
	if (s == "capslock" || s == "caps") return VK_CAPITAL;
	if (s == "numlock" || s == "num") return VK_NUMLOCK;
	if (s == "scrolllock" || s == "scroll") return VK_SCROLL;
	if (s.size() >= 2 && s[0] == 'f') {
		int n = atoi(s.c_str() + 1);
		if (n >= 1 && n <= 24) return VK_F1 + (n - 1);
	}
	
	#ifdef _DEBUG
	OutputDebugStringA(("get_vk_code returning 0 for: " + s + "\n").c_str());
	#endif
	
	return 0;
}

static void press_vk(int vk) {
	if (!vk) return;
	INPUT in{}; in.type = INPUT_KEYBOARD;
	HKL layout = GetKeyboardLayout(0);
	WORD scan = static_cast<WORD>(MapVirtualKeyExA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC, layout));
	DWORD flags = KEYEVENTF_SCANCODE;
	// Algumas teclas exigem KEYEVENTF_EXTENDEDKEY
	if (vk == VK_RCONTROL || vk == VK_RMENU || vk == VK_INSERT || vk == VK_DELETE ||
		vk == VK_HOME || vk == VK_END || vk == VK_NEXT || vk == VK_PRIOR ||
		vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
		vk == VK_NUMLOCK || vk == VK_CANCEL || vk == VK_SNAPSHOT ||
		vk == VK_DIVIDE || vk == VK_RWIN || vk == VK_LWIN || vk == VK_APPS) {
		flags |= KEYEVENTF_EXTENDEDKEY;
	}
	in.ki.wVk = 0;
	in.ki.wScan = scan;
	in.ki.dwFlags = flags;
	SendInput(1, &in, sizeof(in));
}

static void release_vk(int vk) {
	if (!vk) return;
	INPUT in{}; in.type = INPUT_KEYBOARD;
	HKL layout = GetKeyboardLayout(0);
	WORD scan = static_cast<WORD>(MapVirtualKeyExA(static_cast<UINT>(vk), MAPVK_VK_TO_VSC, layout));
	DWORD flags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	if (vk == VK_RCONTROL || vk == VK_RMENU || vk == VK_INSERT || vk == VK_DELETE ||
		vk == VK_HOME || vk == VK_END || vk == VK_NEXT || vk == VK_PRIOR ||
		vk == VK_LEFT || vk == VK_RIGHT || vk == VK_UP || vk == VK_DOWN ||
		vk == VK_NUMLOCK || vk == VK_CANCEL || vk == VK_SNAPSHOT ||
		vk == VK_DIVIDE || vk == VK_RWIN || vk == VK_LWIN || vk == VK_APPS) {
		flags |= KEYEVENTF_EXTENDEDKEY;
	}
	in.ki.wVk = 0;
	in.ki.wScan = scan;
	in.ki.dwFlags = flags;
	SendInput(1, &in, sizeof(in));
}

static void ensure_hold_release_all() {
	if (g_keysHeld.load()) {
		release_vk(VK_HOLD1);
		release_vk(VK_HOLD2);
		g_keysHeld.store(0);
		g_wDisabled.store(0);
		// Cancel timer if active
		if (g_wTimer) {
			KillTimer(NULL, g_wTimer);
			g_wTimer = 0;
		}
		// Cancel mouse timer if active
		if (g_mouseTimer) {
			KillTimer(NULL, g_mouseTimer);
			g_mouseTimer = 0;
		}
		// Cancel hold2 timer if active
		if (g_holdTimer) {
			KillTimer(NULL, g_holdTimer);
			g_holdTimer = 0;
		}
	}
}

static void maybe_reenable_w() {
	if (g_wDisabled.load()) {
		DWORD now = GetTickCount();
		if (now - g_wDisableStart >= W_DISABLE_MS) {
			if (g_keysHeld.load() && VK_HOLD1) {
				press_vk(VK_HOLD1);
			}
			g_wDisabled.store(0);
			// Cancel timer
			if (g_wTimer) {
				KillTimer(NULL, g_wTimer);
				g_wTimer = 0;
			}
		}
	}
}

static void start_w_timer() {
	if (!g_wTimer) {
		g_wTimer = SetTimer(NULL, 0, 50, NULL); // Check every 50ms
	}
}

static void start_mouse_timer() {
	if (!g_mouseTimer) {
		g_mouseTimer = SetTimer(NULL, 0, 50, NULL); // Check every 50ms
	}
}

static void start_jump_timer() {
    if (!g_jumpTimer) {
        g_jumpTimer = SetTimer(NULL, 0, 20, NULL); // Check frequently until delay termina
    }
}

static void check_mouse_hold() {
	if (g_mouseDown.load() && g_keysHeld.load()) {
		DWORD dur = GetTickCount() - g_mouseDownTick;
		
		// Verifica se ainda está no estado de pulo (850ms)
		bool isCurrentlyJumping = g_isJumping.load() && 
			(GetTickCount() - g_jumpStart) < JUMP_DURATION_MS;
		
		// Se estiver em pulo e segurou por 300ms: para o hold IMEDIATAMENTE
		if (isCurrentlyJumping && dur >= JUMP_MOUSE_HOLD_MS) {
			ensure_hold_release_all();
			g_mouseDown.store(0); // Reseta o estado do mouse
		}
	}
}

static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
		if (nCode == HC_ACTION) {
		KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
		if (!(p->flags & 0x10)) { // not injected
			if (g_isActive.load()) {

				if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
					
					// Trigger: ativa hold (não alterna)
                    if ((int)p->vkCode == VK_TRIGGER) {
                        if (is_jump_delay_active()) {
                            g_pendingHold.store(1);
                            start_jump_timer();
                            return 1;
                        }
                        attempt_enable_hold();
                        return 1; // intercept trigger
                    }
					// Jump físico -> envia virtual e desabilita W temporariamente
					if ((int)p->vkCode == VK_JUMP_PHYS) {
						if (VK_JUMP_VIRT) press_vk(VK_JUMP_VIRT);
                        g_isJumping.store(1); // Marca como em pulo
                        g_jumpStart = GetTickCount(); // Inicia o timer do estado de pulo
                        start_jump_timer();
						if (g_keysHeld.load() && VK_HOLD1 && !g_wDisabled.load()) {
							release_vk(VK_HOLD1);
							g_wDisabled.store(1);
							g_wDisableStart = GetTickCount();
							start_w_timer(); // Start timer to re-enable W
						}
						return 1;
					}
					// Crouch físico -> envia virtual e desativa hold totalmente
					if ((int)p->vkCode == VK_CROUCH_PHYS) {
						if (VK_CROUCH_VIRT) press_vk(VK_CROUCH_VIRT);
						ensure_hold_release_all();
						return 1;
					}
					// WASD/ESC cancela hold instantâneo
					if (g_keysHeld.load()) {
						if ((int)p->vkCode == 'W' || (int)p->vkCode == 'A' || (int)p->vkCode == 'S' || (int)p->vkCode == 'D' || (int)p->vkCode == VK_ESCAPE) {
							ensure_hold_release_all();
						}
					}
				}
				else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
					if ((int)p->vkCode == VK_JUMP_PHYS) {
						if (VK_JUMP_VIRT) release_vk(VK_JUMP_VIRT);
						return 1;
					}
					if ((int)p->vkCode == VK_CROUCH_PHYS) {
						if (VK_CROUCH_VIRT) release_vk(VK_CROUCH_VIRT);
						return 1;
					}
				}
			}
		}
	}
	return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION) {
		// Tratar botões laterais do mouse como gatilhos configuráveis
		if (wParam == WM_XBUTTONDOWN || wParam == WM_XBUTTONUP) {
			MSLLHOOKSTRUCT* ms = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
			WORD xbCode = static_cast<WORD>((ms->mouseData >> 16) & 0xFFFF);
			int vkBtn = (xbCode == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
			bool isDown = (wParam == WM_XBUTTONDOWN);
			if (g_isActive.load()) {
				if (isDown) {
					// Trigger via botão lateral
                    if (vkBtn == VK_TRIGGER) {
                        if (is_jump_delay_active()) {
                            g_pendingHold.store(1);
                            start_jump_timer();
                            return 1;
                        }
                        attempt_enable_hold();
                        return 1;
                    }
					// Pulo físico via botão lateral
					if (vkBtn == VK_JUMP_PHYS) {
						if (VK_JUMP_VIRT) press_vk(VK_JUMP_VIRT);
                        g_isJumping.store(1);
                        g_jumpStart = GetTickCount();
                        start_jump_timer();
						if (g_keysHeld.load() && VK_HOLD1 && !g_wDisabled.load()) {
							release_vk(VK_HOLD1);
							g_wDisabled.store(1);
							g_wDisableStart = GetTickCount();
							start_w_timer();
						}
						return 1;
					}
					// Agachar físico via botão lateral
					if (vkBtn == VK_CROUCH_PHYS) {
						if (VK_CROUCH_VIRT) press_vk(VK_CROUCH_VIRT);
						ensure_hold_release_all();
						return 1;
					}
				} else {
					return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
				}
			} else {
				// XBUTTON UP
				MSLLHOOKSTRUCT* msu = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);
				WORD xbCodeU = static_cast<WORD>((msu->mouseData >> 16) & 0xFFFF);
				int vkBtnU = (xbCodeU == XBUTTON1) ? VK_XBUTTON1 : VK_XBUTTON2;
				if (vkBtnU == VK_JUMP_PHYS) {
					if (VK_JUMP_VIRT) release_vk(VK_JUMP_VIRT);
					return 1;
				}
				if (vkBtnU == VK_CROUCH_PHYS) {
					if (VK_CROUCH_VIRT) release_vk(VK_CROUCH_VIRT);
					return 1;
				}
			}
		}
		if (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN) {
			g_mouseDown.store(1);
			g_mouseDownTick = GetTickCount();
			// Fora do estado de pulo: clique único deve parar o hold imediatamente no DOWN
			if (g_keysHeld.load()) {
				bool isCurrentlyJumping = g_isJumping.load() &&
					(GetTickCount() - g_jumpStart) < JUMP_DURATION_MS;
				if (!isCurrentlyJumping) {
					ensure_hold_release_all();
					g_mouseDown.store(0);
				} else {
					// Em pulo: iniciar timer para verificar os 300ms de segurar
					start_mouse_timer();
				}
			}
		}
		if (wParam == WM_LBUTTONUP || wParam == WM_RBUTTONUP) {
			if (g_mouseDown.load()) {
				DWORD dur = GetTickCount() - g_mouseDownTick;
				
				// Verifica se ainda está no estado de pulo (850ms)
				bool isCurrentlyJumping = g_isJumping.load() && 
					(GetTickCount() - g_jumpStart) < JUMP_DURATION_MS;
				
				// Fora do pulo já foi tratado no DOWN. Em pulo, pare se segurou por 300ms.
				if (g_keysHeld.load()) {
					if (isCurrentlyJumping && dur >= JUMP_MOUSE_HOLD_MS) {
						// Está em pulo e segurou por 300ms: para o hold
						ensure_hold_release_all();
					}
				}
			}
			g_mouseDown.store(0);
			// Para o timer do mouse
			if (g_mouseTimer) {
				KillTimer(NULL, g_mouseTimer);
				g_mouseTimer = 0;
			}
			
			// Se há hold pendente e não há mais mouse clicado, ativar IMEDIATAMENTE
			if (g_pendingHold.load() && !is_mouse_clicking()) {
				force_enable_hold();
				g_pendingHold.store(0);
			}
		}
	}
	return CallNextHookEx(g_mouseHook, nCode, wParam, lParam);
}

static DWORD WINAPI HookThread(LPVOID) {
	HINSTANCE hInstance = GetModuleHandle(NULL);
	g_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, hInstance, 0);
	g_mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInstance, 0);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		// Check for timer messages to re-enable W
		if (msg.message == WM_TIMER && msg.wParam == g_wTimer) {
			maybe_reenable_w();
		}
        // Check for mouse hold timer messages
		if (msg.message == WM_TIMER && msg.wParam == g_mouseTimer) {
			check_mouse_hold();
		}
        // Check for end of jump delay and apply pending hold
        if (msg.message == WM_TIMER && msg.wParam == g_jumpTimer) {
            if (!is_jump_delay_active()) {
                if (g_pendingHold.load()) {
                    // Quando delay de pulo terminar, ativar hold IMEDIATAMENTE
                    force_enable_hold();
                    g_pendingHold.store(0);
                }
                // Limpar timer de jump e estado
                if (g_jumpTimer) { KillTimer(NULL, g_jumpTimer); g_jumpTimer = 0; }
                g_isJumping.store(0);
            }
        }
		// Verifica estado de HOLD2 a cada 50ms
		if (msg.message == WM_TIMER && msg.wParam == g_holdTimer) {
			if (g_keysHeld.load()) {
				SHORT state = VK_HOLD2 ? GetAsyncKeyState(VK_HOLD2) : 0;
				if (VK_HOLD2 && (state & 0x8000) == 0) {
					ensure_hold_release_all();
				}
			} else if (g_holdTimer) {
				KillTimer(NULL, g_holdTimer);
				g_holdTimer = 0;
			}
		}
	}
	if (g_keyboardHook) { UnhookWindowsHookEx(g_keyboardHook); g_keyboardHook = nullptr; }
	if (g_mouseHook) { UnhookWindowsHookEx(g_mouseHook); g_mouseHook = nullptr; }
	// Clean up timers
	if (g_wTimer) {
		KillTimer(NULL, g_wTimer);
		g_wTimer = 0;
	}
	if (g_mouseTimer) {
		KillTimer(NULL, g_mouseTimer);
		g_mouseTimer = 0;
	}
	if (g_holdTimer) {
		KillTimer(NULL, g_holdTimer);
		g_holdTimer = 0;
	}
	if (g_jumpTimer) {
		KillTimer(NULL, g_jumpTimer);
		g_jumpTimer = 0;
	}
	return 0;
}

extern "C" {

void start_hook() {
	// Verificação de proteção
	if (ANTI_DEBUG_CHECK()) {
		JUNK_CODE();
		return;
	}
	
	if (!g_thread) {
		g_thread = CreateThread(NULL, 0, HookThread, NULL, 0, &g_threadId);
	}
}

void stop_hook() {
	if (g_thread) {
		PostThreadMessage(g_threadId, WM_QUIT, 0, 0);
		WaitForSingleObject(g_thread, INFINITE);
		CloseHandle(g_thread);
		g_thread = nullptr;
		g_threadId = 0;
	}
	// Clean up timers
	if (g_wTimer) {
		KillTimer(NULL, g_wTimer);
		g_wTimer = 0;
	}
	if (g_mouseTimer) {
		KillTimer(NULL, g_mouseTimer);
		g_mouseTimer = 0;
	}
	if (g_holdTimer) {
		KillTimer(NULL, g_holdTimer);
		g_holdTimer = 0;
	}
	if (g_jumpTimer) {
		KillTimer(NULL, g_jumpTimer);
		g_jumpTimer = 0;
	}
}

void set_active(int active) {
	// Verificação de proteção
	if (ANTI_DEBUG_CHECK()) {
		JUNK_CODE();
		return;
	}
	
	g_isActive.store(active ? 1 : 0);
}

void set_hold(int hold) {
	// Verificação de proteção
	if (ANTI_DEBUG_CHECK()) {
		JUNK_CODE();
		return;
	}
	
    if (hold) {
        if (is_jump_delay_active()) {
            g_pendingHold.store(1);
            start_jump_timer();
            return;
        }
        attempt_enable_hold();
    } else {
		ensure_hold_release_all();
	}
}

void set_restrict_to_emulators(int restrict_flag) {
	// Função mantida para compatibilidade, mas não faz nada
	// O programa agora funciona globalmente em todas as aplicações
	(void)restrict_flag; // Evitar warning de parâmetro não usado
}

void configure_keys(const char* jump_phys,
					const char* jump_virt,
					const char* crouch_phys,
					const char* crouch_virt,
					const char* trigger,
					const char* hold1,
					const char* hold2) {
	// Verificação de proteção
	if (ANTI_DEBUG_CHECK()) {
		JUNK_CODE();
		return;
	}
	
	// Gerar código lixo para ofuscar
	JUNK_CODE();
	
	VK_JUMP_PHYS = get_vk_code(jump_phys);
	VK_JUMP_VIRT = get_vk_code(jump_virt);
	VK_CROUCH_PHYS = get_vk_code(crouch_phys);
	VK_CROUCH_VIRT = get_vk_code(crouch_virt);
	VK_TRIGGER = get_vk_code(trigger);
	VK_HOLD1 = get_vk_code(hold1);
	VK_HOLD2 = get_vk_code(hold2);
}

}
