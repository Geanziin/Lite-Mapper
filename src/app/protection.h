#pragma once
#include <windows.h>
#include <string>
#include <vector>

// Sistema de proteção avançada contra detecção e engenharia reversa
namespace Protection {

// Ofuscação de strings em tempo de compilação
#define OBFUSCATE_STR(str) Protection::StringObfuscator::obfuscate(str)
#define DEOBFUSCATE_STR(obf_str) Protection::StringObfuscator::deobfuscate(obf_str)

// Proteção contra análise estática
#define JUNK_CODE() Protection::JunkCodeGenerator::generate()

// Proteção contra debugging
#define ANTI_DEBUG_CHECK() Protection::AntiDebug::check()

// Proteção contra análise dinâmica
#define ANTI_ANALYSIS_CHECK() Protection::AntiAnalysis::check()

// Proteção contra virtualização
#define ANTI_VM_CHECK() Protection::AntiVM::check()

// Proteção contra sandbox
#define ANTI_SANDBOX_CHECK() Protection::AntiSandbox::check()

// Ofuscação de imports
#define OBFUSCATE_IMPORT(lib, func) Protection::ImportObfuscator::get(lib, func)

// Criptografia de dados sensíveis
#define ENCRYPT_DATA(data, key) Protection::Crypto::encrypt(data, key)
#define DECRYPT_DATA(encrypted, key) Protection::Crypto::decrypt(encrypted, key)

// Proteção de memória
#define PROTECT_MEMORY(ptr, size) Protection::MemoryProtector::protect(ptr, size)
#define UNPROTECT_MEMORY(ptr, size) Protection::MemoryProtector::unprotect(ptr, size)

// Verificação de integridade
#define CHECK_INTEGRITY() Protection::IntegrityChecker::verify()

// Inicialização do sistema de proteção
void initialize();

// Limpeza do sistema de proteção
void cleanup();

// Classe para ofuscação de strings
class StringObfuscator {
public:
    static std::string obfuscate(const char* str);
    static std::string deobfuscate(const std::string& obf_str);
    // Disponibilizado publicamente para inicialização explícita
    static void generateKey();
private:
    static std::vector<BYTE> key;
};

// Classe para geração de código lixo
class JunkCodeGenerator {
public:
    static void generate();
private:
    static void generateArithmetic();
    static void generateBranches();
    static void generateLoops();
};

// Classe para proteção contra debugging
class AntiDebug {
public:
    static bool check();
    static void startWatchdog();
    static void stopWatchdog();
private:
    static HANDLE watchdogThread;
    static DWORD WINAPI watchdogProc(LPVOID);
    static bool checkIsDebuggerPresent();
    static bool checkRemoteDebugger();
    static bool checkPebFlags();
    static bool checkTiming();
    static bool checkHardwareBreakpoints();
    static bool checkSoftwareBreakpoints();
    static bool checkInt3();
    static bool checkTrapFlag();
};

// Classe para proteção contra análise
class AntiAnalysis {
public:
    static bool check();
private:
    static bool checkProcessList();
    static bool checkWindowTitles();
    static bool checkRegistryKeys();
    static bool checkFileSystem();
    static bool checkNetworkConnections();
    static bool checkSystemInfo();
    static bool checkUserInteraction();
    static bool checkExecutionTime();
};

// Classe para proteção contra VM
class AntiVM {
public:
    static bool check();
private:
    static bool checkRegistryVM();
    static bool checkProcessListVM();
    static bool checkFileSystemVM();
    static bool checkHardwareVM();
    static bool checkTimingVM();
    static bool checkInstructionSet();
};

// Classe para proteção contra sandbox
class AntiSandbox {
public:
    static bool check();
private:
    static bool checkExecutionTime();
    static bool checkUserActivity();
    static bool checkSystemResources();
    static bool checkNetworkActivity();
    static bool checkFileAccess();
};

// Classe para ofuscação de imports
class ImportObfuscator {
public:
    template<typename T>
    static T get(const char* lib, const char* func);
private:
    static HMODULE loadLibrary(const char* lib);
    static FARPROC getProcAddress(HMODULE hModule, const char* func);
};

// Classe para criptografia
class Crypto {
public:
    static std::vector<BYTE> encrypt(const std::vector<BYTE>& data, const std::vector<BYTE>& key);
    static std::vector<BYTE> decrypt(const std::vector<BYTE>& encrypted, const std::vector<BYTE>& key);
    static std::string encryptString(const std::string& str, const std::string& key);
    static std::string decryptString(const std::string& encrypted, const std::string& key);
private:
    static void xorCrypt(std::vector<BYTE>& data, const std::vector<BYTE>& key);
    static std::vector<BYTE> generateKey(const std::string& password);
};

// Classe para proteção de memória
class MemoryProtector {
public:
    static bool protect(void* ptr, size_t size);
    static bool unprotect(void* ptr, size_t size);
    static void encryptMemory(void* ptr, size_t size);
    static void decryptMemory(void* ptr, size_t size);
    // Disponibilizado publicamente para inicialização explícita
    static void generateMemoryKey();
private:
    static std::vector<BYTE> memoryKey;
};

// Classe para verificação de integridade
class IntegrityChecker {
public:
    static bool verify();
private:
    static bool verifyCodeSection();
    static bool verifyDataSection();
    static bool verifyImports();
    static bool verifyChecksum();
};

} // namespace Protection
