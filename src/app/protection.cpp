#include "protection.h"
#include <tlhelp32.h>
#include <psapi.h>
#include <winreg.h>
#include <iphlpapi.h>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

namespace Protection {

// Implementação da classe StringObfuscator
std::vector<BYTE> StringObfuscator::key;

void StringObfuscator::generateKey() {
    if (key.empty()) {
        key.resize(32);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& k : key) k = static_cast<BYTE>(dis(gen));
    }
}

std::string StringObfuscator::obfuscate(const char* str) {
    generateKey();
    std::string result;
    for (size_t i = 0; str[i]; ++i) {
        result += static_cast<char>(str[i] ^ key[i % key.size()]);
    }
    return result;
}

std::string StringObfuscator::deobfuscate(const std::string& obf_str) {
    generateKey();
    std::string result;
    for (size_t i = 0; i < obf_str.size(); ++i) {
        result += static_cast<char>(obf_str[i] ^ key[i % key.size()]);
    }
    return result;
}

// Implementação da classe JunkCodeGenerator
void JunkCodeGenerator::generate() {
    generateArithmetic();
    generateBranches();
    generateLoops();
}

void JunkCodeGenerator::generateArithmetic() {
    volatile int a = 42, b = 17, c = 0;
    c = a + b;
    c = a - b;
    c = a * b;
    c = a / b;
    c = a % b;
    c = a & b;
    c = a | b;
    c = a ^ b;
    c = ~a;
    c = a << 2;
    c = a >> 1;
}

void JunkCodeGenerator::generateBranches() {
    volatile int x = GetTickCount() % 100;
    if (x > 50) {
        x = x * 2;
    } else if (x > 25) {
        x = x + 10;
    } else {
        x = x - 5;
    }
    
    switch (x % 3) {
        case 0: x += 1; break;
        case 1: x += 2; break;
        case 2: x += 3; break;
    }
}

void JunkCodeGenerator::generateLoops() {
    volatile int sum = 0;
    for (int i = 0; i < 5; ++i) {
        sum += i;
    }
    
    int j = 0;
    while (j < 3) {
        sum += j;
        j++;
    }
    
    do {
        sum += 10;
    } while (sum < 100);
}

// Implementação da classe AntiDebug
HANDLE AntiDebug::watchdogThread = nullptr;

bool AntiDebug::check() {
    if (checkIsDebuggerPresent()) return true;
    if (checkRemoteDebugger()) return true;
    if (checkPebFlags()) return true;
    if (checkTiming()) return true;
    if (checkHardwareBreakpoints()) return true;
    if (checkSoftwareBreakpoints()) return true;
    if (checkInt3()) return true;
    if (checkTrapFlag()) return true;
    return false;
}

bool AntiDebug::checkIsDebuggerPresent() {
    return IsDebuggerPresent() != FALSE;
}

bool AntiDebug::checkRemoteDebugger() {
    BOOL remote = FALSE;
    return CheckRemoteDebuggerPresent(GetCurrentProcess(), &remote) && remote;
}

