#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSystemTrayIcon>
#include <QMenu>
#include <memory>

#include "../app/config.h"

class SettingsWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget* parent = nullptr);
    ~SettingsWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    // UI
    QTabWidget* tabWidget = nullptr;

    // Page 0 - Configurações
    QLineEdit* editTrigger = nullptr;
    QLineEdit* editHold = nullptr;
    QLabel* statusText = nullptr;
    QPushButton* btnSelTrigger = nullptr;
    QPushButton* btnSelHold = nullptr;
    QCheckBox* chkOnlyBluestacks = nullptr;

    // Page 1 - Teclas Extras
    QLineEdit* editJumpPhys = nullptr;
    QLineEdit* editJumpVirt = nullptr;
    QLineEdit* editCrouchPhys = nullptr;
    QLineEdit* editCrouchVirt = nullptr;
    QLineEdit* editWeaponSwap = nullptr;
    QPushButton* btnSelJumpPhys = nullptr;
    QPushButton* btnSelJumpVirt = nullptr;
    QPushButton* btnSelCrouchPhys = nullptr;
    QPushButton* btnSelCrouchVirt = nullptr;
    QPushButton* btnSelWeaponSwap = nullptr;

    // Page 2 - Sistema
    QCheckBox* chkStartWin = nullptr;
    QCheckBox* chkStartMin = nullptr;

    // Tray
    QSystemTrayIcon* trayIcon = nullptr;
    QMenu* trayMenu = nullptr;

    // Data
    AppConfig cfg;

    // Capture
    QLineEdit* captureTarget = nullptr;
    void startCapture(QLineEdit* target);
    void stopCapture();
    QString keyToString(int key, Qt::KeyboardModifiers mods) const;

    // Actions
    void loadToControls();
    void saveFromControls();
    void setupUi();
    void setupTray();

    // Helpers
    void applyMonospaceToKeyFields();
    void applyHookConfig();

    // Tweakers
    void applyBluestacksTweaks();
    void applyBluestacksSensi();
    void applyMouseFix();

public slots:
    void bringToFront();
};


