#define WIN32_LEAN_AND_MEAN
#include "config.h"
#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <shlwapi.h>

// Sistema de log simples para debug do autostart
static void LogAutostart(const std::string& message) {
	std::wstring logPath;
	PWSTR path = nullptr;
	SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
	if (path) {
		logPath = path;
		CoTaskMemFree(path);
		logPath += L"\\KeyMapper\\autostart_debug.log";
		
		std::ofstream log(logPath, std::ios::app);
		if (log) {
			SYSTEMTIME st;
			GetLocalTime(&st);
			log << "[" << st.wHour << ":" << st.wMinute << ":" << st.wSecond << "] " << message << std::endl;
		}
	}
}

// Helper: cria tarefa via XML com RunLevel=HighestAvailable e LogonTrigger
static bool CreateTaskViaXml(const wchar_t* taskName, const wchar_t* exePath) {
	if (!taskName || !exePath) return false;
	PWSTR localApp = nullptr;
	if (SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &localApp) != S_OK || !localApp) return false;
	std::wstring dir = localApp; CoTaskMemFree(localApp);
	std::wstring xmlPath = dir + L"\\KeyMapper\\keymapper_task.xml";
	CreateDirectoryW((dir + L"\\KeyMapper").c_str(), NULL);

	// WorkingDirectory = pasta do executável
	std::wstring exe(exePath);
	std::wstring workDir = exe;
	size_t pos = workDir.find_last_of(L"\\/");
	if (pos != std::wstring::npos) workDir = workDir.substr(0, pos); else workDir = L".";

	std::wstring xml =
		L"<?xml version=\"1.0\" encoding=\"UTF-16\"?>\n"
		L"<Task version=\"1.4\" xmlns=\"http://schemas.microsoft.com/windows/2004/02/mit/task\">\n"
		L"  <RegistrationInfo/>\n"
		L"  <Triggers>\n"
		L"    <LogonTrigger>\n"
		L"      <Enabled>true</Enabled>\n"
		L"    </LogonTrigger>\n"
		L"  </Triggers>\n"
		L"  <Principals>\n"
		L"    <Principal id=\"Author\">\n"
		L"      <RunLevel>HighestAvailable</RunLevel>\n"
		L"      <LogonType>InteractiveToken</LogonType>\n"
		L"    </Principal>\n"
		L"  </Principals>\n"
		L"  <Settings>\n"
		L"    <MultipleInstancesPolicy>IgnoreNew</MultipleInstancesPolicy>\n"
		L"    <DisallowStartIfOnBatteries>false</DisallowStartIfOnBatteries>\n"
		L"    <StopIfGoingOnBatteries>false</StopIfGoingOnBatteries>\n"
		L"    <StartWhenAvailable>true</StartWhenAvailable>\n"
		L"  </Settings>\n"
		L"  <Actions Context=\"Author\">\n"
		L"    <Exec>\n"
		L"      <Command>" + exe + L"</Command>\n"
		L"      <Arguments>--autostart</Arguments>\n"
		L"      <WorkingDirectory>" + workDir + L"</WorkingDirectory>\n"
		L"    </Exec>\n"
		L"  </Actions>\n"
		L"</Task>\n";

	// Gravar em UTF-16LE com BOM
	HANDLE h = CreateFileW(xmlPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		LogAutostart("CreateTaskViaXml: falha ao abrir arquivo XML");
		return false;
	}
	DWORD written = 0; WORD bom = 0xFEFF; // BOM LE
	WriteFile(h, &bom, sizeof(bom), &written, NULL);
	WriteFile(h, xml.data(), (DWORD)(xml.size() * sizeof(wchar_t)), &written, NULL);
	CloseHandle(h);

	wchar_t cmd[1024];
	wsprintfW(cmd, L"schtasks /Create /TN %s /XML \"%s\" /F", taskName, xmlPath.c_str());
	char logMsg[2048]; WideCharToMultiByte(CP_UTF8, 0, cmd, -1, logMsg, sizeof(logMsg), NULL, NULL);
	LogAutostart(std::string("Comando schtasks (XML): ") + logMsg);
	STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
	BOOL ok = CreateProcessW(NULL, cmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	DWORD exitCode = 1;
	if (ok) {
		WaitForSingleObject(pi.hProcess, 10000);
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	if (exitCode == 0) {
		LogAutostart("SUCESSO: Tarefa criada via XML");
		return true;
	}
	LogAutostart("ERRO: schtasks via XML retornou codigo " + std::to_string(exitCode));
	return false;
}

static std::string Trim(const std::string& s) {
	size_t i = 0, j = s.size();
	while (i < j && (unsigned char)s[i] <= ' ') ++i;
	while (j > i && (unsigned char)s[j - 1] <= ' ') --j;
	return s.substr(i, j - i);
}

static bool ExtractString(const std::string& json, const char* key, std::string& out) {
	std::string k = std::string("\"") + key + "\"";
	size_t p = json.find(k);
	if (p == std::string::npos) return false;
	p = json.find(':', p);
	if (p == std::string::npos) return false;
	p++;
	while (p < json.size() && (unsigned char)json[p] <= ' ') ++p;
	if (p >= json.size() || json[p] != '"') return false;
	++p; size_t start = p;
	while (p < json.size() && json[p] != '"') ++p;
	if (p >= json.size()) return false;
	out = json.substr(start, p - start);
	return true;
}

static bool ExtractBool(const std::string& json, const char* key, bool& out) {
	std::string k = std::string("\"") + key + "\"";
	size_t p = json.find(k);
	if (p == std::string::npos) return false;
	p = json.find(':', p);
	if (p == std::string::npos) return false;
	p++;
	while (p < json.size() && (unsigned char)json[p] <= ' ') ++p;
	if (json.compare(p, 4, "true") == 0) { out = true; return true; }
	if (json.compare(p, 5, "false") == 0) { out = false; return true; }
	return false;
}

std::wstring GetConfigPathW() {
	PWSTR path = nullptr;
	SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &path);
	std::wstring dir = path ? path : L".";
	if (path) CoTaskMemFree(path);
	dir += L"\\KeyMapper";
	CreateDirectoryW(dir.c_str(), NULL);
	return dir + L"\\key_config.json";
}