bool AntiDebug::checkPebFlags() {
    __try {
        #ifdef _M_X64
        PBYTE pPeb = (PBYTE)__readgsqword(0x60);
        #else
        PBYTE pPeb = (PBYTE)__readfsdword(0x30);
        #endif
        if (pPeb && pPeb[2]) return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}

bool AntiDebug::checkTiming() {
    LARGE_INTEGER freq, start, end;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    Sleep(1);
    QueryPerformanceCounter(&end);
    
    double elapsed = (double)(end.QuadPart - start.QuadPart) / freq.QuadPart;
    return elapsed > 0.1; // Se demorou mais de 100ms, provavelmente está sendo debugado
}

bool AntiDebug::checkHardwareBreakpoints() {
    CONTEXT ctx = {0};
    ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
    if (GetThreadContext(GetCurrentThread(), &ctx)) {
        return (ctx.Dr0 != 0 || ctx.Dr1 != 0 || ctx.Dr2 != 0 || ctx.Dr3 != 0);
    }
    return false;
}

bool AntiDebug::checkSoftwareBreakpoints() {
    #if defined(_M_IX86)
    __try {
        __asm int 3;
        return false;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
    #else
    // Inline asm não suportado em x64/MSVC; já cobrimos via APIs, retornar falso aqui
    return false;
    #endif
}

bool AntiDebug::checkInt3() {
    #if defined(_M_IX86)
    __try {
        __asm int 3;
        return false;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
    #else
    return false;
    #endif
}

bool AntiDebug::checkTrapFlag() {
    #if defined(_M_IX86)
    __try {
        __asm pushf
        __asm pop eax
        __asm or eax, 0x100
        __asm push eax
        __asm popf
        __asm pushf
        __asm pop eax
        __asm and eax, 0x100
        return eax == 0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        return true;
    }
    #else
    return false;
    #endif
}

DWORD WINAPI AntiDebug::watchdogProc(LPVOID) {
    while (true) {
        if (check()) {
            // Se debugger detectado, fazer o processo mais difícil de analisar
            Sleep(100 + (GetTickCount() % 200));
            JUNK_CODE();
        }
        Sleep(500);
    }
    return 0;
}

void AntiDebug::startWatchdog() {
    if (!watchdogThread) {
        watchdogThread = CreateThread(NULL, 0, watchdogProc, NULL, 0, NULL);
    }
}

void AntiDebug::stopWatchdog() {
    if (watchdogThread) {
        TerminateThread(watchdogThread, 0);
        CloseHandle(watchdogThread);
        watchdogThread = nullptr;
    }
}

// Implementação da classe AntiAnalysis
bool AntiAnalysis::check() {
    if (checkProcessList()) return true;
    if (checkWindowTitles()) return true;
    if (checkRegistryKeys()) return true;
    if (checkFileSystem()) return true;
    if (checkNetworkConnections()) return true;
    if (checkSystemInfo()) return true;
    if (checkUserInteraction()) return true;
    if (checkExecutionTime()) return true;
    return false;
}

bool AntiAnalysis::checkProcessList() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;
    
    PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    bool found = false;
    if (Process32First(hSnapshot, &pe32)) {
        do {
            std::wstring processName = pe32.szExeFile;
            std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
            
            if (processName.find(L"ollydbg") != std::wstring::npos ||
                processName.find(L"x64dbg") != std::wstring::npos ||
                processName.find(L"windbg") != std::wstring::npos ||
                processName.find(L"ida") != std::wstring::npos ||
                processName.find(L"ghidra") != std::wstring::npos ||
                processName.find(L"processhacker") != std::wstring::npos ||
                processName.find(L"procmon") != std::wstring::npos) {
                found = true;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return found;
}

bool AntiAnalysis::checkWindowTitles() {
    bool found = false;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        wchar_t title[256] = {0};
        GetWindowTextW(hwnd, title, 255);
        std::wstring titleStr = title;
        std::transform(titleStr.begin(), titleStr.end(), titleStr.begin(), ::tolower);
        
        if (titleStr.find(L"ollydbg") != std::wstring::npos ||
            titleStr.find(L"x64dbg") != std::wstring::npos ||
            titleStr.find(L"windbg") != std::wstring::npos ||
            titleStr.find(L"ida") != std::wstring::npos ||
            titleStr.find(L"ghidra") != std::wstring::npos) {
            *reinterpret_cast<bool*>(lParam) = true;
            return FALSE;
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&found));
    return found;
}

bool AntiAnalysis::checkRegistryKeys() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // Verificar chaves de análise
        RegCloseKey(hKey);
    }
    return false;
}

bool AntiAnalysis::checkFileSystem() {
    // Verificar arquivos de análise
    return false;
}

bool AntiAnalysis::checkNetworkConnections() {
    // Verificar conexões de rede suspeitas
    return false;
}

bool AntiAnalysis::checkSystemInfo() {
    // Verificar informações do sistema
    return false;
}

bool AntiAnalysis::checkUserInteraction() {
    // Verificar interação do usuário
    return false;
}

bool AntiAnalysis::checkExecutionTime() {
    // Verificar tempo de execução
    return false;
}

// Implementação da classe AntiVM
bool AntiVM::check() {
    if (checkRegistryVM()) return true;
    if (checkProcessListVM()) return true;
    if (checkFileSystemVM()) return true;
    if (checkHardwareVM()) return true;
    if (checkTimingVM()) return true;
    if (checkInstructionSet()) return true;
    return false;
}

bool AntiVM::checkRegistryVM() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\Scsi", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
    }
    return false;
}

bool AntiVM::checkProcessListVM() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) return false;
    
    PROCESSENTRY32 pe32 = {0};
    pe32.dwSize = sizeof(PROCESSENTRY32);
    
    bool found = false;
    if (Process32First(hSnapshot, &pe32)) {
        do {
            std::wstring processName = pe32.szExeFile;
            std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
            
            if (processName.find(L"vmtools") != std::wstring::npos ||
                processName.find(L"vboxservice") != std::wstring::npos ||
                processName.find(L"vboxtray") != std::wstring::npos ||
                processName.find(L"vmusrvc") != std::wstring::npos ||
                processName.find(L"vmsrvc") != std::wstring::npos) {
                found = true;
                break;
            }
        } while (Process32Next(hSnapshot, &pe32));
    }
    
    CloseHandle(hSnapshot);
    return found;
}

bool AntiVM::checkFileSystemVM() {
    // Verificar arquivos específicos de VM
    return false;
}

bool AntiVM::checkHardwareVM() {
    // Verificar hardware específico de VM
    return false;
}

bool AntiVM::checkTimingVM() {
    // Verificar timing específico de VM
    return false;
}

bool AntiVM::checkInstructionSet() {
    // Verificar conjunto de instruções
    return false;
}

// Implementação da classe AntiSandbox
bool AntiSandbox::check() {
    if (checkExecutionTime()) return true;
    if (checkUserActivity()) return true;
    if (checkSystemResources()) return true;
    if (checkNetworkActivity()) return true;
    if (checkFileAccess()) return true;
    return false;
}

