#include "SettingsWindow.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QWidget>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QFile>
#include <QTextStream>
#include <QStyle>
#include <QScreen>
#include <QIcon>
#include <QKeySequence>
#include <QGuiApplication>
#include <QFrame>
#include <QLabel>
#include <QFont>
#include <QTimer>
#include <QThread>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>
#include <QPushButton>
#include <QSize>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QStringList>
#include <QDir>
#include <QDebug>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QSequentialAnimationGroup>
#include <QEasingCurve>

#ifdef _WIN32
#  include <windows.h>
#  include <shlwapi.h>
#  include <dwmapi.h>
#  include <shlobj.h>
#  include "../app/protection.h"
#endif

#include "../keyhook.h"

static QString loadStylesheet() {
    // Modern dark style with smooth animations
    const char* css = R"( 
        QWidget { 
            background-color: #2A2C2F; 
            color: #F1F3F4; 
            font-family: 'Segoe UI', 'Inter', sans-serif; 
            font-size: 11px; 
            letter-spacing: 0px; 
        }
        
        QMainWindow {
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #2A2C2F, stop:0.5 #252729, stop:1 #2A2C2F);
        }
        
        QTabWidget::pane { 
            border: 1px solid #3C4043; 
            border-radius: 6px; 
            margin: 2px;
            background: rgba(42, 44, 47, 0.8);
        }
        
        QTabBar::tab { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #2D2F32, stop:1 #2A2C2F);
            padding: 4px 10px; 
            border-top-left-radius: 4px; 
            border-top-right-radius: 4px; 
            color: #B8BABD;
            margin-right: 2px;
            border: 1px solid transparent;
            font-size: 11px;
        }
        
        QTabBar::tab:hover { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #35373B, stop:1 #2D2F32);
            color: #E8EAED;
            border: 1px solid #FF8C3A;
        }
        
        QTabBar::tab:selected { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #FF8C3A, stop:1 #E67E22);
            color: #FFFFFF;
            font-weight: 600;
            border: 1px solid #FF8C3A;
        }
        
        QLineEdit { 
            background: #35373B; 
            border: 1px solid #3C4043; 
            border-radius: 4px; 
            padding: 3px 8px; 
            color: #E8EAED; 
            letter-spacing: 0px; 
            min-height: 20px;
            font-size: 11px;
            selection-background-color: #FF8C3A;
        }
        
        QLineEdit:focus { 
            border: 2px solid #FF8C3A;
            background: #3C3E42;
        }
        
        QLineEdit:hover { 
            border: 2px solid #4A5568;
            background: #3A3C40;
        }
        
        QLabel[role="pill"] { 
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0, 
                stop:0 #303134, stop:1 #35373B);
            border-radius: 4px; 
            padding: 4px 8px; 
            color: #E8EAED;
            border: 1px solid #3C4043;
            font-size: 11px;
        }
        
        QFrame[role="header"] { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #2D2F32, stop:1 #2A2C2F);
            border-bottom: 2px solid #FF8C3A; 
            padding: 6px 10px;
        }
        
        QLabel[role="headerTitle"] { 
            font-size: 18px; 
            font-weight: 800; 
            color: #FFFFFF;
        }
        
        QLabel[role="headerLink"] { 
            color: #FF8C3A;
            font-weight: 600;
            font-size: 11px;
        }
        
        QLabel[role="title"], QLabel[role="titleBlue"], QLabel[role="titleOrange"], QLabel[role="titlePurple"] { 
            font-weight: 700; 
            font-size: 13px; 
            margin: 1px 0 2px 0;
            padding: 1px 0;
        }
        
        QLabel[role="title"] { 
            color: #FF8C3A;
        }
        
        QLabel[role="titleBlue"] { 
            color: #5DADE2;
        }
        
        QLabel[role="titleOrange"] { 
            color: #F39C12;
        }
        
        QLabel[role="titlePurple"] { 
            color: #BB8FCE;
        }
        
        QLabel[role="hint"] { 
            color: #FF8C3A; 
            font-size: 10px;
            font-style: italic;
        }
        
        QPushButton { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #FF9D4A, stop:1 #FF8C3A);
            color: #FFFFFF; 
            border: none; 
            border-radius: 4px; 
            padding: 3px 10px; 
            font-weight: 600; 
            min-height: 22px;
            font-size: 11px;
        }
        
        QPushButton:hover { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #FFAE5A, stop:1 #FF9D4A);
        }
        
        QPushButton:pressed { 
            background: qlineargradient(x1:0, y1:0, x2:0, y2:1, 
                stop:0 #E67E22, stop:1 #D35400);
            padding-top: 8px;
            padding-bottom: 4px;
        }
        
        QCheckBox { 
            spacing: 6px; 
            font-size: 11px; 
            font-weight: 500; 
            color: #E8EAED;
            padding: 1px;
        }
        
        QCheckBox::indicator { 
            width: 16px; 
            height: 16px; 
            border-radius: 4px; 
            border: 2px solid #4A5568; 
            background: #2A2C2F;
        }
        
        QCheckBox::indicator:hover { 
            border-color: #FF8C3A; 
            background: #35373B;
        }
        
        QCheckBox::indicator:checked { 
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                stop:0 #FF8C3A, stop:1 #E67E22);
            border-color: #FF8C3A;
        }
        
        QCheckBox::indicator:checked:hover { 
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1, 
                stop:0 #FFAE5A, stop:1 #FF9D4A);
        }
        
        QCheckBox::indicator:disabled { 
            background: #3C4043; 
            border-color: #4A5568;
        }
        
        QToolTip { 
            background: #35373B; 
            color: #E8EAED; 
            border: 1px solid #FF8C3A;
            border-radius: 4px;
            padding: 3px 6px;
            font-size: 10px;
        }
    )";
    return QString::fromUtf8(css);
}

