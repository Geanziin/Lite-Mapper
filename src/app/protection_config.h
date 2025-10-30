#pragma once

// Configurações do sistema de proteção
#define PROTECTION_ENABLED 1
#define PROTECTION_LEVEL 3  // 1=Baixo, 2=Médio, 3=Alto

// Configurações de anti-debug
#define ANTI_DEBUG_ENABLED 1
#define ANTI_DEBUG_WATCHDOG_INTERVAL 500  // ms
#define ANTI_DEBUG_TIMING_THRESHOLD 100   // ms

// Configurações de anti-VM
#define ANTI_VM_ENABLED 1
#define ANTI_VM_REGISTRY_CHECK 1
#define ANTI_VM_PROCESS_CHECK 1
#define ANTI_VM_HARDWARE_CHECK 1

// Configurações de anti-sandbox
#define ANTI_SANDBOX_ENABLED 1
#define ANTI_SANDBOX_TIMING_CHECK 1
#define ANTI_SANDBOX_USER_ACTIVITY_CHECK 1

// Configurações de ofuscação
#define STRING_OBFUSCATION_ENABLED 1
#define STRING_OBFUSCATION_KEY_SIZE 32
#define MEMORY_ENCRYPTION_ENABLED 1

// Configurações de integridade
#define INTEGRITY_CHECK_ENABLED 1
#define INTEGRITY_CHECK_INTERVAL 10000  // ms

// Configurações de código lixo
#define JUNK_CODE_ENABLED 1
#define JUNK_CODE_FREQUENCY 0.3  // 30% de chance de executar

// Configurações de criptografia
#define CRYPTO_ENABLED 1
#define CRYPTO_KEY_SIZE 32
#define CRYPTO_ALGORITHM "XOR"  // XOR, AES, etc.

// Configurações de proteção de memória
#define MEMORY_PROTECTION_ENABLED 1
#define MEMORY_PROTECTION_LEVEL 2  // 1=Baixo, 2=Médio, 3=Alto

// Configurações de ofuscação de imports
#define IMPORT_OBFUSCATION_ENABLED 1
#define IMPORT_OBFUSCATION_DELAY 100  // ms

// Configurações de detecção de análise
#define ANTI_ANALYSIS_ENABLED 1
#define ANTI_ANALYSIS_PROCESS_CHECK 1
#define ANTI_ANALYSIS_WINDOW_CHECK 1
#define ANTI_ANALYSIS_REGISTRY_CHECK 1

// Configurações de comportamento em detecção
#define DETECTION_RESPONSE "EXIT"  // EXIT, HANG, CORRUPT, JUNK
#define DETECTION_DELAY 1000  // ms antes de executar resposta

// Configurações de logging (apenas em debug)
#ifdef _DEBUG
#define PROTECTION_LOGGING_ENABLED 1
#define PROTECTION_LOG_LEVEL 2  // 1=Erro, 2=Warning, 3=Info, 4=Debug
#else
#define PROTECTION_LOGGING_ENABLED 0
#define PROTECTION_LOG_LEVEL 0
#endif

// Configurações de performance
#define PROTECTION_PERFORMANCE_MODE 0  // 0=Segurança máxima, 1=Equilibrado, 2=Performance
#define PROTECTION_CHECK_INTERVAL 5000  // ms entre verificações

// Configurações de evasão
#define EVASION_ENABLED 1
#define EVASION_RANDOM_DELAY 1
#define EVASION_CODE_MUTATION 1
#define EVASION_STRING_ENCRYPTION 1

// Configurações de persistência
#define PERSISTENCE_ENABLED 0  // 0=Desabilitado, 1=Habilitado
#define PERSISTENCE_METHOD "REGISTRY"  // REGISTRY, TASK, SERVICE

// Configurações de comunicação
#define COMMUNICATION_ENABLED 0  // 0=Desabilitado, 1=Habilitado
#define COMMUNICATION_PROTOCOL "HTTP"  // HTTP, HTTPS, TCP, UDP
#define COMMUNICATION_ENDPOINT ""  // URL ou IP:Porta