bool AntiSandbox::checkExecutionTime() {
    // Verificar se o processo está sendo executado muito rapidamente
    return false;
}

bool AntiSandbox::checkUserActivity() {
    // Verificar atividade do usuário
    return false;
}

bool AntiSandbox::checkSystemResources() {
    // Verificar recursos do sistema
    return false;
}

bool AntiSandbox::checkNetworkActivity() {
    // Verificar atividade de rede
    return false;
}

bool AntiSandbox::checkFileAccess() {
    // Verificar acesso a arquivos
    return false;
}

// Implementação da classe Crypto
std::vector<BYTE> Crypto::encrypt(const std::vector<BYTE>& data, const std::vector<BYTE>& key) {
    std::vector<BYTE> result = data;
    xorCrypt(result, key);
    return result;
}

std::vector<BYTE> Crypto::decrypt(const std::vector<BYTE>& encrypted, const std::vector<BYTE>& key) {
    std::vector<BYTE> result = encrypted;
    xorCrypt(result, key);
    return result;
}

std::string Crypto::encryptString(const std::string& str, const std::string& password) {
    std::vector<BYTE> data(str.begin(), str.end());
    std::vector<BYTE> key = generateKey(password);
    std::vector<BYTE> encrypted = encrypt(data, key);
    return std::string(encrypted.begin(), encrypted.end());
}

std::string Crypto::decryptString(const std::string& encrypted, const std::string& password) {
    std::vector<BYTE> data(encrypted.begin(), encrypted.end());
    std::vector<BYTE> key = generateKey(password);
    std::vector<BYTE> decrypted = decrypt(data, key);
    return std::string(decrypted.begin(), decrypted.end());
}

void Crypto::xorCrypt(std::vector<BYTE>& data, const std::vector<BYTE>& key) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % key.size()];
    }
}

std::vector<BYTE> Crypto::generateKey(const std::string& password) {
    std::vector<BYTE> key(32);
    for (size_t i = 0; i < key.size(); ++i) {
        key[i] = static_cast<BYTE>(password[i % password.size()] ^ (i * 7 + 13));
    }
    return key;
}

// Implementação da classe MemoryProtector
std::vector<BYTE> MemoryProtector::memoryKey;

void MemoryProtector::generateMemoryKey() {
    if (memoryKey.empty()) {
        memoryKey.resize(16);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        for (auto& k : memoryKey) k = static_cast<BYTE>(dis(gen));
    }
}

bool MemoryProtector::protect(void* ptr, size_t size) {
    DWORD oldProtect;
    return VirtualProtect(ptr, size, PAGE_READWRITE, &oldProtect) != FALSE;
}

bool MemoryProtector::unprotect(void* ptr, size_t size) {
    DWORD oldProtect;
    return VirtualProtect(ptr, size, PAGE_EXECUTE_READWRITE, &oldProtect) != FALSE;
}

void MemoryProtector::encryptMemory(void* ptr, size_t size) {
    generateMemoryKey();
    BYTE* data = static_cast<BYTE*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= memoryKey[i % memoryKey.size()];
    }
}

void MemoryProtector::decryptMemory(void* ptr, size_t size) {
    generateMemoryKey();
    BYTE* data = static_cast<BYTE*>(ptr);
    for (size_t i = 0; i < size; ++i) {
        data[i] ^= memoryKey[i % memoryKey.size()];
    }
}

// Implementação da classe IntegrityChecker
bool IntegrityChecker::verify() {
    if (!verifyCodeSection()) return false;
    if (!verifyDataSection()) return false;
    if (!verifyImports()) return false;
    if (!verifyChecksum()) return false;
    return true;
}

bool IntegrityChecker::verifyCodeSection() {
    // Verificar integridade da seção de código
    return true;
}

bool IntegrityChecker::verifyDataSection() {
    // Verificar integridade da seção de dados
    return true;
}

bool IntegrityChecker::verifyImports() {
    // Verificar integridade dos imports
    return true;
}

bool IntegrityChecker::verifyChecksum() {
    // Verificar checksum do executável
    return true;
}

// Implementação da classe ImportObfuscator
template<typename T>
T ImportObfuscator::get(const char* lib, const char* func) {
    HMODULE hModule = loadLibrary(lib);
    if (!hModule) return nullptr;
    
    FARPROC proc = getProcAddress(hModule, func);
    return reinterpret_cast<T>(proc);
}

HMODULE ImportObfuscator::loadLibrary(const char* lib) {
    return LoadLibraryA(lib);
}

FARPROC ImportObfuscator::getProcAddress(HMODULE hModule, const char* func) {
    return GetProcAddress(hModule, func);
}

// Funções globais do namespace
void initialize() {
    StringObfuscator::generateKey();
    MemoryProtector::generateMemoryKey();
    AntiDebug::startWatchdog();
}

void cleanup() {
    AntiDebug::stopWatchdog();
}

} // namespace Protection