// Helper para adicionar animação de bounce em botões
static void addButtonAnimation(QPushButton* btn) {
    QObject::connect(btn, &QPushButton::pressed, btn, [btn]() {
        QPropertyAnimation* scaleDown = new QPropertyAnimation(btn, "geometry");
        scaleDown->setDuration(100);
        QRect original = btn->geometry();
        int shrink = 3;
        QRect smaller(original.x() + shrink, original.y() + shrink,
                     original.width() - shrink*2, original.height() - shrink*2);
        scaleDown->setStartValue(original);
        scaleDown->setEndValue(smaller);
        scaleDown->setEasingCurve(QEasingCurve::OutQuad);
        
        QObject::connect(scaleDown, &QPropertyAnimation::finished, btn, [btn, original]() {
            QPropertyAnimation* scaleUp = new QPropertyAnimation(btn, "geometry");
            scaleUp->setDuration(150);
            scaleUp->setStartValue(btn->geometry());
            scaleUp->setEndValue(original);
            scaleUp->setEasingCurve(QEasingCurve::OutElastic);
            scaleUp->start(QAbstractAnimation::DeleteWhenStopped);
        });
        
        scaleDown->start(QAbstractAnimation::DeleteWhenStopped);
    });
}

// Helper para adicionar animação nos campos quando ganham foco
static void addFocusAnimation(QLineEdit* lineEdit) {
    // Instalar event filter para capturar eventos de foco
    lineEdit->installEventFilter(new QObject(lineEdit));
    
    // Conectar ao evento de mudança de foco
    QObject::connect(lineEdit, &QLineEdit::selectionChanged, lineEdit, []() {
        // A animação de foco já está no CSS com border transition
    });
}

SettingsWindow::SettingsWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    setupTray();
    setStyleSheet(loadStylesheet());

    // Tamanho fixo (janela não redimensionável) - reduzido para ser mais compacto
    setFixedSize(520, 420);
    setWindowTitle("KeyMapper");
    
    // Debug: verificar recursos disponíveis
    #ifdef _DEBUG
    QDir resDir(":/imagens");
    if (resDir.exists()) {
        qDebug() << "Recursos encontrados em :/imagens";
        qDebug() << "Arquivos:" << resDir.entryList();
    } else {
        qDebug() << "ERRO: Recursos :/imagens não encontrados!";
    }
    #endif
    
    QIcon windowIconObj(":/imagens/logo.ico");
    if (windowIconObj.isNull()) {
        windowIconObj = QIcon(":/imagens/logo.png");
    }
    setWindowIcon(windowIconObj);

    // Center on screen
    if (QScreen* s = QGuiApplication::primaryScreen()) {
        QRect r = s->availableGeometry();
        move(r.center() - rect().center());
    }

    // Animação de fade in e slide ao abrir
    QGraphicsOpacityEffect* opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    
    QPropertyAnimation* fadeIn = new QPropertyAnimation(opacityEffect, "opacity", this);
    fadeIn->setDuration(400);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);
    
    // Animação de slide suave
    QPropertyAnimation* slideIn = new QPropertyAnimation(this, "pos", this);
    QPoint currentPos = pos();
    slideIn->setDuration(400);
    slideIn->setStartValue(QPoint(currentPos.x(), currentPos.y() - 30));
    slideIn->setEndValue(currentPos);
    slideIn->setEasingCurve(QEasingCurve::OutCubic);
    
    // Executar animações em paralelo
    QParallelAnimationGroup* openAnimation = new QParallelAnimationGroup(this);
    openAnimation->addAnimation(fadeIn);
    openAnimation->addAnimation(slideIn);
    
    // Remover efeito de opacidade após a animação para melhor performance
    connect(openAnimation, &QParallelAnimationGroup::finished, this, [this]() {
        setGraphicsEffect(nullptr);
    });
    
    openAnimation->start(QAbstractAnimation::DeleteWhenStopped);

    LoadConfig(cfg);
    loadToControls();

#ifdef _WIN32
    // Deixa a barra de título do Windows em modo escuro (Windows 10/11)
    BOOL dark = TRUE;
    DwmSetWindowAttribute((HWND)winId(), 20 /*DWMWA_USE_IMMERSIVE_DARK_MODE*/, &dark, sizeof(dark));
    
    // Inicializar proteções básicas
    Protection::initialize();
    
    // Timer para verificações periódicas de proteção
    QTimer* protectionTimer = new QTimer(this);
    connect(protectionTimer, &QTimer::timeout, this, [this]() {
        #ifdef _WIN32
        if (ANTI_DEBUG_CHECK() || ANTI_VM_CHECK() || ANTI_SANDBOX_CHECK()) {
            JUNK_CODE();
            QApplication::quit();
        }
        #endif
    });
    protectionTimer->start(5000); // Verificar a cada 5 segundos
#endif
}

SettingsWindow::~SettingsWindow() {
    saveFromControls();
}

void SettingsWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout* root = new QVBoxLayout(central);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(4);

    // Header bar
    QFrame* header = new QFrame(central);
    header->setProperty("role", "header");
    QHBoxLayout* hheader = new QHBoxLayout(header);
    hheader->setContentsMargins(0, 0, 0, 0);
    QLabel* appTitle = new QLabel("KeyMapper", header);
    appTitle->setProperty("role", "headerTitle");
    QLabel* by = new QLabel("by GEANSWAG", header);
    by->setProperty("role", "headerLink");
    hheader->addWidget(appTitle, 0, Qt::AlignVCenter|Qt::AlignLeft);
    hheader->addStretch(1);
    hheader->addWidget(by, 0, Qt::AlignVCenter|Qt::AlignRight);
    root->addWidget(header);

    tabWidget = new QTabWidget(central);
    root->addWidget(tabWidget);

    // Footer bar
    QFrame* footer = new QFrame(central);
    footer->setProperty("role", "header"); // reuse header style for consistent bar
    QHBoxLayout* hfooter = new QHBoxLayout(footer);
    hfooter->setContentsMargins(3, 3, 3, 3);
    // Left social icons with links
    QWidget* leftBox = new QWidget(footer);
    QHBoxLayout* leftLayout = new QHBoxLayout(leftBox);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(8);
    // Discord link com imagem clicável
    QLabel* discordLabel = new QLabel(leftBox);
    QPixmap discordPix(":/imagens/discord.png");
    if (discordPix.isNull()) {
        discordPix = QPixmap("imagens/discord.png");
    }
    if (!discordPix.isNull()) {
        discordLabel->setPixmap(discordPix.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        discordLabel->setText("D");
    }
    discordLabel->setFixedSize(24, 24);
    discordLabel->setAlignment(Qt::AlignCenter);
    discordLabel->setCursor(Qt::PointingHandCursor);
    discordLabel->setToolTip("Discord");
    discordLabel->setStyleSheet(
        "QLabel { "
        "   padding: 4px; "
        "   border-radius: 6px; "
        "   background-color: transparent; "
        "} "
        "QLabel:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255,140,58,0.2), stop:1 rgba(255,140,58,0.1)); "
        "   border: 1px solid rgba(255,140,58,0.5); "
        "}"
    );
    discordLabel->installEventFilter(this);
    discordLabel->setProperty("clickUrl", "https://discord.gg/JuEzkT4puD");
    
    // Instagram link com imagem clicável
    QLabel* instagramLabel = new QLabel(leftBox);
    QPixmap instagramPix(":/imagens/instagram_icon.png");
    if (instagramPix.isNull()) {
        instagramPix = QPixmap("imagens/instagram_icon.png");
    }
    if (!instagramPix.isNull()) {
        instagramLabel->setPixmap(instagramPix.scaled(18, 18, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        instagramLabel->setText("I");
    }
    instagramLabel->setFixedSize(24, 24);
    instagramLabel->setAlignment(Qt::AlignCenter);
    instagramLabel->setCursor(Qt::PointingHandCursor);
    instagramLabel->setToolTip("Instagram");
    instagramLabel->setStyleSheet(
        "QLabel { "
        "   padding: 4px; "
        "   border-radius: 6px; "
        "   background-color: transparent; "
        "} "
        "QLabel:hover { "
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 rgba(255,140,58,0.2), stop:1 rgba(255,140,58,0.1)); "
        "   border: 1px solid rgba(255,140,58,0.5); "
        "}"
    );
    instagramLabel->installEventFilter(this);
    instagramLabel->setProperty("clickUrl", "https://www.instagram.com/geanswag/");
    
    leftLayout->addWidget(discordLabel, 0, Qt::AlignVCenter);
    leftLayout->addWidget(instagramLabel, 0, Qt::AlignVCenter);
    QLabel* right = new QLabel(QString("v") + QString::fromLatin1(KEYMAPPER_VERSION), footer);
    right->setProperty("role", "headerLink");
    hfooter->addWidget(leftBox, 0, Qt::AlignVCenter|Qt::AlignLeft);
    hfooter->addStretch(1);
    hfooter->addWidget(right, 0, Qt::AlignVCenter|Qt::AlignRight);
    root->addWidget(footer);

    // Page 0
    QWidget* page0 = new QWidget(tabWidget);
    QGridLayout* g0 = new QGridLayout(page0);
    g0->setHorizontalSpacing(6);
    g0->setVerticalSpacing(3);
    g0->setContentsMargins(6, 6, 6, 6);

    QLabel* pg0Title = new QLabel("Configuração Principal", page0);
    pg0Title->setProperty("role", "title");
    editTrigger = new QLineEdit(page0);
    btnSelTrigger = new QPushButton("Selecionar", page0);
    editHold = new QLineEdit(page0);
    btnSelHold = new QPushButton("Selecionar", page0);
    chkOnlyBluestacks = new QCheckBox("Apenas BlueStacks", page0);
    chkOnlyBluestacks->setToolTip("Quando marcado, o mapeamento só funciona no BlueStacks.");
    statusText = new QLabel(page0);
    statusText->setProperty("role", "pill");
    statusText->setAlignment(Qt::AlignCenter);

    QLabel* lblTrigger = new QLabel("Tecla de Ativação:", page0);
    QLabel* lblHold = new QLabel("Tecla de Corrida:", page0);
    editTrigger->setPlaceholderText("Nenhuma tecla selecionada");
    editHold->setPlaceholderText("Nenhuma tecla selecionada");
    editTrigger->setToolTip("Tecla que ativa/desativa o mapeamento durante o jogo.");
    btnSelTrigger->setToolTip("Clique e depois pressione a tecla desejada para ativação.");
    editHold->setToolTip("Tecla adicional que será segurada junto com W quando ativo.");
    btnSelHold->setToolTip("Clique e depois pressione a tecla que deseja segurar.");

    int r0 = 0;
    g0->addWidget(pg0Title, r0++, 0, 1, 3);
    g0->addWidget(lblTrigger, r0, 0);
    g0->addWidget(editTrigger, r0, 1);
    g0->addWidget(btnSelTrigger, r0++, 2);
    g0->setColumnStretch(1, 1);
    g0->addWidget(lblHold, r0, 0);
    g0->addWidget(editHold, r0, 1);
    g0->addWidget(btnSelHold, r0++, 2);
    g0->setColumnStretch(1, 1);
    g0->addWidget(chkOnlyBluestacks, r0++, 0, 1, 3);
    g0->addWidget(statusText, r0++, 0, 1, 3);

    tabWidget->addTab(page0, "Configurações");

    // Page 1
    QWidget* page1 = new QWidget(tabWidget);
    QGridLayout* g1 = new QGridLayout(page1);
    g1->setHorizontalSpacing(6);
    g1->setVerticalSpacing(2);
    g1->setContentsMargins(6, 6, 6, 6);

    QLabel* pg1Title1 = new QLabel("Teclas de Pulo", page1);
    pg1Title1->setProperty("role", "titleBlue");
    editJumpPhys = new QLineEdit(page1);
    btnSelJumpPhys = new QPushButton("Selecionar", page1);
    editJumpVirt = new QLineEdit(page1);
    btnSelJumpVirt = new QPushButton("Selecionar", page1);
    QLabel* lblJumpPhys = new QLabel("Tecla Física:", page1);
    QLabel* lblJumpVirt = new QLabel("Tecla Virtual:", page1);
    editJumpPhys->setPlaceholderText("Nenhuma tecla selecionada");
    editJumpVirt->setPlaceholderText("Nenhuma tecla selecionada");
    editJumpPhys->setToolTip("Tecla do seu teclado que será detectada como comando de Pulo.");
    editJumpVirt->setToolTip("Tecla enviada ao jogo quando o Pulo for acionado.");
    btnSelJumpPhys->setToolTip("Selecione a tecla física para Pulo.");
    btnSelJumpVirt->setToolTip("Selecione a tecla virtual que o app enviará para Pulo.");

    int r1 = 0;
    g1->addWidget(pg1Title1, r1++, 0, 1, 3);
    g1->addWidget(lblJumpPhys, r1, 0); g1->addWidget(editJumpPhys, r1, 1); g1->addWidget(btnSelJumpPhys, r1++, 2);
    g1->addWidget(lblJumpVirt, r1, 0); g1->addWidget(editJumpVirt, r1, 1); g1->addWidget(btnSelJumpVirt, r1++, 2);
    g1->setColumnStretch(1, 1);

    QLabel* pg1Title2 = new QLabel("Teclas de Agachar", page1);
    pg1Title2->setProperty("role", "titleOrange");
    g1->addWidget(pg1Title2, r1++, 0, 1, 3);
    editCrouchPhys = new QLineEdit(page1);
    btnSelCrouchPhys = new QPushButton("Selecionar", page1);
    editCrouchVirt = new QLineEdit(page1);
    btnSelCrouchVirt = new QPushButton("Selecionar", page1);
    QLabel* lblCrouchPhys = new QLabel("Tecla Física:", page1);
    QLabel* lblCrouchVirt = new QLabel("Tecla Virtual:", page1);
    editCrouchPhys->setPlaceholderText("Nenhuma tecla selecionada");
    editCrouchVirt->setPlaceholderText("Nenhuma tecla selecionada");
    editCrouchPhys->setToolTip("Tecla do seu teclado que será detectada como comando de Agachar.");
    editCrouchVirt->setToolTip("Tecla enviada ao jogo quando o Agachar for acionado.");
    btnSelCrouchPhys->setToolTip("Selecione a tecla física para Agachar.");
    btnSelCrouchVirt->setToolTip("Selecione a tecla virtual que o app enviará para Agachar.");

    g1->addWidget(lblCrouchPhys, r1, 0); g1->addWidget(editCrouchPhys, r1, 1); g1->addWidget(btnSelCrouchPhys, r1++, 2);
    g1->addWidget(lblCrouchVirt, r1, 0); g1->addWidget(editCrouchVirt, r1, 1); g1->addWidget(btnSelCrouchVirt, r1++, 2);

    QLabel* pg1Title3 = new QLabel("Tecla para Troca de Arma", page1);
    pg1Title3->setProperty("role", "titlePurple");
    g1->addWidget(pg1Title3, r1++, 0, 1, 3);
    editWeaponSwap = new QLineEdit(page1);
    btnSelWeaponSwap = new QPushButton("Selecionar", page1);
    QLabel* lblWeaponSwap = new QLabel("Tecla:", page1);
    editWeaponSwap->setPlaceholderText("Nenhuma tecla selecionada");
    editWeaponSwap->setToolTip("Tecla que reapertar a última arma (1, 2, 3 ou 4) pressionada.");
    btnSelWeaponSwap->setToolTip("Selecione a tecla que irá trocar para a última arma.");
    g1->addWidget(lblWeaponSwap, r1, 0); g1->addWidget(editWeaponSwap, r1, 1); g1->addWidget(btnSelWeaponSwap, r1++, 2);

    tabWidget->addTab(page1, "Teclas Extras");

    // Page 2
    QWidget* page2 = new QWidget(tabWidget);
    QVBoxLayout* v2 = new QVBoxLayout(page2);
    v2->setContentsMargins(6, 6, 6, 6);
    v2->setSpacing(4);
    QLabel* pg2Title = new QLabel("Configurações do Sistema", page2);
    pg2Title->setProperty("role", "titlePurple");
    v2->addWidget(pg2Title);

    chkStartWin = new QCheckBox("Iniciar com Windows", page2);
    chkStartMin = new QCheckBox("Iniciar minimizado na bandeja", page2);
    chkStartWin->setToolTip("Cria entrada no Registro para iniciar junto com o Windows.");
    chkStartMin->setToolTip("Ao iniciar, a janela não aparece; o ícone fica apenas na bandeja.");

    v2->addWidget(chkStartWin);

    QHBoxLayout* hmin = new QHBoxLayout();
    hmin->addWidget(chkStartMin);
    QLabel* hintMin = new QLabel("(Inicia direto na bandeja sem mostrar janela)", page2);
    hintMin->setProperty("role", "hint");
    hmin->addWidget(hintMin);
    hmin->addStretch(1);
    v2->addLayout(hmin);
    v2->addStretch(1);
    tabWidget->addTab(page2, "Sistema");

    // Page 3 - Tweakers
    QWidget* page3 = new QWidget(tabWidget);
    QVBoxLayout* v3 = new QVBoxLayout(page3);
    v3->setContentsMargins(6, 6, 6, 6);
    v3->setSpacing(4);
    QLabel* pg3Title = new QLabel("Tweakers", page3);
    pg3Title->setProperty("role", "titlePurple");
    v3->addWidget(pg3Title);
    QHBoxLayout* h3 = new QHBoxLayout();
    QLabel* desc3 = new QLabel("Aprimorar Delay", page3);
    QPushButton* btnApplyTweaks = new QPushButton("Aplicar", page3);
    h3->addWidget(desc3, 1);
    h3->addWidget(btnApplyTweaks, 0, Qt::AlignLeft);
    v3->addLayout(h3);

    // Novo: Aprimorar Sensi (Tweaks -> 714, apenas primeira ocorrência)
    QHBoxLayout* h3b = new QHBoxLayout();
    QLabel* desc3b = new QLabel("Aprimorar Sensi", page3);
    QPushButton* btnApplySensi = new QPushButton("Aplicar", page3);
    h3b->addWidget(desc3b, 1);
    h3b->addWidget(btnApplySensi, 0, Qt::AlignLeft);
    v3->addLayout(h3b);

    // Novo: Mouse Fix (aplica .reg nas chaves do mouse)
    QHBoxLayout* h3c = new QHBoxLayout();
    QLabel* desc3c = new QLabel("Mouse Fix", page3);
    QPushButton* btnMouseFix = new QPushButton("Aplicar", page3);
    h3c->addWidget(desc3c, 1);
    h3c->addWidget(btnMouseFix, 0, Qt::AlignLeft);
    v3->addLayout(h3c);
    v3->addStretch(1);
    tabWidget->addTab(page3, "Tweakers");

    // Signals
    auto wireCapture = [this](QLineEdit* edit, QPushButton* btn){
        connect(btn, &QPushButton::clicked, this, [this, edit]{ startCapture(edit); });
        edit->setReadOnly(true);
    };
    wireCapture(editTrigger, btnSelTrigger);
    wireCapture(editHold, btnSelHold);
    wireCapture(editJumpPhys, btnSelJumpPhys);
    wireCapture(editJumpVirt, btnSelJumpVirt);
    wireCapture(editCrouchPhys, btnSelCrouchPhys);
    wireCapture(editCrouchVirt, btnSelCrouchVirt);
    wireCapture(editWeaponSwap, btnSelWeaponSwap);
    
    // Adicionar animações de bounce aos botões
    addButtonAnimation(btnSelTrigger);
    addButtonAnimation(btnSelHold);
    addButtonAnimation(btnSelJumpPhys);
    addButtonAnimation(btnSelJumpVirt);
    addButtonAnimation(btnSelCrouchPhys);
    addButtonAnimation(btnSelCrouchVirt);
    addButtonAnimation(btnSelWeaponSwap);
    addButtonAnimation(btnApplyTweaks);
    addButtonAnimation(btnApplySensi);
    addButtonAnimation(btnMouseFix);
    
    // Adicionar animações de foco aos campos de texto
    addFocusAnimation(editTrigger);
    addFocusAnimation(editHold);
    addFocusAnimation(editJumpPhys);
    addFocusAnimation(editJumpVirt);
    addFocusAnimation(editCrouchPhys);
    addFocusAnimation(editCrouchVirt);
    addFocusAnimation(editWeaponSwap);
    
    // Animação de fade nas tabs ao trocar
    connect(tabWidget, &QTabWidget::currentChanged, this, [this](int index) {
        QWidget* currentTab = tabWidget->widget(index);
        if (currentTab) {
            QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect(currentTab);
            currentTab->setGraphicsEffect(effect);
            
            QPropertyAnimation* fadeIn = new QPropertyAnimation(effect, "opacity");
            fadeIn->setDuration(250);
            fadeIn->setStartValue(0.0);
            fadeIn->setEndValue(1.0);
            fadeIn->setEasingCurve(QEasingCurve::InOutQuad);
            
            connect(fadeIn, &QPropertyAnimation::finished, currentTab, [currentTab]() {
                currentTab->setGraphicsEffect(nullptr);
            });
            
            fadeIn->start(QAbstractAnimation::DeleteWhenStopped);
        }
    });

    applyMonospaceToKeyFields();

    // Aplicar imediatamente a restrição quando a checkbox mudar
    connect(chkOnlyBluestacks, &QCheckBox::toggled, this, [this](bool checked){
        cfg.only_bluestacks = checked;
        SaveConfig(cfg);
        set_restrict_to_emulators(checked ? 1 : 0);
        // Garantir que o hook use a configuração mais recente
        applyHookConfig();
    });

#ifdef _WIN32
    // Aplicar imediatamente o início com Windows (schtasks) ao alternar
    connect(chkStartWin, &QCheckBox::toggled, this, [this](bool checked){
        cfg.start_with_windows = checked;
        SaveConfig(cfg);
        SetRunAtStartup(checked);
    });
#endif
    // Salvar imediatamente a preferência de iniciar minimizado
    connect(chkStartMin, &QCheckBox::toggled, this, [this](bool checked){
        cfg.start_minimized = checked;
        SaveConfig(cfg);
    });
#ifdef _WIN32
    connect(btnApplyTweaks, &QPushButton::clicked, this, [this]{ applyBluestacksTweaks(); });
    connect(btnApplySensi, &QPushButton::clicked, this, [this]{ applyBluestacksSensi(); });
    connect(btnMouseFix, &QPushButton::clicked, this, [this]{ applyMouseFix(); });
#else
    connect(btnApplyTweaks, &QPushButton::clicked, this, [this]{ QMessageBox::warning(this, "Tweakers", "Disponível apenas no Windows."); });
#endif
}
#ifdef _WIN32
void SettingsWindow::applyBluestacksTweaks() {
    // Caminho base do BlueStacks
    wchar_t* programData = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &programData) != S_OK || !programData) {
        QMessageBox::critical(this, "Tweakers", "Não foi possível localizar %ProgramData%.");
        return;
    }
    QString programDataQ = QString::fromWCharArray(programData);
    CoTaskMemFree(programData);

    auto replaceExclusiveDelay = [](const std::wstring& pathW) -> int {
        QString path = QString::fromWCharArray(pathW.c_str());
        QFile f(path);
        if (!f.exists()) return 0; // não existe
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1; // erro
        QTextStream in(&f);
        QString content = in.readAll();
        f.close();

        // Substitui valores numéricos após ExclusiveDelay preservando espaços
        QRegularExpression re("(\\\"ExclusiveDelay\\\"\\s*:\\s*)\\d+");
        QString replaced = content;
        replaced.replace(re, "\\1" "2");

        if (replaced == content) {
            // Sem mudança, mas arquivo existe
            return 2;
        }

        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return -1;
        QTextStream out(&f);
        out << replaced;
        f.close();
        return 1; // alterado com sucesso
    };

    // Candidatos comuns (BS5 NXT e BS5 legado)
    QStringList candidates;
    candidates << programDataQ + "\\BlueStacks_nxt\\Engine\\UserData\\InputMapper\\com.dts.freefiremax.cfg";
    candidates << programDataQ + "\\BlueStacks_nxt\\Engine\\UserData\\InputMapper\\UserFiles\\com.dts.freefiremax.cfg";
    candidates << programDataQ + "\\BlueStacks\\Engine\\UserData\\InputMapper\\com.dts.freefiremax.cfg";
    candidates << programDataQ + "\\BlueStacks\\Engine\\UserData\\InputMapper\\UserFiles\\com.dts.freefiremax.cfg";

    int changed = 0, unchanged = 0, missing = 0, errors = 0;
    for (const QString& qpath : candidates) {
        int r = replaceExclusiveDelay(qpath.toStdWString());
        if (r == 1) ++changed;
        else if (r == 2) ++unchanged; // já estava 2
        else if (r == 0) ++missing;   // arquivo não existe
        else if (r == -1) ++errors;   // erro IO
    }

    if (changed > 0) {
        QMessageBox::information(this, "Tweakers", QString("ExclusiveDelay ajustado para 2 em %1 arquivo(s). Reinicie o BlueStacks.").arg(changed));
        return;
    }
    if (unchanged > 0) {
        QMessageBox::information(this, "Tweakers", "ExclusiveDelay já estava em 2.");
        return;
    }
    if (errors > 0) {
        QMessageBox::critical(this, "Tweakers", "Falha ao abrir/gravar o arquivo. Execute como administrador.");
        return;
    }
    QMessageBox::warning(this, "Tweakers", "Arquivo com.dts.freefiremax.cfg não encontrado nos diretórios do BlueStacks.");
}

