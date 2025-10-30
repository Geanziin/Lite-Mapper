#include <windows.h>
#include <shellapi.h>
#include <string>
#include "../keyhook.h"
#include "config.h"
#include "protection.h"

static NOTIFYICONDATA nid{};
static const UINT WM_TRAYICON = WM_APP + 1;
static const UINT ID_TRAY_EXIT = 1001;
static const UINT ID_TRAY_OPEN = 1002;
static const UINT WM_SHOW_EXISTING = WM_APP + 2;

// Mutex para instância única
static HANDLE g_singleInstanceMutex = NULL;
static const wchar_t* MUTEX_NAME = L"Global\\KeyMapperSingleInstance";

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE: {
			// Não adicionar tray icon - janela será mostrada diretamente
			return 0;
		}
		case WM_TRAYICON: {
			if (LOWORD(lParam) == WM_RBUTTONUP || LOWORD(lParam) == WM_LBUTTONUP) {
				POINT pt; GetCursorPos(&pt);
				HMENU menu = CreatePopupMenu();
				AppendMenu(menu, MF_STRING, ID_TRAY_OPEN, L"Open");
				AppendMenu(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
				SetForegroundWindow(hwnd);
				int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
				DestroyMenu(menu);
				if (cmd == ID_TRAY_EXIT) {
					PostMessage(hwnd, WM_CLOSE, 0, 0);
				}
				if (cmd == ID_TRAY_OPEN) {
					HINSTANCE h = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
					extern HWND UI_Show(HINSTANCE, HWND);
					UI_Show(h, hwnd);
				}
			}
			return 0;
		}
		case WM_SHOW_EXISTING: {
			// Trazer janela existente para o primeiro plano
			ShowWindow(hwnd, SW_SHOW);
			SetForegroundWindow(hwnd);
			return 0;
		}
		case WM_CLOSE: {
			// Remover tray icon se existir
			if (nid.cbSize > 0) {
				Shell_NotifyIcon(NIM_DELETE, &nid);
			}
			DestroyWindow(hwnd);
			return 0;
		}
		case WM_DESTROY: {
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int) {
	// Inicializar sistema de proteção
	Protection::initialize();
	
	// Verificações de proteção
	if (ANTI_DEBUG_CHECK()) {
		// Se debugger detectado, executar código lixo e sair
		JUNK_CODE();
		ExitProcess(0);
	}
	
	if (ANTI_VM_CHECK()) {
		// Se VM detectada, executar código lixo e sair
		JUNK_CODE();
		ExitProcess(0);
	}
	
	if (ANTI_SANDBOX_CHECK()) {
		// Se sandbox detectado, executar código lixo e sair
		JUNK_CODE();
		ExitProcess(0);
	}
	
	// Verificar integridade do código
	if (!CHECK_INTEGRITY()) {
		// Se integridade comprometida, executar código lixo e sair
		JUNK_CODE();
		ExitProcess(0);
	}

	// Verificar se já existe uma instância rodando
	g_singleInstanceMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// Já existe uma instância, encontrar a janela existente e trazê-la para o primeiro plano
		HWND existingWindow = FindWindowW(L"KeyMapperWnd", NULL);
		if (existingWindow) {
			// Enviar mensagem para a janela existente se mostrar
			PostMessage(existingWindow, WM_SHOW_EXISTING, 0, 0);
		}
		return 0; // Sair sem iniciar nova instância
	}

	// Carregar config e configurar DLL
	AppConfig cfg; LoadConfig(cfg);
	
	std::string hold1 = "w"; // W sempre implícito
	
	// Ofuscar strings sensíveis
	std::string obf_trigger = OBFUSCATE_STR(cfg.trigger_key.c_str());
	std::string obf_hold = OBFUSCATE_STR(cfg.hold_key.c_str());
	std::string obf_jump_phys = OBFUSCATE_STR(cfg.jump_physical_key.c_str());
	std::string obf_jump_virt = OBFUSCATE_STR(cfg.jump_virtual_key.c_str());
	std::string obf_crouch_phys = OBFUSCATE_STR(cfg.crouch_physical_key.c_str());
	std::string obf_crouch_virt = OBFUSCATE_STR(cfg.crouch_virtual_key.c_str());
	
	// Desofuscar para uso
	std::string trigger_key = DEOBFUSCATE_STR(obf_trigger);
	std::string hold_key = DEOBFUSCATE_STR(obf_hold);
	std::string jump_phys_key = DEOBFUSCATE_STR(obf_jump_phys);
	std::string jump_virt_key = DEOBFUSCATE_STR(obf_jump_virt);
	std::string crouch_phys_key = DEOBFUSCATE_STR(obf_crouch_phys);
	std::string crouch_virt_key = DEOBFUSCATE_STR(obf_crouch_virt);
	
	configure_keys(
		jump_phys_key.c_str(),
		jump_virt_key.c_str(),
		crouch_phys_key.c_str(),
		crouch_virt_key.c_str(),
		trigger_key.c_str(),
		hold1.c_str(),
		hold_key.c_str()
	);
	
	// Gerar código lixo para ofuscar
	JUNK_CODE();
	
	start_hook();
	
	// Ativa mapeamento se config estiver com trigger e pelo menos uma tecla do usuário
	if (!trigger_key.empty() && !hold_key.empty()) {
		set_active(1);
	}

	const wchar_t* clsName = L"KeyMapperWnd";
	WNDCLASS wc{}; wc.lpfnWndProc = WndProc; wc.hInstance = hInstance; wc.lpszClassName = clsName;
	RegisterClass(&wc);
	HWND hwnd = CreateWindowEx(0, clsName, L"KeyMapper", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 300, 200, NULL, NULL, hInstance, NULL);
	
	// Manter janela principal oculta (apenas para gerenciar o hook e mensagens)
	ShowWindow(hwnd, SW_HIDE);
	
	// Mostrar janela de configurações diretamente (não usar bandeja)
	extern HWND UI_Show(HINSTANCE, HWND);
	UI_Show(hInstance, hwnd);

	MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { 
		// Verificações periódicas de proteção
		static DWORD lastCheck = 0;
		if (GetTickCount() - lastCheck > 10000) { // A cada 10 segundos
			lastCheck = GetTickCount();
			
			if (ANTI_DEBUG_CHECK() || ANTI_VM_CHECK() || ANTI_SANDBOX_CHECK()) {
				JUNK_CODE();
				ExitProcess(0);
			}
		}
		
		TranslateMessage(&msg); 
		DispatchMessage(&msg); 
	}

	stop_hook();
	
	// Liberar mutex
	if (g_singleInstanceMutex) {
		ReleaseMutex(g_singleInstanceMutex);
		CloseHandle(g_singleInstanceMutex);
	}
	
	// Limpar sistema de proteção
	Protection::cleanup();
	
	return 0;
}