// Configurações de atualização
#define UPDATE_ENABLED 0  // 0=Desabilitado, 1=Habilitado
#define UPDATE_CHECK_INTERVAL 86400000  // ms (24 horas)
#define UPDATE_URL ""

// Configurações de telemetria
#define TELEMETRY_ENABLED 0  // 0=Desabilitado, 1=Habilitado
#define TELEMETRY_INTERVAL 300000  // ms (5 minutos)

// Configurações de sandbox evasion
#define SANDBOX_EVASION_ENABLED 1
#define SANDBOX_EVASION_DELAY 30000  // ms (30 segundos)
#define SANDBOX_EVASION_USER_INPUT 1
#define SANDBOX_EVASION_SYSTEM_RESOURCES 1

// Configurações de VM evasion
#define VM_EVASION_ENABLED 1
#define VM_EVASION_HARDWARE_CHECK 1
#define VM_EVASION_TIMING_CHECK 1
#define VM_EVASION_INSTRUCTION_CHECK 1

// Configurações de debug evasion
#define DEBUG_EVASION_ENABLED 1
#define DEBUG_EVASION_BREAKPOINT_CHECK 1
#define DEBUG_EVASION_TRAP_FLAG_CHECK 1
#define DEBUG_EVASION_TIMING_CHECK 1
#define DEBUG_EVASION_PEB_CHECK 1

// Configurações de análise evasion
#define ANALYSIS_EVASION_ENABLED 1
#define ANALYSIS_EVASION_PROCESS_CHECK 1
#define ANALYSIS_EVASION_WINDOW_CHECK 1
#define ANALYSIS_EVASION_FILE_CHECK 1
#define ANALYSIS_EVASION_REGISTRY_CHECK 1

// Configurações de código lixo avançado
#define ADVANCED_JUNK_CODE_ENABLED 1
#define ADVANCED_JUNK_CODE_COMPLEXITY 3  // 1=Simples, 2=Médio, 3=Complexo
#define ADVANCED_JUNK_CODE_BRANCHES 1
#define ADVANCED_JUNK_CODE_LOOPS 1
#define ADVANCED_JUNK_CODE_EXCEPTIONS 1

// Configurações de ofuscação avançada
#define ADVANCED_OBFUSCATION_ENABLED 1
#define ADVANCED_OBFUSCATION_CONTROL_FLOW 1
#define ADVANCED_OBFUSCATION_DATA_FLOW 1
#define ADVANCED_OBFUSCATION_STRING_ENCRYPTION 1
#define ADVANCED_OBFUSCATION_IMPORT_TABLE 1

// Configurações de criptografia avançada
#define ADVANCED_CRYPTO_ENABLED 1
#define ADVANCED_CRYPTO_ALGORITHM "AES"  // XOR, AES, RSA, etc.
#define ADVANCED_CRYPTO_KEY_SIZE 256
#define ADVANCED_CRYPTO_IV_SIZE 16

// Configurações de proteção de memória avançada
#define ADVANCED_MEMORY_PROTECTION_ENABLED 1
#define ADVANCED_MEMORY_PROTECTION_ENCRYPTION 1
#define ADVANCED_MEMORY_PROTECTION_COMPRESSION 1
#define ADVANCED_MEMORY_PROTECTION_SCRAMBLING 1

// Configurações de integridade avançada
#define ADVANCED_INTEGRITY_ENABLED 1
#define ADVANCED_INTEGRITY_CHECKSUM 1
#define ADVANCED_INTEGRITY_SIGNATURE 1
#define ADVANCED_INTEGRITY_HASH 1

// Configurações de evasão avançada
#define ADVANCED_EVASION_ENABLED 1
#define ADVANCED_EVASION_POLYMORPHISM 1
#define ADVANCED_EVASION_METAMORPHISM 1
#define ADVANCED_EVASION_ENCRYPTION 1
#define ADVANCED_EVASION_PACKING 1