void SettingsWindow::applyMonospaceToKeyFields() {
    QFont monoFont("Cascadia Mono, Consolas, 'Courier New'", 11);
    monoFont.setStyleHint(QFont::Monospace);
    monoFont.setFixedPitch(true);
    const QList<QLineEdit*> edits = { editTrigger, editHold, editJumpPhys, editJumpVirt, editCrouchPhys, editCrouchVirt };
    for (QLineEdit* e : edits) if (e) e->setFont(monoFont);
}

void SettingsWindow::applyBluestacksSensi() {
    // Localiza %ProgramData%
    wchar_t* programData = nullptr;
    if (SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &programData) != S_OK || !programData) {
        QMessageBox::critical(this, "Tweakers", "Não foi possível localizar %ProgramData%.");
        return;
    }
    QString programDataQ = QString::fromWCharArray(programData);
    CoTaskMemFree(programData);

    // Candidatos comuns
    QStringList candidates;
    candidates << programDataQ + "\\BlueStacks_nxt\\Engine\\UserData\\InputMapper\\com.dts.freefiremax.cfg";
    candidates << programDataQ + "\\BlueStacks_nxt\\Engine\\UserData\\InputMapper\\UserFiles\\com.dts.freefiremax.cfg";
    candidates << programDataQ + "\\BlueStacks\\Engine\\UserData\\InputMapper\\com.dts.freefiremax.cfg";
    candidates << programDataQ + "\\BlueStacks\\Engine\\UserData\\InputMapper\\UserFiles\\com.dts.freefiremax.cfg";

    auto tweakFirstTweaks = [](const QString& path) -> int {
        QFile f(path);
        if (!f.exists()) return 0; // não existe
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return -1; // erro IO
        QString content = QTextStream(&f).readAll();
        f.close();

        // Encontrar a primeira ocorrência de "Tweaks" : <numero> e trocar por 714
        QRegularExpression re("\\\"Tweaks\\\"\\s*:\\s*(\\d+)");
        QRegularExpressionMatch m = re.match(content);
        if (!m.hasMatch()) return 2; // sem ocorrências

        // Trocar somente a primeira ocorrência
        int start = m.capturedStart(1);
        int len = m.capturedLength(1);
        QString updated = content;
        updated.replace(start, len, "714");

        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return -1;
        QTextStream out(&f);
        out << updated;
        f.close();
        return 1; // alterado
    };

    int changed = 0, missing = 0, errors = 0, unchanged = 0;
    for (const QString& qpath : candidates) {
        int r = tweakFirstTweaks(qpath);
        if (r == 1) { ++changed; }
        else if (r == 0) { ++missing; }
        else if (r == -1) { ++errors; }
        else if (r == 2) { ++unchanged; } // sem ocorrências (incomum)
    }

    if (changed > 0) {
        QMessageBox::information(this, "Tweakers", QString("Sensi aprimorada (Tweaks=714) em %1 arquivo(s). Reinicie o BlueStacks.").arg(changed));
        return;
    }
    if (errors > 0) {
        QMessageBox::critical(this, "Tweakers", "Falha ao abrir/gravar arquivo. Execute como administrador.");
        return;
    }
    QMessageBox::warning(this, "Tweakers", "Arquivo não encontrado nos diretórios do BlueStacks.");
}

