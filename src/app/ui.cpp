#include "ui.h"
#include "config.h"
#include <commctrl.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#include <dwmapi.h>
#include <shellapi.h>

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
		lf.lfHeight = -14; // ~10pt (menor)
		wcscpy_s(lf.lfFaceName, L"Segoe UI");
		hFont = CreateFontIndirectW(&lf);
	}
	SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
}

static void ApplyLabelFont(HWND hwnd) {
	static HFONT hLabelFont = NULL;
	if (!hLabelFont) {
		LOGFONTW lf{};
		lf.lfHeight = -13; // ~9pt
		wcscpy_s(lf.lfFaceName, L"Segoe UI");
		hLabelFont = CreateFontIndirectW(&lf);
	}
	SendMessage(hwnd, WM_SETFONT, (WPARAM)hLabelFont, TRUE);
}

// Controles
static HWND g_mainWnd = NULL;

// Labels
static HWND g_lblTrigger = NULL;
static HWND g_lblHold = NULL;
static HWND g_lblJumpPhys = NULL;
static HWND g_lblJumpVirt = NULL;
static HWND g_lblCrouchPhys = NULL;
static HWND g_lblCrouchVirt = NULL;
static HWND g_lblSuporte = NULL;  // Label "Suporte" clicável
static HWND g_lblFooter = NULL;   // Label "KeyMapper by Geanswag"

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
static HWND g_btnSelJumpPhys = NULL;
static HWND g_btnSelJumpVirt = NULL;
static HWND g_btnSelCrouchPhys = NULL;
static HWND g_btnSelCrouchVirt = NULL;

static AppConfig g_cfg;

// Tema moderno - Cores inspiradas em Windows 11 e Material Design
static HBRUSH g_brWnd = NULL, g_brEdit = NULL, g_brBtn = NULL;
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
static const int WINDOW_WIDTH = 480;
static const int WINDOW_HEIGHT = 450;

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
	// Configurações do sistema são lidas do arquivo config.json, não da UI
	SaveConfig(g_cfg);
}

static void LayoutResize(RECT rc) {
	// Layout compacto com labels - manter posições fixas (não redimensionar)
	int startY = 15;
	int labelX = 15, editX = 180, btnX = 340;
	int y = startY;
	int labelW = 155, editW = 140, btnW = 90;
	int h = 26;
	
	// Configurações principais - Trigger
	if (g_lblTrigger) MoveWindow(g_lblTrigger, labelX, y + 3, labelW, h, TRUE);
	if (g_editTrigger) MoveWindow(g_editTrigger, editX, y, editW, h, TRUE);
	if (g_btnSelTrigger) MoveWindow(g_btnSelTrigger, btnX, y, btnW, h, TRUE);
	y += h + 10;
	
	// Hold
	if (g_lblHold) MoveWindow(g_lblHold, labelX, y + 3, labelW, h, TRUE);
	if (g_editHold) MoveWindow(g_editHold, editX, y, editW, h, TRUE);
	if (g_btnSelHold) MoveWindow(g_btnSelHold, btnX, y, btnW, h, TRUE);
	y += h + 18;
	
	// Status
	if (g_statusText) MoveWindow(g_statusText, labelX, y, labelW + editW + btnW + 10, h, TRUE);
	y += h + 20;

	// Teclas Extras - Pulo Físico
	if (g_lblJumpPhys) MoveWindow(g_lblJumpPhys, labelX, y + 3, labelW, h, TRUE);
	if (g_editJumpPhys) MoveWindow(g_editJumpPhys, editX, y, editW, h, TRUE);
	if (g_btnSelJumpPhys) MoveWindow(g_btnSelJumpPhys, btnX, y, btnW, h, TRUE);
	y += h + 10;
	
	// Pulo Virtual
	if (g_lblJumpVirt) MoveWindow(g_lblJumpVirt, labelX, y + 3, labelW, h, TRUE);
	if (g_editJumpVirt) MoveWindow(g_editJumpVirt, editX, y, editW, h, TRUE);
	if (g_btnSelJumpVirt) MoveWindow(g_btnSelJumpVirt, btnX, y, btnW, h, TRUE);
	y += h + 10;
	
	// Agachar Físico
	if (g_lblCrouchPhys) MoveWindow(g_lblCrouchPhys, labelX, y + 3, labelW, h, TRUE);
	if (g_editCrouchPhys) MoveWindow(g_editCrouchPhys, editX, y, editW, h, TRUE);
	if (g_btnSelCrouchPhys) MoveWindow(g_btnSelCrouchPhys, btnX, y, btnW, h, TRUE);
	y += h + 10;
	
	// Agachar Virtual
	if (g_lblCrouchVirt) MoveWindow(g_lblCrouchVirt, labelX, y + 3, labelW, h, TRUE);
	if (g_editCrouchVirt) MoveWindow(g_editCrouchVirt, editX, y, editW, h, TRUE);
	if (g_btnSelCrouchVirt) MoveWindow(g_btnSelCrouchVirt, btnX, y, btnW, h, TRUE);
	y += h + 20;
	
	// Footer - parte inferior
	int footerY = rc.bottom - 25; // 25px do fundo
	// Label "Suporte" à esquerda
	if (g_lblSuporte) MoveWindow(g_lblSuporte, 15, footerY, 100, 20, TRUE);
	// Label "KeyMapper by Geanswag" à direita
	if (g_lblFooter) {
		int footerWidth = 200;
		int footerX = rc.right - footerWidth - 15; // 15px da margem direita
		MoveWindow(g_lblFooter, footerX, footerY, footerWidth, 20, TRUE);
	}
}