bool LoadConfig(AppConfig& cfg) {
	std::wstring cfgPath = GetConfigPathW();
	std::ifstream f(cfgPath, std::ios::binary);
	if (!f) return false;
	std::ostringstream ss; ss << f.rdbuf();
	std::string json = ss.str();
	if (json.empty()) return false;

	std::string s;
	bool b;
	if (ExtractString(json, "trigger_key", s)) cfg.trigger_key = Trim(s);
	if (ExtractString(json, "hold_key", s)) cfg.hold_key = Trim(s);
	if (ExtractString(json, "jump_physical_key", s)) cfg.jump_physical_key = Trim(s);
	if (ExtractString(json, "jump_virtual_key", s)) cfg.jump_virtual_key = Trim(s);
	if (ExtractString(json, "crouch_physical_key", s)) cfg.crouch_physical_key = Trim(s);
	if (ExtractString(json, "crouch_virtual_key", s)) cfg.crouch_virtual_key = Trim(s);

	return true;
}

bool SaveConfig(const AppConfig& cfg) {
	std::wstring cfgPath = GetConfigPathW();
	std::ofstream f(cfgPath, std::ios::binary | std::ios::trunc);
	if (!f) return false;
	f << "{\n";
	f << "  \"trigger_key\": \"" << cfg.trigger_key << "\",\n";
	f << "  \"hold_key\": \"" << cfg.hold_key << "\",\n";
	f << "  \"jump_physical_key\": \"" << cfg.jump_physical_key << "\",\n";
	f << "  \"jump_virtual_key\": \"" << cfg.jump_virtual_key << "\",\n";
	f << "  \"crouch_physical_key\": \"" << cfg.crouch_physical_key << "\",\n";
	f << "  \"crouch_virtual_key\": \"" << cfg.crouch_virtual_key << "\",\n";
	f << "  \"start_with_windows\": " << (cfg.start_with_windows ? "true" : "false") << ",\n";
	f << "  \"start_minimized\": " << (cfg.start_minimized ? "true" : "false") << "\n";
	f << "}\n";
	return true;
}