void SettingsWindow::applyMouseFix() {
    // Conteúdo do .reg fornecido
    static const char* regText =
        "Windows Registry Editor Version 5.00\r\n"
        "; Windows_10+8.x_MouseFix_ItemsSize=100%_Scale=1-to-1_@4-of-11\r\n\r\n"
        "[HKEY_CURRENT_USER\\Control Panel\\Mouse]\r\n\r\n"
        "\"MouseSensitivity\"=\"6\"\r\n"
        "\"SmoothMouseXCurve\"=hex:\\\r\n"
        "\t00,00,00,00,00,00,00,00,\\\r\n"
        "\t04,AE,07,00,00,00,00,00,\\\r\n"
        "\t08,5C,0F,00,00,00,00,00,\\\r\n"
        "\t0C,0A,17,00,00,00,00,00,\\\r\n"
        "\t10,B8,1E,00,00,00,00,00\r\n"
        "\"SmoothMouseYCurve\"=hex:\\\r\n"
        "\t00,00,00,00,00,00,00,00,\\\r\n"
        "\tF9,FF,37,00,00,00,00,00,\\\r\n"
        "\tF2,FF,6F,00,00,00,00,00,\\\r\n"
        "\tEB,FF,A7,00,00,00,00,00,\\\r\n"
        "\tE4,FF,DF,00,00,00,00,00\r\n\r\n"
        "[HKEY_USERS\\.DEFAULT\\Control Panel\\Mouse]\r\n\r\n"
        "\"MouseSpeed\"=\"0\"\r\n"
        "\"MouseThreshold1\"=\"0\"\r\n"
        "\"MouseThreshold2\"=\"0\"\r\n";

    // Gravar .reg temporário e aplicar com reg.exe import
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) == 0) {
        QMessageBox::critical(this, "Mouse Fix", "Não foi possível obter o diretório temporário.");
        return;
    }
    std::wstring regPath = std::wstring(tempPath) + L"keymapper_mouse_fix.reg";

    HANDLE h = CreateFileW(regPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        QMessageBox::critical(this, "Mouse Fix", "Falha ao criar arquivo temporário .reg.");
        return;
    }
    DWORD written = 0;
    BOOL ok = WriteFile(h, regText, (DWORD)strlen(regText), &written, NULL);
    CloseHandle(h);
    if (!ok) {
        QMessageBox::critical(this, "Mouse Fix", "Falha ao gravar arquivo .reg.");
        return;
    }

    // Executar: reg.exe import "arquivo"
    SHELLEXECUTEINFOW sei{}; sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.hwnd = (HWND)winId();
    sei.lpVerb = L"runas"; // tentar elevar
    sei.lpFile = L"reg.exe";
    std::wstring params = L"import \"" + regPath + L"\"";
    sei.lpParameters = params.c_str();
    sei.nShow = SW_HIDE;
    if (!ShellExecuteExW(&sei)) {
        QMessageBox::critical(this, "Mouse Fix", "Falha ao executar reg.exe. Tente executar como administrador.");
        return;
    }
    WaitForSingleObject(sei.hProcess, INFINITE);
    DWORD exitCode = 1; GetExitCodeProcess(sei.hProcess, &exitCode);
    CloseHandle(sei.hProcess);

    if (exitCode == 0) {
        QMessageBox::information(this, "Mouse Fix", "Mouse Fix aplicado. Faça logoff ou reinicie para garantir.");
    } else {
        QMessageBox::critical(this, "Mouse Fix", "Falha ao aplicar o Mouse Fix. Execute como administrador.");
    }
}
#endif
void SettingsWindow::setupTray() {
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return; // Sistema não suporta bandeja
    }
    
    // Tentar múltiplos caminhos para o ícone
    QIcon icon;
    QStringList iconPaths = {
        ":/imagens/logo.ico",
        ":/imagens/logo.png",
        "imagens/logo.ico",
        "imagens/logo.png",
        "../imagens/logo.ico",
        "../imagens/logo.png"
    };
    
    for (const QString& path : iconPaths) {
        icon = QIcon(path);
        if (!icon.isNull() && !icon.pixmap(16, 16).isNull()) {
            break; // Encontrou um ícone válido
        }
    }
    
    // Se ainda não carregou, usar o ícone padrão da janela
    if (icon.isNull() || icon.pixmap(16, 16).isNull()) {
        icon = windowIcon();
    }
    
    // Último fallback: criar ícone simples vermelho (para debug)
    if (icon.isNull() || icon.pixmap(16, 16).isNull()) {
        QPixmap pix(16, 16);
        pix.fill(Qt::red);
        icon = QIcon(pix);
    }
    
    trayIcon = new QSystemTrayIcon(icon, this);
    trayMenu = new QMenu(this);
    QAction* openAct = trayMenu->addAction("Open");
    QAction* exitAct = trayMenu->addAction("Exit");
    connect(openAct, &QAction::triggered, this, [this]{ this->showNormal(); this->activateWindow(); });
    connect(exitAct, &QAction::triggered, qApp, &QApplication::quit);
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setToolTip("KeyMapper");
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason){
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            this->showNormal();
            this->activateWindow();
        }
    });
    
    // Garantir que o ícone seja exibido e sempre visível
    trayIcon->setVisible(true);
    
    // No Windows, garantir que o ícone seja mostrado na bandeja principal
    // Isso força o Windows a mostrar o ícone mesmo se outros ícones estão no overflow
    trayIcon->show();
    
    // Verificar se realmente está visível após alguns milissegundos
    // Isso ajuda o Windows a reconhecer o ícone e mantê-lo na bandeja principal
    QTimer::singleShot(100, this, [this]() {
        if (trayIcon) {
            if (!trayIcon->isVisible()) {
                trayIcon->show();
            }
            // Atualizar o ícone para "ativar" na bandeja do Windows
            trayIcon->setIcon(trayIcon->icon());
        }
    });
    
    // Segunda verificação após um tempo maior para garantir persistência
    QTimer::singleShot(1000, this, [this]() {
        if (trayIcon && !trayIcon->isVisible()) {
            trayIcon->show();
        }
    });

    // Lógica de visibilidade movida para qt_main.cpp para evitar duplicação
}

void SettingsWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            // Sempre minimizar para bandeja
            QTimer::singleShot(0, this, [this]{ this->hide(); });
        }
    }
    QMainWindow::changeEvent(event);
}

void SettingsWindow::loadToControls() {
    editTrigger->setText(QString::fromStdString(cfg.trigger_key));
    editHold->setText(QString::fromStdString(cfg.hold_key));
    editJumpPhys->setText(QString::fromStdString(cfg.jump_physical_key));
    editJumpVirt->setText(QString::fromStdString(cfg.jump_virtual_key));
    editCrouchPhys->setText(QString::fromStdString(cfg.crouch_physical_key));
    editCrouchVirt->setText(QString::fromStdString(cfg.crouch_virtual_key));
    editWeaponSwap->setText(QString::fromStdString(cfg.weapon_swap_key));
    chkStartWin->setChecked(cfg.start_with_windows);
    chkStartMin->setChecked(cfg.start_minimized);
    if (chkOnlyBluestacks) chkOnlyBluestacks->setChecked(cfg.only_bluestacks);
    statusText->setText("Mapeamento ativo! Pressione a tecla de ativação para alternar.");
}

void SettingsWindow::saveFromControls() {
    cfg.trigger_key = editTrigger->text().toStdString();
    cfg.hold_key = editHold->text().toStdString();
    cfg.jump_physical_key = editJumpPhys->text().toStdString();
    cfg.jump_virtual_key = editJumpVirt->text().toStdString();
    cfg.crouch_physical_key = editCrouchPhys->text().toStdString();
    cfg.crouch_virtual_key = editCrouchVirt->text().toStdString();
    cfg.weapon_swap_key = editWeaponSwap->text().toStdString();
    cfg.start_with_windows = chkStartWin->isChecked();
    cfg.start_minimized = chkStartMin->isChecked();
    if (chkOnlyBluestacks) cfg.only_bluestacks = chkOnlyBluestacks->isChecked();
    SaveConfig(cfg);
    // Reaplicar no hook sempre que salvar
    applyHookConfig();

#ifdef _WIN32
    SetRunAtStartup(cfg.start_with_windows);
#endif
}

