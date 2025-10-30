#include "ui.h"
#include "config.h"
#include <commctrl.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#include <dwmapi.h>

// Conversão UTF-8 <-> UTF-16 para interação com WinAPI Unicode
static std::wstring Utf8ToW(const std::string& s) {
	if (s.empty()) return L"";
	int wlen = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0);
	std::wstring out(wlen, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], wlen);
	return out;
}

static std::string WToUtf8(const std::wstring& s) {
	if (s.empty()) return std::string();
	int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), NULL, 0, NULL, NULL);
	std::string out(len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len, NULL, NULL);
	return out;
}

static void ApplyUIFont(HWND hwnd) {
	static HFONT hFont = NULL;
	if (!hFont) {
		LOGFONTW lf{};
		lf.lfHeight = -16; // ~12pt
		wcscpy_s(lf.lfFaceName, L"Segoe UI");
		hFont = CreateFontIndirectW(&lf);
	}
	SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

// Controles
static HWND g_mainWnd = NULL;

// Edits (Config aba)
static HWND g_editTrigger = NULL;
static HWND g_editHold = NULL;
static HWND g_statusText = NULL;
static HWND g_btnSelTrigger = NULL;
static HWND g_btnSelHold = NULL;

// Edits (Teclas Extras)
static HWND g_editJumpPhys = NULL;
static HWND g_editJumpVirt = NULL;
static HWND g_editCrouchPhys = NULL;
static HWND g_editCrouchVirt = NULL;
static HWND g_editWeaponSwap = NULL;
static HWND g_btnSelJumpPhys = NULL;
static HWND g_btnSelJumpVirt = NULL;
static HWND g_btnSelCrouchPhys = NULL;
static HWND g_btnSelCrouchVirt = NULL;
static HWND g_btnSelWeaponSwap = NULL;

static AppConfig g_cfg;

// Tema moderno - Cores inspiradas em Windows 11 e Material Design
static HBRUSH g_brWnd = NULL, g_brEdit = NULL, g_brBtn = NULL, g_brTab = NULL;
static COLORREF g_colBg = RGB(32, 32, 32);           // Fundo escuro moderno
static COLORREF g_colCtrl = RGB(45, 45, 45);         // Controles
static COLORREF g_colEdit = RGB(55, 55, 55);         // Campos de entrada
static COLORREF g_colBtn = RGB(0, 120, 215);         // Botões azul Windows
static COLORREF g_colBtnHover = RGB(0, 102, 184);    // Botões hover
static COLORREF g_colText = RGB(255, 255, 255);      // Texto branco
static COLORREF g_colTextDim = RGB(200, 200, 200);   // Texto secundário
static COLORREF g_colAccent = RGB(0, 120, 215);      // Cor de destaque

// Captura de teclas
static HWND g_captureTarget = NULL;
static HHOOK g_capHook = NULL;

// Dimensões fixas da janela - MENOR
static const int WINDOW_WIDTH = 600;
static const int WINDOW_HEIGHT = 500; // Aumentado para acomodar todos os controles em uma única página

static LRESULT CALLBACK CaptureKbProc(int nCode, WPARAM wParam, LPARAM lParam) {
	if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
		KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
		if (g_captureTarget) {
			char buf[32] = {0};
			// Mapear VK para string simples (A-Z, 0-9, especiais básicos)
			if (p->vkCode >= 'A' && p->vkCode <= 'Z') { buf[0] = (char)p->vkCode; buf[1] = 0; }
			else if (p->vkCode >= '0' && p->vkCode <= '9') { buf[0] = (char)p->vkCode; buf[1] = 0; }
			else if (p->vkCode == VK_SPACE) strcpy_s(buf, sizeof(buf), "SPACE");
			else if (p->vkCode == VK_RETURN) strcpy_s(buf, sizeof(buf), "ENTER");
			else if (p->vkCode >= VK_F1 && p->vkCode <= VK_F24) { wsprintfA(buf, "F%u", (unsigned)(p->vkCode - VK_F1 + 1)); }
			else if (p->vkCode == VK_TAB) strcpy_s(buf, sizeof(buf), "TAB");
			else if (p->vkCode == VK_ESCAPE) strcpy_s(buf, sizeof(buf), "ESC");
			else if (p->vkCode == VK_SHIFT) strcpy_s(buf, sizeof(buf), "SHIFT");
			else if (p->vkCode == VK_CONTROL) strcpy_s(buf, sizeof(buf), "CTRL");
			else if (p->vkCode == VK_MENU) strcpy_s(buf, sizeof(buf), "ALT");
			if (buf[0]) {
				wchar_t wbuf[32] = {0};
				MultiByteToWideChar(CP_UTF8, 0, buf, -1, wbuf, 32);
				SetWindowTextW(g_captureTarget, wbuf);
			}
			UnhookWindowsHookEx(g_capHook); g_capHook = NULL; g_captureTarget = NULL;
			return 1;
		}
	}
	return CallNextHookEx(g_capHook, nCode, wParam, lParam);
}

