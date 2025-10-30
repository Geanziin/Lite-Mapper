#include <QApplication>
#include <QFont>
#include <QString>
#include <QLocalServer>
#include <QLocalSocket>
#include "SettingsWindow.h"
#include "../app/config.h"
#include "../app/antidebug.h"
#include "../keyhook.h"

#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <fstream>
#endif

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    // Start anti-debug watchdog (best-effort)
    AntiDebug_Start();

    // Single instance: try connect to existing instance
    const QString instanceKey = "KeyMapper.SingleInstance";
    {
        QLocalSocket sock;
        sock.connectToServer(instanceKey, QIODevice::WriteOnly);
        if (sock.waitForConnected(150)) {
            sock.write("SHOW");
            sock.flush();
            sock.waitForBytesWritten(100);
            return 0;
        }
    }

    // Fonte padrão consistente (evita espaçamento estranho)
    QFont uiFont("Segoe UI", 10);
    uiFont.setKerning(true);
    uiFont.setStyleStrategy(QFont::PreferAntialias);
    uiFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.0);
    app.setFont(uiFont);

    // Create server to accept future signals
    QLocalServer server;
    QLocalServer::removeServer(instanceKey);
    server.listen(instanceKey);

    // Load and configure hook
    AppConfig cfg; LoadConfig(cfg);
    
    // Aplicar configuração de inicialização automática
    #ifdef _WIN32
    // Log para debug
    wchar_t* localAppData = nullptr;
    SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, (PWSTR*)&localAppData);
    if (localAppData) {
        std::wstring logPath = localAppData;
        logPath += L"\\KeyMapper\\qt_startup_debug.log";
        std::ofstream debugLog(logPath, std::ios::app);
        if (debugLog) {
            debugLog << "qt_main.cpp: start_with_windows = " << (cfg.start_with_windows ? "true" : "false") << std::endl;
            debugLog << "qt_main.cpp: Chamando SetRunAtStartup(" << (cfg.start_with_windows ? "true" : "false") << ")" << std::endl;
        }
        CoTaskMemFree(localAppData);
    }
    SetRunAtStartup(cfg.start_with_windows);
    #endif
    
    std::string hold1 = "w";
    configure_keys(
        cfg.jump_physical_key.c_str(),
        cfg.jump_virtual_key.c_str(),
        cfg.crouch_physical_key.c_str(),
        cfg.crouch_virtual_key.c_str(),
        cfg.trigger_key.c_str(),
        hold1.c_str(),
        cfg.hold_key.c_str(),
        cfg.weapon_swap_key.c_str()
    );
    start_hook();
    set_restrict_to_emulators(cfg.only_bluestacks ? 1 : 0);
    if (!cfg.trigger_key.empty() && !cfg.hold_key.empty()) {
        set_active(1);
    }

    SettingsWindow w;
    QObject::connect(&server, &QLocalServer::newConnection, &w, [&](){
        while (QLocalSocket* s = server.hasPendingConnections() ? server.nextPendingConnection() : nullptr) {
            s->readAll();
            s->close();
            s->deleteLater();
            w.bringToFront();
        }
    });
    const bool isAutoStart = QApplication::arguments().contains("--autostart");
    
    // Debug log para autostart
    #ifdef _WIN32
    {
        wchar_t* debugLocalAppData = nullptr;
        SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, (PWSTR*)&debugLocalAppData);
        if (debugLocalAppData) {
            std::wstring logPath = std::wstring(debugLocalAppData) + L"\\KeyMapper\\qt_startup_debug.log";
            std::ofstream debugLog(logPath, std::ios::app);
            if (debugLog) {
                debugLog << "qt_main.cpp: isAutoStart = " << (isAutoStart ? "true" : "false") << std::endl;
                debugLog << "qt_main.cpp: start_minimized = " << (cfg.start_minimized ? "true" : "false") << std::endl;
            }
            CoTaskMemFree(debugLocalAppData);
        }
    }
    #endif
    
    if (isAutoStart) {
        // Quando iniciado automaticamente pelo Windows
        if (cfg.start_minimized) {
            // Iniciar direto na bandeja sem mostrar janela
            w.hide();
        } else {
            // Mostrar janela normalmente
            w.show();
        }
    } else {
        // Início manual - sempre mostrar janela
        w.show();
    }
    int rc = app.exec();
    stop_hook();
    return rc;
}