void SettingsWindow::applyHookConfig() {
#ifdef _WIN32
    // Reconfigurar teclas e flags no hook para refletir mudança imediata
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
    set_restrict_to_emulators(cfg.only_bluestacks ? 1 : 0);
#endif
}

QString SettingsWindow::keyToString(int key, Qt::KeyboardModifiers mods) const {
    QString s;
    
    // Tratar teclas especiais primeiro para evitar duplicação
    if (key == Qt::Key_Shift) {
        return "SHIFT";
    }
    if (key == Qt::Key_Control) {
        return "CTRL";
    }
    if (key == Qt::Key_Alt) {
        return "ALT";
    }
    if (key == Qt::Key_Tab) {
        return "TAB";
    }
    if (key == Qt::Key_Escape) {
        return "ESC";
    }
    if (key == Qt::Key_Backspace) {
        return "BACKSPACE";
    }
    if (key == Qt::Key_Delete) {
        return "DELETE";
    }
    if (key == Qt::Key_Return || key == Qt::Key_Enter) {
        return "ENTER";
    }
    if (key == Qt::Key_Space) {
        return "SPACE";
    }
    
    // Para teclas de função
    if (key >= Qt::Key_F1 && key <= Qt::Key_F24) {
        return QString("F%1").arg(key - Qt::Key_F1 + 1);
    }
    
    // Para teclas de direção
    if (key == Qt::Key_Up) return "UP";
    if (key == Qt::Key_Down) return "DOWN";
    if (key == Qt::Key_Left) return "LEFT";
    if (key == Qt::Key_Right) return "RIGHT";
    
    // Para teclas numéricas
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return QString::number(key - Qt::Key_0);
    }
    
    // Para teclas alfabéticas
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return QString(QChar(key));
    }
    
    // Para combinações com modificadores (apenas quando não for a própria tecla modificadora)
    if (mods & Qt::ControlModifier && key != Qt::Key_Control) s += "CTRL+";
    if (mods & Qt::AltModifier && key != Qt::Key_Alt) s += "ALT+";
    if (mods & Qt::ShiftModifier && key != Qt::Key_Shift) s += "SHIFT+";
    
    // Adicionar a tecla base
    QString keyText = QKeySequence(key).toString(QKeySequence::NativeText);
    if (!keyText.isEmpty()) {
        s += keyText;
    } else {
        // Fallback para teclas não reconhecidas
        s += QString("Key_%1").arg(key);
    }
    
    return s;
}