static void StartCapture(HWND target) {
	g_captureTarget = target;
	g_capHook = SetWindowsHookEx(WH_KEYBOARD_LL, CaptureKbProc, GetModuleHandle(NULL), 0);
	SetWindowTextW(g_statusText, L"Pressione uma tecla...");
}



static void UI_LoadToControls() {
	SetWindowTextW(g_editTrigger, Utf8ToW(g_cfg.trigger_key).c_str());
	SetWindowTextW(g_editHold, Utf8ToW(g_cfg.hold_key).c_str());
	SetWindowTextW(g_editJumpPhys, Utf8ToW(g_cfg.jump_physical_key).c_str());
	SetWindowTextW(g_editJumpVirt, Utf8ToW(g_cfg.jump_virtual_key).c_str());
	SetWindowTextW(g_editCrouchPhys, Utf8ToW(g_cfg.crouch_physical_key).c_str());
	SetWindowTextW(g_editCrouchVirt, Utf8ToW(g_cfg.crouch_virtual_key).c_str());
	if (g_editWeaponSwap) SetWindowTextW(g_editWeaponSwap, Utf8ToW(g_cfg.weapon_swap_key).c_str());
	SetWindowTextW(g_statusText, L"Mapeamento ativo! Pressione a tecla de ativação para alternar.");
}

static void UI_SaveFromControls() {
	wchar_t wbuf[256];
	GetWindowTextW(g_editTrigger, wbuf, 256); g_cfg.trigger_key = WToUtf8(wbuf);
	GetWindowTextW(g_editHold, wbuf, 256); g_cfg.hold_key = WToUtf8(wbuf);
	GetWindowTextW(g_editJumpPhys, wbuf, 256); g_cfg.jump_physical_key = WToUtf8(wbuf);
	GetWindowTextW(g_editJumpVirt, wbuf, 256); g_cfg.jump_virtual_key = WToUtf8(wbuf);
	GetWindowTextW(g_editCrouchPhys, wbuf, 256); g_cfg.crouch_physical_key = WToUtf8(wbuf);
	GetWindowTextW(g_editCrouchVirt, wbuf, 256); g_cfg.crouch_virtual_key = WToUtf8(wbuf);
	if (g_editWeaponSwap) {
		GetWindowTextW(g_editWeaponSwap, wbuf, 256); g_cfg.weapon_swap_key = WToUtf8(wbuf);
	}
	// Configurações do sistema são lidas do arquivo config.json, não da UI
	SaveConfig(g_cfg);
	SetRunAtStartup(g_cfg.start_with_windows);
}