bool SetRunAtStartup(bool enable) {
	LogAutostart(enable ? "SetRunAtStartup: HABILITANDO autostart" : "SetRunAtStartup: DESABILITANDO autostart");
	
	// Sempre usar Tarefa Agendada porque o manifesto requer administrador (UAC bloqueia HKCU Run)
	// Limpar possível entrada no registro para evitar duplicidade/confusão
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
		RegDeleteValueW(hKey, L"KeyMapper");
		RegCloseKey(hKey);
		LogAutostart("Removida entrada antiga do registro HKCU Run");
	}

	const wchar_t* taskName = L"KeyMapper_Autostart";
	if (enable) {
		wchar_t exePath[MAX_PATH];
		GetModuleFileNameW(NULL, exePath, MAX_PATH);
		wchar_t cmdQuoted[MAX_PATH + 32];
		wsprintfW(cmdQuoted, L"\"%s\" --autostart", exePath);

		// Converter para string para log
		char logMsg[2048];
		WideCharToMultiByte(CP_UTF8, 0, exePath, -1, logMsg, sizeof(logMsg), NULL, NULL);
		LogAutostart(std::string("Caminho do executavel: ") + logMsg);

		wchar_t createCmd[1024];
		// Remover /IT (interativo) para evitar erro 2147500037 em alguns ambientes; manter /RL HIGHEST
		wsprintfW(createCmd, L"schtasks /Create /TN %s /TR \"%s\" /SC ONLOGON /RL HIGHEST /DELAY 0000:10 /F",
					 taskName, cmdQuoted);
		
		WideCharToMultiByte(CP_UTF8, 0, createCmd, -1, logMsg, sizeof(logMsg), NULL, NULL);
		LogAutostart(std::string("Comando schtasks: ") + logMsg);
		
		STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
		BOOL ok = CreateProcessW(NULL, createCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		if (!ok) {
			LogAutostart("ERRO: CreateProcessW falhou para criar tarefa");
			return false;
		}
		// Aguardar conclusão e validar código de saída do schtasks (0 = sucesso)
		WaitForSingleObject(pi.hProcess, 10000);
		DWORD exitCode = 1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		if (exitCode == 0) {
			LogAutostart("SUCESSO: Tarefa agendada criada com sucesso");
			return true;
		} else {
			LogAutostart("ERRO: schtasks retornou codigo " + std::to_string(exitCode) + "; tentando sem /RL HIGHEST...");
		}
		
		// Fallback 1: tentar sem /RL HIGHEST
		wsprintfW(createCmd, L"schtasks /Create /TN %s /TR \"%s\" /SC ONLOGON /DELAY 0000:10 /F", taskName, cmdQuoted);
		WideCharToMultiByte(CP_UTF8, 0, createCmd, -1, logMsg, sizeof(logMsg), NULL, NULL);
		LogAutostart(std::string("Comando schtasks (fallback1): ") + logMsg);
		ZeroMemory(&si, sizeof(si)); si.cb = sizeof(si); ZeroMemory(&pi, sizeof(pi));
		ok = CreateProcessW(NULL, createCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		exitCode = 1;
		if (ok) {
			WaitForSingleObject(pi.hProcess, 10000);
			GetExitCodeProcess(pi.hProcess, &exitCode);
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		if (exitCode == 0) {
			LogAutostart("SUCESSO: Tarefa criada no fallback1 (sem /RL HIGHEST)");
			return true;
		}
		LogAutostart("ERRO: schtasks falhou no fallback1 com codigo " + std::to_string(exitCode) + "; tentando criar via XML...");
		
		// Fallback 1.5: criar via XML com RunLevel=HighestAvailable
		if (CreateTaskViaXml(taskName, exePath)) {
			return true;
		}
		
		LogAutostart("ERRO: criação via XML falhou; aplicando fallback2 (HKCU Run)...");
		
		// Fallback 2: criar entrada HKCU Run
		if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, NULL, 0, KEY_SET_VALUE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
			wchar_t runCmd[MAX_PATH + 64];
			wsprintfW(runCmd, L"\"%s\" --autostart", exePath);
			LONG r = RegSetValueExW(hKey, L"KeyMapper", 0, REG_SZ, (const BYTE*)runCmd, (DWORD)((wcslen(runCmd) + 1) * sizeof(wchar_t)));
			RegCloseKey(hKey);
			if (r == ERROR_SUCCESS) {
				LogAutostart("Fallback2: entrada no registro HKCU Run criada com sucesso");
				return true;
			}
		}
		LogAutostart("ERRO: Falharam todas as tentativas de configurar autostart");
		return false;
	} else {
		wchar_t deleteCmd[512];
		wsprintfW(deleteCmd, L"schtasks /Delete /TN %s /F", taskName);
		STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
		BOOL ok = CreateProcessW(NULL, deleteCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
		if (!ok) {
			LogAutostart("ERRO: CreateProcessW falhou para deletar tarefa");
			return false;
		}
		WaitForSingleObject(pi.hProcess, 10000);
		DWORD exitCode = 1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		
		LogAutostart("Tarefa deletada, codigo retorno: " + std::to_string(exitCode));
		return exitCode == 0;
	}
}

bool IsRunAtStartupEnabled() {
	// Priorizar verificação da Tarefa Agendada devido ao manifesto com requireAdministrator
	wchar_t queryCmd[512];
	wsprintfW(queryCmd, L"schtasks /Query /TN KeyMapper_Autostart");
	STARTUPINFOW si{}; si.cb = sizeof(si); PROCESS_INFORMATION pi{};
	BOOL ok = CreateProcessW(NULL, queryCmd, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
	if (ok) {
		WaitForSingleObject(pi.hProcess, 3000);
		DWORD exitCode = 1;
		GetExitCodeProcess(pi.hProcess, &exitCode);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		if (exitCode == 0) return true; // Tarefa existe
	}

	// Fallback: checar registro (para instalações antigas)
	HKEY hKey;
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
		DWORD type = 0; DWORD size = 0;
		LONG res = RegQueryValueExW(hKey, L"KeyMapper", NULL, &type, NULL, &size);
		RegCloseKey(hKey);
		return res == ERROR_SUCCESS && type == REG_SZ;
	}
	return false;
}