void SettingsWindow::startCapture(QLineEdit* target) {
    captureTarget = target;
    statusText->setText("Pressione uma tecla...");
    
    // Instalar filtro de eventos para capturar todas as teclas
    // Capturar no nível do aplicativo para interceptar TAB antes da navegação de foco
    qApp->installEventFilter(this);
    // Garantir que a janela receba eventos de teclado prioritariamente
    this->grabKeyboard();
    
    #ifdef _WIN32
    // Verificação de proteção durante captura
    if (ANTI_DEBUG_CHECK()) {
        JUNK_CODE();
        stopCapture();
        return;
    }
    #endif
}

void SettingsWindow::stopCapture() {
    captureTarget = nullptr;
    statusText->setText("Mapeamento ativo! Pressione a tecla de ativação para alternar.");
    
    // Remover filtro de eventos
    qApp->removeEventFilter(this);
    this->releaseKeyboard();

    // Persistir alterações e aplicar no hook imediatamente
    saveFromControls();
}

bool SettingsWindow::eventFilter(QObject* obj, QEvent* event) {
    if (captureTarget && (event->type() == QEvent::KeyPress || event->type() == QEvent::ShortcutOverride)) {
        #ifdef _WIN32
        // Verificação de proteção
        if (ANTI_DEBUG_CHECK()) {
            JUNK_CODE();
            stopCapture();
            return true;
        }
        #endif
        
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        // Consumir TAB antes de mudar foco
        if (keyEvent->key() == Qt::Key_Tab) {
            captureTarget->setText("TAB");
            stopCapture();
            return true;
        }
        
        // Tratamento especial para teclas modificadoras
        if (keyEvent->key() == Qt::Key_Shift) {
            captureTarget->setText("SHIFT");
            stopCapture();
            return true;
        }
        
        if (keyEvent->key() == Qt::Key_Control) {
            captureTarget->setText("CTRL");
            stopCapture();
            return true;
        }
        
        if (keyEvent->key() == Qt::Key_Alt) {
            captureTarget->setText("ALT");
            stopCapture();
            return true;
        }
        
        // Para outras teclas, usar a função keyToString
        QString txt = keyToString(keyEvent->key(), keyEvent->modifiers());
        captureTarget->setText(txt);
        stopCapture();
        return true; // Consumir o evento
    }
    // Captura de botões laterais do mouse durante seleção
    if (captureTarget && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* me = static_cast<QMouseEvent*>(event);
        if (me->button() == Qt::ExtraButton1) {
            captureTarget->setText("MOUSE4");
            stopCapture();
            return true;
        }
        if (me->button() == Qt::ExtraButton2) {
            captureTarget->setText("MOUSE5");
            stopCapture();
            return true;
        }
    }
    
    // Clique em imagens sociais (Discord/Instagram)
    if (event->type() == QEvent::MouseButtonPress) {
        QLabel* label = qobject_cast<QLabel*>(obj);
        if (label && label->property("clickUrl").isValid()) {
            QString url = label->property("clickUrl").toString();
            if (!url.isEmpty()) {
                QDesktopServices::openUrl(QUrl(url));
                return true;
            }
        }
    }
    
    // Para eventos que não são de teclado ou quando não estamos capturando
    return QMainWindow::eventFilter(obj, event);
}

void SettingsWindow::keyPressEvent(QKeyEvent* event) {
    if (!captureTarget) {
        QMainWindow::keyPressEvent(event);
        return;
    }
    
    // O eventFilter já está tratando todas as teclas durante a captura
    QMainWindow::keyPressEvent(event);
}

void SettingsWindow::closeEvent(QCloseEvent* event) {
    saveFromControls();
    if (chkStartMin && chkStartMin->isChecked()) {
        hide();
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void SettingsWindow::bringToFront() {
    if (isMinimized()) showNormal();
    show();
    raise();
    activateWindow();
}