static void LayoutResize(RECT rc) {
	int padding = 20; // Padding menor para janela menor
	int startY = padding + 20; // Começar após o padding superior
	
	int x = padding + 24, y = startY, w = 200, h = 32; // Controles menores
	
	// Configurações principais
	MoveWindow(g_editTrigger, x, y, w, h, TRUE);
	MoveWindow(g_btnSelTrigger, x + w + 12, y, 100, h, TRUE);
	y += h + 16;
	
	MoveWindow(g_editHold, x, y, w, h, TRUE);
	MoveWindow(g_btnSelHold, x + w + 12, y, 100, h, TRUE);
	y += h + 24;
	
	MoveWindow(g_statusText, x, y, w + 112, h, TRUE);
	y += h + 32;

	// Teclas Extras - Pulo
	MoveWindow(g_editJumpPhys, x, y, w, h, TRUE);
	MoveWindow(g_btnSelJumpPhys, x + w + 12, y, 100, h, TRUE);
	y += h + 16;
	
	MoveWindow(g_editJumpVirt, x, y, w, h, TRUE);
	MoveWindow(g_btnSelJumpVirt, x + w + 12, y, 100, h, TRUE);
	y += h + 16;
	
	// Teclas Extras - Agachar
	MoveWindow(g_editCrouchPhys, x, y, w, h, TRUE);
	MoveWindow(g_btnSelCrouchPhys, x + w + 12, y, 100, h, TRUE);
	y += h + 16;
	
	MoveWindow(g_editCrouchVirt, x, y, w, h, TRUE);
	MoveWindow(g_btnSelCrouchVirt, x + w + 12, y, 100, h, TRUE);
	y += h + 16;
	
	// Teclas Extras - Weapon Swap
	if (g_editWeaponSwap) {
		MoveWindow(g_editWeaponSwap, x, y, w, h, TRUE);
		MoveWindow(g_btnSelWeaponSwap, x + w + 12, y, 100, h, TRUE);
	}
}

static void CreateControls(HWND hwnd) {
	// Todas as configurações em uma única página - posições iniciais
	int startY = 20;
	int x = 44, y = startY, w = 200, h = 32;
	
	// Configurações principais
	g_editTrigger = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)201, GetModuleHandle(NULL), NULL);
	g_btnSelTrigger = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)211, GetModuleHandle(NULL), NULL);
	y += h + 16;
	
	g_editHold = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)202, GetModuleHandle(NULL), NULL);
	g_btnSelHold = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)212, GetModuleHandle(NULL), NULL);
	y += h + 24;
	
	g_statusText = CreateWindowExW(0, L"STATIC", L"", WS_CHILD|WS_VISIBLE|SS_CENTER,
		x, y, w + 112, h, hwnd, (HMENU)203, GetModuleHandle(NULL), NULL);
	y += h + 32;

	// Teclas Extras - Pulo
	g_editJumpPhys = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)301, GetModuleHandle(NULL), NULL);
	g_btnSelJumpPhys = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)311, GetModuleHandle(NULL), NULL);
	y += h + 16;
	
	g_editJumpVirt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)302, GetModuleHandle(NULL), NULL);
	g_btnSelJumpVirt = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)312, GetModuleHandle(NULL), NULL);
	y += h + 16;
	
	// Teclas Extras - Agachar
	g_editCrouchPhys = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)303, GetModuleHandle(NULL), NULL);
	g_btnSelCrouchPhys = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)313, GetModuleHandle(NULL), NULL);
	y += h + 16;
	
	g_editCrouchVirt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)304, GetModuleHandle(NULL), NULL);
	g_btnSelCrouchVirt = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)314, GetModuleHandle(NULL), NULL);
	y += h + 16;
	
	// Teclas Extras - Weapon Swap
	g_editWeaponSwap = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		x, y, w, h, hwnd, (HMENU)305, GetModuleHandle(NULL), NULL);
	g_btnSelWeaponSwap = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		x + w + 12, y, 100, h, hwnd, (HMENU)315, GetModuleHandle(NULL), NULL);

	// Aplicar fonte Segoe UI em todos os controles
	HWND ctrls1[] = { g_editTrigger, g_editHold, g_statusText, g_btnSelTrigger, g_btnSelHold,
		g_editJumpPhys, g_editJumpVirt, g_editCrouchPhys, g_editCrouchVirt, g_editWeaponSwap,
		g_btnSelJumpPhys, g_btnSelJumpVirt, g_btnSelCrouchPhys, g_btnSelCrouchVirt, g_btnSelWeaponSwap };
	for (HWND h : ctrls1) if (h) ApplyUIFont(h);
}