static void CreateControls(HWND hwnd) {
	// Layout compacto com labels explicativos
	int startY = 15;
	int labelX = 15, editX = 180, btnX = 340;
	int y = startY;
	int labelW = 155, editW = 140, btnW = 90;
	int h = 26; // Altura menor
	
	// Configurações principais - Trigger
	g_lblTrigger = CreateWindowExW(0, L"STATIC", L"Tecla de Ativação:", WS_CHILD|WS_VISIBLE|SS_LEFT,
		labelX, y + 3, labelW, h, hwnd, NULL, GetModuleHandle(NULL), NULL);
	g_editTrigger = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		editX, y, editW, h, hwnd, (HMENU)201, GetModuleHandle(NULL), NULL);
	g_btnSelTrigger = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		btnX, y, btnW, h, hwnd, (HMENU)211, GetModuleHandle(NULL), NULL);
	y += h + 10;
	
	// Hold
	g_lblHold = CreateWindowExW(0, L"STATIC", L"Tecla de Segurar:", WS_CHILD|WS_VISIBLE|SS_LEFT,
		labelX, y + 3, labelW, h, hwnd, NULL, GetModuleHandle(NULL), NULL);
	g_editHold = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		editX, y, editW, h, hwnd, (HMENU)202, GetModuleHandle(NULL), NULL);
	g_btnSelHold = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		btnX, y, btnW, h, hwnd, (HMENU)212, GetModuleHandle(NULL), NULL);
	y += h + 18;
	
	// Status
	g_statusText = CreateWindowExW(0, L"STATIC", L"", WS_CHILD|WS_VISIBLE|SS_CENTER,
		labelX, y, labelW + editW + btnW + 10, h, hwnd, (HMENU)203, GetModuleHandle(NULL), NULL);
	y += h + 20;

	// Teclas Extras - Pulo Físico
	g_lblJumpPhys = CreateWindowExW(0, L"STATIC", L"Tecla Física de Pulo:", WS_CHILD|WS_VISIBLE|SS_LEFT,
		labelX, y + 3, labelW, h, hwnd, NULL, GetModuleHandle(NULL), NULL);
	g_editJumpPhys = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		editX, y, editW, h, hwnd, (HMENU)301, GetModuleHandle(NULL), NULL);
	g_btnSelJumpPhys = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		btnX, y, btnW, h, hwnd, (HMENU)311, GetModuleHandle(NULL), NULL);
	y += h + 10;
	
	// Pulo Virtual
	g_lblJumpVirt = CreateWindowExW(0, L"STATIC", L"Tecla Virtual de Pulo:", WS_CHILD|WS_VISIBLE|SS_LEFT,
		labelX, y + 3, labelW, h, hwnd, NULL, GetModuleHandle(NULL), NULL);
	g_editJumpVirt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		editX, y, editW, h, hwnd, (HMENU)302, GetModuleHandle(NULL), NULL);
	g_btnSelJumpVirt = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		btnX, y, btnW, h, hwnd, (HMENU)312, GetModuleHandle(NULL), NULL);
	y += h + 10;
	
	// Agachar Físico
	g_lblCrouchPhys = CreateWindowExW(0, L"STATIC", L"Tecla Física de Agachar:", WS_CHILD|WS_VISIBLE|SS_LEFT,
		labelX, y + 3, labelW, h, hwnd, NULL, GetModuleHandle(NULL), NULL);
	g_editCrouchPhys = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		editX, y, editW, h, hwnd, (HMENU)303, GetModuleHandle(NULL), NULL);
	g_btnSelCrouchPhys = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		btnX, y, btnW, h, hwnd, (HMENU)313, GetModuleHandle(NULL), NULL);
	y += h + 10;
	
	// Agachar Virtual
	g_lblCrouchVirt = CreateWindowExW(0, L"STATIC", L"Tecla Virtual de Agachar:", WS_CHILD|WS_VISIBLE|SS_LEFT,
		labelX, y + 3, labelW, h, hwnd, NULL, GetModuleHandle(NULL), NULL);
	g_editCrouchVirt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|ES_AUTOHSCROLL|ES_CENTER,
		editX, y, editW, h, hwnd, (HMENU)304, GetModuleHandle(NULL), NULL);
	g_btnSelCrouchVirt = CreateWindowExW(0, L"BUTTON", L"Selecionar", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
		btnX, y, btnW, h, hwnd, (HMENU)314, GetModuleHandle(NULL), NULL);
	y += h + 10;

	// Footer - Labels na parte inferior
	// Label "Suporte" (clicável) à esquerda
	g_lblSuporte = CreateWindowExW(0, L"STATIC", L"Suporte", 
		WS_CHILD|WS_VISIBLE|SS_LEFT|SS_NOTIFY, 
		15, WINDOW_HEIGHT - 40, 100, 20, hwnd, (HMENU)401, GetModuleHandle(NULL), NULL);
	
	// Label "KeyMapper by Geanswag" à direita
	g_lblFooter = CreateWindowExW(0, L"STATIC", L"KeyMapper by Geanswag", 
		WS_CHILD|WS_VISIBLE|SS_RIGHT,
		WINDOW_WIDTH - 200, WINDOW_HEIGHT - 40, 185, 20, hwnd, (HMENU)402, GetModuleHandle(NULL), NULL);

	// Aplicar fontes
	HWND labels[] = { g_lblTrigger, g_lblHold, g_lblJumpPhys, g_lblJumpVirt, 
		g_lblCrouchPhys, g_lblCrouchVirt, g_lblSuporte, g_lblFooter };
	for (HWND h : labels) if (h) ApplyLabelFont(h);
	
	// Estilizar label "Suporte" como link (cor azul)
	if (g_lblSuporte) {
		SetWindowLongPtr(g_lblSuporte, GWL_STYLE, GetWindowLongPtr(g_lblSuporte, GWL_STYLE) | SS_NOTIFY);
	}
	
	HWND ctrls[] = { g_editTrigger, g_editHold, g_statusText, g_btnSelTrigger, g_btnSelHold,
		g_editJumpPhys, g_editJumpVirt, g_editCrouchPhys, g_editCrouchVirt,
		g_btnSelJumpPhys, g_btnSelJumpVirt, g_btnSelCrouchPhys, g_btnSelCrouchVirt };
	for (HWND h : ctrls) if (h) ApplyUIFont(h);
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
		} else if (code == STN_CLICKED) {
			// Label "Suporte" clicada - abrir Discord
			if (id == 401) {
				ShellExecuteW(NULL, L"open", L"https://discord.gg/JuEzkT4puD", NULL, NULL, SW_SHOWNORMAL);
			}
		}
		return 0;
	}
	case WM_CTLCOLORSTATIC: {
		HWND hwndCtrl = (HWND)lParam;
		// Label "Suporte" tem cor de link (azul)
		if (hwndCtrl == g_lblSuporte) {
			SetTextColor((HDC)wParam, RGB(0, 120, 215)); // Azul Windows
			SetBkColor((HDC)wParam, g_colBg);
			return (LRESULT)g_brWnd;
		}
		SetTextColor((HDC)wParam, g_colText);
		SetBkColor((HDC)wParam, g_colBg);
		return (LRESULT)g_brWnd;
	}
	case WM_SETCURSOR: {
		// Cursor de mão quando passar sobre a label "Suporte"
		POINT pt;
		GetCursorPos(&pt);
		HWND hwndUnder = WindowFromPoint(pt);
		if (hwndUnder == g_lblSuporte) {
			SetCursor(LoadCursor(NULL, IDC_HAND));
			return TRUE;
		}
		break;
	}
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
		// Fechar o programa completamente quando a janela de config for fechada
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