static LRESULT CALLBACK UI_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		LoadConfig(g_cfg);
		CreateControls(hwnd);
		UI_LoadToControls();
		return 0;
	case WM_SIZE: {
		RECT rc; GetClientRect(hwnd, &rc);
		LayoutResize(rc);
		return 0;
	}
	case WM_COMMAND: {
		UINT id = LOWORD(wParam); UINT code = HIWORD(wParam);
		if (code == BN_CLICKED) {
			if (id == 211) StartCapture(g_editTrigger);
			if (id == 212) StartCapture(g_editHold);
			if (id == 311) StartCapture(g_editJumpPhys);
			if (id == 312) StartCapture(g_editJumpVirt);
			if (id == 313) StartCapture(g_editCrouchPhys);
			if (id == 314) StartCapture(g_editCrouchVirt);
			if (id == 315 && g_editWeaponSwap) StartCapture(g_editWeaponSwap);
		}
		return 0;
	}
	case WM_CTLCOLORSTATIC:
		SetTextColor((HDC)wParam, g_colText);
		SetBkColor((HDC)wParam, g_colBg);
		return (LRESULT)g_brWnd;
	case WM_CTLCOLORBTN:
		SetTextColor((HDC)wParam, g_colText);
		SetBkColor((HDC)wParam, g_colBtn);
		return (LRESULT)g_brBtn;
	case WM_CTLCOLOREDIT:
		SetTextColor((HDC)wParam, g_colText);
		SetBkColor((HDC)wParam, g_colEdit);
		return (LRESULT)g_brEdit;
	case WM_ERASEBKGND: {
		RECT r; GetClientRect(hwnd, &r);
		HBRUSH br = g_brWnd; FillRect((HDC)wParam, &r, br);
		return 1;
	}
	case WM_CLOSE:
		UI_SaveFromControls();
		DestroyWindow(hwnd);
		g_mainWnd = NULL;
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND UI_Show(HINSTANCE hInstance, HWND owner) {
	if (g_mainWnd) {
		ShowWindow(g_mainWnd, SW_SHOW);
		SetForegroundWindow(g_mainWnd);
		return g_mainWnd;
	}
	const wchar_t* cls = L"KeyMapperUIWnd";
	WNDCLASSEXW wc{}; 
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = UI_WndProc; 
	wc.hInstance = hInstance; 
	wc.lpszClassName = cls;
	wc.hbrBackground = NULL;
	
	// Carregar ícone personalizado para a janela
	HICON hIcon = (HICON)LoadImage(NULL, L"imagens\\logo.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
	if (hIcon) {
		wc.hIcon = hIcon;
		wc.hIconSm = hIcon;
	}
	
	RegisterClassExW(&wc);
	
	// Criar brushes para o tema moderno
	if (!g_brWnd) g_brWnd = CreateSolidBrush(g_colBg);
	if (!g_brEdit) g_brEdit = CreateSolidBrush(g_colEdit);
	if (!g_brBtn) g_brBtn = CreateSolidBrush(g_colBtn);
	if (!g_brTab) g_brTab = CreateSolidBrush(g_colCtrl);
	
	// Calcular posição central da tela
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int x = (screenWidth - WINDOW_WIDTH) / 2;
	int y = (screenHeight - WINDOW_HEIGHT) / 2;
	
	// Criar janela com tamanho fixo MENOR
	g_mainWnd = CreateWindowExW(WS_EX_APPWINDOW, cls, L"KeyMapper - Configurações",
		WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME), x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
		owner, NULL, hInstance, NULL);
	
	// Habilitar dark mode no título (Windows 10/11)
	BOOL value = TRUE;
	DwmSetWindowAttribute(g_mainWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
	
	return g_mainWnd;
}