HWND UI_Show(HINSTANCE hInstance, HWND owner) {
	if (g_mainWnd && IsWindow(g_mainWnd)) {
		ShowWindow(g_mainWnd, SW_SHOW);
		SetForegroundWindow(g_mainWnd);
		return g_mainWnd;
	}
	const wchar_t* cls = L"KeyMapperUIWnd";
	
	// Verificar se a classe já foi registrada
	WNDCLASSEXW wc;
	if (!GetClassInfoExW(hInstance, cls, &wc)) {
		// Registrar a classe apenas se não existir
		WNDCLASSEXW wcNew{}; 
		wcNew.cbSize = sizeof(WNDCLASSEXW);
		wcNew.lpfnWndProc = UI_WndProc; 
		wcNew.hInstance = hInstance; 
		wcNew.lpszClassName = cls;
		wcNew.hbrBackground = NULL;
		wcNew.style = CS_HREDRAW | CS_VREDRAW;
		
		// Carregar ícone personalizado para a janela
		HICON hIcon = (HICON)LoadImage(NULL, L"imagens\\logo.ico", IMAGE_ICON, 32, 32, LR_LOADFROMFILE);
		if (hIcon) {
			wcNew.hIcon = hIcon;
			wcNew.hIconSm = hIcon;
		}
		
		if (!RegisterClassExW(&wcNew)) {
			// Se falhar, pode ser que já exista - continuar mesmo assim
		}
	}
	
	// Criar brushes para o tema moderno
	if (!g_brWnd) g_brWnd = CreateSolidBrush(g_colBg);
	if (!g_brEdit) g_brEdit = CreateSolidBrush(g_colEdit);
	if (!g_brBtn) g_brBtn = CreateSolidBrush(g_colBtn);
	
	// Calcular posição central da tela
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int x = (screenWidth - WINDOW_WIDTH) / 2;
	int y = (screenHeight - WINDOW_HEIGHT) / 2;
	
	// Criar janela com tamanho fixo MENOR
	g_mainWnd = CreateWindowExW(WS_EX_APPWINDOW, cls, L"KeyMapper - Lite",
		WS_OVERLAPPEDWINDOW & ~(WS_MAXIMIZEBOX | WS_THICKFRAME), x, y, WINDOW_WIDTH, WINDOW_HEIGHT,
		owner, NULL, hInstance, NULL);
	
	if (!g_mainWnd) {
		return NULL;
	}
	
	// Habilitar dark mode no título (Windows 10/11)
	BOOL value = TRUE;
	DwmSetWindowAttribute(g_mainWnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
	
	// Exibir a janela
	ShowWindow(g_mainWnd, SW_SHOW);
	UpdateWindow(g_mainWnd);
	
	return g_mainWnd;
}


