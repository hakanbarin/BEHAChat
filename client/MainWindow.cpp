// ═══════════════════════════════════════════════════════════════════════════
//                         ANA PENCERE IMPLEMENTASYONU
// MainWindow sınıfının tüm metodlarının implementasyonu
// ═══════════════════════════════════════════════════════════════════════════

#include "MainWindow.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QMessageBox>
#include <QCheckBox>
#include <QDateTime>
#include <QScrollBar>
#include <QDebug>
#include <QApplication>

// ═══════════════════════════════════════════════════════════════════════════
//                         CONSTRUCTOR VE DESTRUCTOR
// ═══════════════════════════════════════════════════════════════════════════

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      tcpSocket(nullptr),
      currentPermission(ClientPermission::BANNED),
      isConnected(false)
{
    // Pencere başlığı ve boyutu
    setWindowTitle("🔐 Secure Chat Client v2.0");
    setMinimumSize(900, 700);
    resize(1100, 800);
    
    // UI bileşenlerini oluştur
    setupUi();
    
    // Stilleri uygula
    setupStyles();
    
    // TCP soket oluştur
    tcpSocket = new QTcpSocket(this);
    
    // TCP sinyallerini bağla
    connect(tcpSocket, &QTcpSocket::connected, this, &MainWindow::onTcpConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &MainWindow::onTcpDisconnected);
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onTcpReadyRead);
    connect(tcpSocket, &QTcpSocket::errorOccurred, this, &MainWindow::onTcpError);
    
    // Durum çubuğu mesajı
    statusBar()->showMessage("Hoş geldiniz! Giriş yapın.");
}

MainWindow::~MainWindow()
{
    // Bağlantıları kapat
    if (tcpSocket && tcpSocket->isOpen())
    {
        tcpSocket->close();
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         UI OLUŞTURMA METODLARI
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::setupUi()
{
    // Ana merkezi widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Ana layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Stacked widget - sayfalar arası geçiş için
    stackedWidget = new QStackedWidget();
    mainLayout->addWidget(stackedWidget);
    
    // Sayfaları oluştur
    setupLoginPage();
    setupChatPage();
    
    // İlk sayfa: Login
    stackedWidget->setCurrentWidget(loginPage);
}

// ─────────────────────────────────────────────────────────────────────────
// GİRİŞ SAYFASI
// ─────────────────────────────────────────────────────────────────────────
void MainWindow::setupLoginPage()
{
    loginPage = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(loginPage);
    layout->setAlignment(Qt::AlignCenter);
    
    // ═══════════════════════════════════════════════════════════════════
    // BAŞLIK
    // ═══════════════════════════════════════════════════════════════════
    QLabel* titleLabel = new QLabel("Secure Chat");
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);
    
    QLabel* subtitleLabel = new QLabel("Güvenli Mesajlaşma Platformu");
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitleLabel);
    
    layout->addSpacing(30);
    
    // ═══════════════════════════════════════════════════════════════════
    // GİRİŞ FORMU
    // ═══════════════════════════════════════════════════════════════════
    QGroupBox* loginGroup = new QGroupBox("Giriş Bilgileri");
    loginGroup->setObjectName("loginGroup");
    loginGroup->setMaximumWidth(400);
    
    QFormLayout* formLayout = new QFormLayout(loginGroup);
    formLayout->setSpacing(15);
    
    // Sunucu adresi
    serverAddressEdit = new QLineEdit("localhost");
    serverAddressEdit->setPlaceholderText("Sunucu IP adresi");
    formLayout->addRow("📡 Sunucu:", serverAddressEdit);
    
    // Port ayarları (yan yana)
    QHBoxLayout* portLayout = new QHBoxLayout();
    grpcPortSpin = new QSpinBox();
    grpcPortSpin->setRange(1, 65535);
    grpcPortSpin->setValue(50051);
    grpcPortSpin->setMinimumWidth(120);  // Tam görünsün
    portLayout->addWidget(grpcPortSpin);
    
    tcpPortSpin = new QSpinBox();
    tcpPortSpin->setRange(1, 65535);
    tcpPortSpin->setValue(5000);
    tcpPortSpin->setMinimumWidth(100);
    portLayout->addWidget(tcpPortSpin);
    
    QWidget* portWidget = new QWidget();
    portWidget->setLayout(portLayout);
    formLayout->addRow("🔌 Portlar:", portWidget);
    
    // Kullanıcı adı
    usernameEdit = new QLineEdit();
    usernameEdit->setPlaceholderText("Kullanıcı adınızı girin");
    formLayout->addRow("👤 Kullanıcı:", usernameEdit);
    
    // Şifre
    passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setPlaceholderText("Şifrenizi girin");
    formLayout->addRow("🔑 Şifre:", passwordEdit);

    // E-posta (opsiyonel)
    emailEdit = new QLineEdit();
    emailEdit->setPlaceholderText("E-posta (opsiyonel)");
    formLayout->addRow("✉️ E-posta:", emailEdit);
    
    layout->addWidget(loginGroup, 0, Qt::AlignCenter);
    
    // ═══════════════════════════════════════════════════════════════════
    // GİRİŞ BUTONU
    // ═══════════════════════════════════════════════════════════════════
    layout->addSpacing(20);
    
    // Giriş ve Kayıt butonları yan yana
    QHBoxLayout* authButtonsLayout = new QHBoxLayout();

    loginButton = new QPushButton("🚀 Giriş Yap");
    loginButton->setObjectName("loginButton");
    loginButton->setFixedSize(180, 46);
    connect(loginButton, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    authButtonsLayout->addWidget(loginButton);

    registerButton = new QPushButton("📝 Kayıt Ol");
    registerButton->setObjectName("registerButton");
    registerButton->setFixedSize(180, 46);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    authButtonsLayout->addWidget(registerButton);

    QWidget* authButtonsWidget = new QWidget();
    authButtonsWidget->setLayout(authButtonsLayout);
    layout->addWidget(authButtonsWidget, 0, Qt::AlignCenter);
    
    // Enter tuşu ile giriş
    connect(passwordEdit, &QLineEdit::returnPressed, this, &MainWindow::onLoginClicked);
    connect(usernameEdit, &QLineEdit::returnPressed, this, &MainWindow::onLoginClicked);
    
    // ═══════════════════════════════════════════════════════════════════
    // DURUM ETİKETİ
    // ═══════════════════════════════════════════════════════════════════
    loginStatusLabel = new QLabel("");
    loginStatusLabel->setObjectName("loginStatusLabel");
    loginStatusLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(loginStatusLabel);
    
    // ═══════════════════════════════════════════════════════════════════
    // HAZIR KULLANICI BİLGİLERİ (TEST İÇİN)
    // ═══════════════════════════════════════════════════════════════════
    layout->addSpacing(30);
    
    QGroupBox* testUsersGroup = new QGroupBox("📋 Test Kullanıcıları");
    testUsersGroup->setObjectName("testUsersGroup");
    testUsersGroup->setMaximumWidth(400);
    
    QVBoxLayout* testLayout = new QVBoxLayout(testUsersGroup);
    
    QLabel* testInfo = new QLabel(
        "• admin / admin123 → ADMIN\n"
        "• moderator / mod456 → MODERATOR\n"
        "• user / user789 → USER\n"
        "• guest / guest999 → GUEST"
    );
    testInfo->setObjectName("testInfo");
    testLayout->addWidget(testInfo);
    
    layout->addWidget(testUsersGroup, 0, Qt::AlignCenter);
    
    // Sayfayı stackedWidget'a ekle
    stackedWidget->addWidget(loginPage);
}

// ─────────────────────────────────────────────────────────────────────────
// CHAT SAYFASI
// ─────────────────────────────────────────────────────────────────────────
void MainWindow::setupChatPage()
{
    chatPage = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(chatPage);
    
    // ═══════════════════════════════════════════════════════════════════
    // ÜST BAR - KULLANICI BİLGİSİ VE ÇIKIŞ
    // ═══════════════════════════════════════════════════════════════════
    QHBoxLayout* topBar = new QHBoxLayout();
    
    userInfoLabel = new QLabel("👤 Kullanıcı: -");
    userInfoLabel->setObjectName("userInfoLabel");
    topBar->addWidget(userInfoLabel);
    
    topBar->addStretch();
    
    connectionStatusLabel = new QLabel("🔴 Bağlı Değil");
    connectionStatusLabel->setObjectName("connectionStatusLabel");
    topBar->addWidget(connectionStatusLabel);
    
    logoutButton = new QPushButton("🚪 Çıkış");
    logoutButton->setObjectName("logoutButton");
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    topBar->addWidget(logoutButton);
    
    mainLayout->addLayout(topBar);
    
    // ═══════════════════════════════════════════════════════════════════
    // ANA İÇERİK - SPLITTER İLE AYRILMIŞ
    // ═══════════════════════════════════════════════════════════════════
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    
    // ─────────────────────────────────────────────────────────────────
    // SOL TARAF - CHAT ALANI
    // ─────────────────────────────────────────────────────────────────
    QWidget* chatWidget = new QWidget();
    QVBoxLayout* chatLayout = new QVBoxLayout(chatWidget);
    
    // Mesaj görüntüleme alanı
    chatDisplay = new QTextEdit();
    chatDisplay->setReadOnly(true);
    chatDisplay->setObjectName("chatDisplay");
    chatDisplay->setPlaceholderText("Mesajlar burada görünecek...");
    chatLayout->addWidget(chatDisplay);
    
    // Mesaj yazma alanı
    QHBoxLayout* inputLayout = new QHBoxLayout();
    
    messageEdit = new QLineEdit();
    messageEdit->setPlaceholderText("Mesajınızı yazın...");
    messageEdit->setObjectName("messageEdit");
    inputLayout->addWidget(messageEdit);
    
    sendButton = new QPushButton("📤 Gönder");
    sendButton->setObjectName("sendButton");
    connect(sendButton, &QPushButton::clicked, this, &MainWindow::onSendMessageClicked);
    connect(messageEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendMessageClicked);
    inputLayout->addWidget(sendButton);
    
    chatLayout->addLayout(inputLayout);
    
    splitter->addWidget(chatWidget);
    
    // ─────────────────────────────────────────────────────────────────
    // SAĞ TARAF - ADMİN PANELİ
    // ─────────────────────────────────────────────────────────────────
    setupAdminPanel();
    splitter->addWidget(adminPanel);
    
    // Splitter boyutları
    splitter->setSizes({600, 400});
    
    mainLayout->addWidget(splitter);
    
    // Sayfayı stackedWidget'a ekle
    stackedWidget->addWidget(chatPage);
}

// ─────────────────────────────────────────────────────────────────────────
// ADMİN PANELİ
// ─────────────────────────────────────────────────────────────────────────
void MainWindow::setupAdminPanel()
{
    adminPanel = new QGroupBox("🛡️ Yönetim Paneli");
    adminPanel->setObjectName("adminPanel");
    
    QVBoxLayout* panelLayout = new QVBoxLayout(adminPanel);
    
    // Tab widget
    adminTabs = new QTabWidget();
    adminTabs->setObjectName("adminTabs");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 1: KULLANICI LİSTESİ
    // ═══════════════════════════════════════════════════════════════════
    QWidget* usersTab = new QWidget();
    QVBoxLayout* usersLayout = new QVBoxLayout(usersTab);
    
    QLabel* usersTitle = new QLabel("👥 Aktif Kullanıcılar");
    usersTitle->setObjectName("sectionTitle");
    usersLayout->addWidget(usersTitle);
    
    userListWidget = new QListWidget();
    userListWidget->setObjectName("userListWidget");
    usersLayout->addWidget(userListWidget);
    
    refreshUsersButton = new QPushButton("🔄 Yenile");
    connect(refreshUsersButton, &QPushButton::clicked, this, &MainWindow::onRefreshUsersClicked);
    usersLayout->addWidget(refreshUsersButton);
    
    adminTabs->addTab(usersTab, "👥 Kullanıcılar");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 2: YETKİ YÖNETİMİ
    // ═══════════════════════════════════════════════════════════════════
    QWidget* permTab = new QWidget();
    QVBoxLayout* permLayout = new QVBoxLayout(permTab);
    
    QLabel* permTitle = new QLabel("🔐 Yetki Değiştir (Sadece ADMIN)");
    permTitle->setObjectName("sectionTitle");
    permLayout->addWidget(permTitle);
    
    QFormLayout* permForm = new QFormLayout();
    
    // Kullanıcı seçimi için searchable dropdown
    permTargetCombo = new QComboBox();
    permTargetCombo->setEditable(true);  // Yazılabilir yap
    permTargetCombo->setInsertPolicy(QComboBox::NoInsert); // Yeni item ekleme
    permTargetCombo->setPlaceholderText("Kullanıcı seç veya ara...");
    permTargetCombo->setMaxVisibleItems(10); // 10 item göster, kaydırılabilir
    permTargetCombo->lineEdit()->setPlaceholderText("Kullanıcı seç veya ara...");
    permForm->addRow("Hedef:", permTargetCombo);
    
    permLevelCombo = new QComboBox();
    permLevelCombo->addItem("ADMIN", 0);
    permLevelCombo->addItem("MODERATOR", 1);
    permLevelCombo->addItem("USER", 2);
    permLevelCombo->addItem("GUEST", 3);
    permLevelCombo->addItem("BANNED", 4);
    permLevelCombo->setCurrentIndex(2);  // Default: USER
    permForm->addRow("Yeni Yetki:", permLevelCombo);
    
    permLayout->addLayout(permForm);
    
    changePermButton = new QPushButton("✅ Yetkiyi Değiştir");
    changePermButton->setObjectName("adminButton");
    connect(changePermButton, &QPushButton::clicked, this, &MainWindow::onChangePermissionClicked);
    permLayout->addWidget(changePermButton);
    
    permLayout->addStretch();
    
    adminTabs->addTab(permTab, "🔐 Yetki");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 3: BAN İŞLEMLERİ
    // ═══════════════════════════════════════════════════════════════════
    QWidget* banTab = new QWidget();
    QVBoxLayout* banLayout = new QVBoxLayout(banTab);
    
    QLabel* banTitle = new QLabel("🚫 Ban Yönetimi");
    banTitle->setObjectName("sectionTitle");
    banLayout->addWidget(banTitle);
    
    QFormLayout* banForm = new QFormLayout();
    
    // Kullanıcı seçimi için searchable dropdown
    banTargetCombo = new QComboBox();
    banTargetCombo->setEditable(true);
    banTargetCombo->setInsertPolicy(QComboBox::NoInsert);
    banTargetCombo->setPlaceholderText("Kullanıcı seç veya ara...");
    banTargetCombo->setMaxVisibleItems(10);
    banTargetCombo->lineEdit()->setPlaceholderText("Kullanıcı seç veya ara...");
    banForm->addRow("Hedef:", banTargetCombo);
    
    banReasonEdit = new QLineEdit();
    banReasonEdit->setPlaceholderText("Ban sebebi");
    banForm->addRow("Sebep:", banReasonEdit);
    
    banDurationSpin = new QSpinBox();
    banDurationSpin->setRange(0, 10080);  // 0 = kalıcı, max 7 gün
    banDurationSpin->setValue(60);
    banDurationSpin->setSuffix(" dk");
    banDurationSpin->setSpecialValueText("Kalıcı");
    banForm->addRow("Süre:", banDurationSpin);
    
    banLayout->addLayout(banForm);
    
    QHBoxLayout* banButtons = new QHBoxLayout();
    
    banButton = new QPushButton("🚫 Banla");
    banButton->setObjectName("dangerButton");
    connect(banButton, &QPushButton::clicked, this, &MainWindow::onBanUserClicked);
    banButtons->addWidget(banButton);
    
    unbanButton = new QPushButton("✅ Banı Kaldır");
    unbanButton->setObjectName("successButton");
    connect(unbanButton, &QPushButton::clicked, this, &MainWindow::onUnbanUserClicked);
    banButtons->addWidget(unbanButton);
    
    banLayout->addLayout(banButtons);
    banLayout->addStretch();
    
    adminTabs->addTab(banTab, "🚫 Ban");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 4: KICK İŞLEMİ
    // ═══════════════════════════════════════════════════════════════════
    QWidget* kickTab = new QWidget();
    QVBoxLayout* kickLayout = new QVBoxLayout(kickTab);
    
    QLabel* kickTitle = new QLabel("🦵 Kullanıcı At");
    kickTitle->setObjectName("sectionTitle");
    kickLayout->addWidget(kickTitle);
    
    QFormLayout* kickForm = new QFormLayout();
    
    // Kullanıcı seçimi için searchable dropdown
    kickTargetCombo = new QComboBox();
    kickTargetCombo->setEditable(true);
    kickTargetCombo->setInsertPolicy(QComboBox::NoInsert);
    kickTargetCombo->setPlaceholderText("Kullanıcı seç veya ara...");
    kickTargetCombo->setMaxVisibleItems(10);
    kickTargetCombo->lineEdit()->setPlaceholderText("Kullanıcı seç veya ara...");
    kickForm->addRow("Hedef:", kickTargetCombo);
    
    kickReasonEdit = new QLineEdit();
    kickReasonEdit->setPlaceholderText("Atılma sebebi");
    kickForm->addRow("Sebep:", kickReasonEdit);
    
    kickLayout->addLayout(kickForm);
    
    kickButton = new QPushButton("🦵 Kullanıcıyı At");
    kickButton->setObjectName("warningButton");
    connect(kickButton, &QPushButton::clicked, this, &MainWindow::onKickUserClicked);
    kickLayout->addWidget(kickButton);
    
    kickLayout->addStretch();
    
    adminTabs->addTab(kickTab, "🦵 Kick");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 5: BROADCAST
    // ═══════════════════════════════════════════════════════════════════
    QWidget* broadcastTab = new QWidget();
    QVBoxLayout* broadcastLayout = new QVBoxLayout(broadcastTab);
    
    QLabel* broadcastTitle = new QLabel("📢 Duyuru Gönder");
    broadcastTitle->setObjectName("sectionTitle");
    broadcastLayout->addWidget(broadcastTitle);
    
    broadcastEdit = new QLineEdit();
    broadcastEdit->setPlaceholderText("Duyuru mesajı...");
    broadcastLayout->addWidget(broadcastEdit);
    
    systemMsgCheck = new QCheckBox("Sistem mesajı olarak gönder");
    systemMsgCheck->setChecked(true);
    broadcastLayout->addWidget(systemMsgCheck);
    
    broadcastButton = new QPushButton("📢 Duyuruyu Yayınla");
    broadcastButton->setObjectName("adminButton");
    connect(broadcastButton, &QPushButton::clicked, this, &MainWindow::onBroadcastClicked);
    broadcastLayout->addWidget(broadcastButton);
    
    broadcastLayout->addStretch();
    
    adminTabs->addTab(broadcastTab, "📢 Duyuru");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 6: TEHLİKELİ İŞLEMLER
    // ═══════════════════════════════════════════════════════════════════
    QWidget* dangerTab = new QWidget();
    QVBoxLayout* dangerLayout = new QVBoxLayout(dangerTab);
    
    QLabel* dangerTitle = new QLabel("⚠️ Tehlikeli İşlemler (Sadece ADMIN)");
    dangerTitle->setObjectName("dangerTitle");
    dangerLayout->addWidget(dangerTitle);
    
    QLabel* dangerWarning = new QLabel(
        "Bu işlemler geri alınamaz!\n"
        "Dikkatli kullanın."
    );
    dangerWarning->setObjectName("warningLabel");
    dangerWarning->setAlignment(Qt::AlignCenter);
    dangerLayout->addWidget(dangerWarning);
    
    dangerLayout->addSpacing(20);
    
    terminateAllButton = new QPushButton("☠️ TÜM OTURUMLARI KAPAT");
    terminateAllButton->setObjectName("criticalButton");
    connect(terminateAllButton, &QPushButton::clicked, this, &MainWindow::onTerminateAllClicked);
    dangerLayout->addWidget(terminateAllButton);
    
    dangerLayout->addStretch();
    
    adminTabs->addTab(dangerTab, "⚠️ Tehlikeli");
    
    // ═══════════════════════════════════════════════════════════════════
    // SEKME 7: KULLANICI DURUMLARI (HERKES GÖREBİLİR)
    // ═══════════════════════════════════════════════════════════════════
    QWidget* statusTab = new QWidget();
    QVBoxLayout* statusLayout = new QVBoxLayout(statusTab);
    
    QLabel* statusTitle = new QLabel("👥 Online/Offline Kullanıcılar");
    statusTitle->setObjectName("sectionTitle");
    statusLayout->addWidget(statusTitle);
    
    // Sayaç etiketleri
    QHBoxLayout* countsLayout = new QHBoxLayout();
    
    onlineCountLabel = new QLabel("🟢 Online: 0");
    onlineCountLabel->setStyleSheet("color: #00ff00; font-weight: bold; font-size: 14px;");
    countsLayout->addWidget(onlineCountLabel);
    
    offlineCountLabel = new QLabel("🔴 Offline: 0");
    offlineCountLabel->setStyleSheet("color: #ff6b6b; font-weight: bold; font-size: 14px;");
    countsLayout->addWidget(offlineCountLabel);
    
    totalCountLabel = new QLabel("📊 Toplam: 0");
    totalCountLabel->setStyleSheet("color: #00d4ff; font-weight: bold; font-size: 14px;");
    countsLayout->addWidget(totalCountLabel);
    
    statusLayout->addLayout(countsLayout);
    
    // Splitter - Online ve Offline listelerini yan yana
    QSplitter* statusSplitter = new QSplitter(Qt::Horizontal);
    
    // Online kullanıcılar listesi
    QWidget* onlineWidget = new QWidget();
    QVBoxLayout* onlineLayout = new QVBoxLayout(onlineWidget);
    QLabel* onlineLabel = new QLabel("🟢 Online Kullanıcılar");
    onlineLabel->setStyleSheet("color: #00ff00; font-weight: bold;");
    onlineLayout->addWidget(onlineLabel);
    
    onlineUsersList = new QListWidget();
    onlineUsersList->setObjectName("onlineUsersList");
    onlineUsersList->setStyleSheet(
        "QListWidget { background-color: #1a2a1a; border: 1px solid #00ff00; border-radius: 5px; }"
        "QListWidget::item { color: #00ff00; padding: 5px; }"
        "QListWidget::item:selected { background-color: #2a3a2a; }"
    );
    onlineLayout->addWidget(onlineUsersList);
    statusSplitter->addWidget(onlineWidget);
    
    // Offline kullanıcılar listesi
    QWidget* offlineWidget = new QWidget();
    QVBoxLayout* offlineLayout = new QVBoxLayout(offlineWidget);
    QLabel* offlineLabel = new QLabel("🔴 Offline Kullanıcılar");
    offlineLabel->setStyleSheet("color: #ff6b6b; font-weight: bold;");
    offlineLayout->addWidget(offlineLabel);
    
    offlineUsersList = new QListWidget();
    offlineUsersList->setObjectName("offlineUsersList");
    offlineUsersList->setStyleSheet(
        "QListWidget { background-color: #2a1a1a; border: 1px solid #ff6b6b; border-radius: 5px; }"
        "QListWidget::item { color: #ff6b6b; padding: 5px; }"
        "QListWidget::item:selected { background-color: #3a2a2a; }"
    );
    offlineLayout->addWidget(offlineUsersList);
    statusSplitter->addWidget(offlineWidget);
    
    statusLayout->addWidget(statusSplitter);
    
    // Yenile butonu
    refreshStatusButton = new QPushButton("🔄 Durumları Yenile");
    refreshStatusButton->setObjectName("successButton");
    connect(refreshStatusButton, &QPushButton::clicked, this, &MainWindow::onRefreshUserStatusClicked);
    statusLayout->addWidget(refreshStatusButton);
    
    // Otomatik yenileme timer (5 saniye)
    statusRefreshTimer = new QTimer(this);
    connect(statusRefreshTimer, &QTimer::timeout, this, &MainWindow::onStatusTimerTimeout);
    
    adminTabs->addTab(statusTab, "👥 Durumlar");
    
    panelLayout->addWidget(adminTabs);
}

// ═══════════════════════════════════════════════════════════════════════════
//                         CSS STİLLERİ
// ═══════════════════════════════════════════════════════════════════════════
void MainWindow::setupStyles()
{
    QString styleSheet = R"(
        /* ═══════════════════════════════════════════════════════════════════
           ANA PENCERE
           ═══════════════════════════════════════════════════════════════════ */
        QMainWindow {
            background-color: #1a1a2e;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           LOGIN SAYFASI
           ═══════════════════════════════════════════════════════════════════ */
        #titleLabel {
            font-size: 36px;
            font-weight: bold;
            color: #00d4ff;
            margin-bottom: 5px;
        }
        
        #subtitleLabel {
            font-size: 14px;
            color: #888888;
            margin-bottom: 20px;
        }
        
        #loginGroup {
            background-color: #16213e;
            border: 2px solid #0f3460;
            border-radius: 15px;
            padding: 20px;
            color: #ffffff;
            font-size: 14px;
        }
        
        #loginGroup QLineEdit, #loginGroup QSpinBox {
            background-color: #1a1a2e;
            border: 1px solid #0f3460;
            border-radius: 8px;
            padding: 10px;
            color: #ffffff;
            font-size: 14px;
        }
        
        #loginGroup QLineEdit:focus, #loginGroup QSpinBox:focus {
            border: 2px solid #00d4ff;
        }
        
        #loginButton {
            background-color: #00d4ff;
            color: #1a1a2e;
            font-size: 16px;
            font-weight: bold;
            border: none;
            border-radius: 25px;
            padding: 15px 30px;
        }
        
        #loginButton:hover {
            background-color: #00b8e6;
        }
        
        #loginButton:pressed {
            background-color: #0099cc;
        }
        
        #registerButton {
            background-color: #4CAF50;
            color: white;
            font-size: 16px;
            font-weight: bold;
            border: none;
            border-radius: 25px;
            padding: 15px 30px;
        }
        
        #registerButton:hover {
            background-color: #45a049;
        }
        
        #registerButton:pressed {
            background-color: #3d8b40;
        }
        
        #loginStatusLabel {
            font-size: 14px;
            color: #ff6b6b;
        }
        
        #testUsersGroup {
            background-color: #16213e;
            border: 1px solid #0f3460;
            border-radius: 10px;
            padding: 15px;
            color: #888888;
        }
        
        #testInfo {
            font-family: monospace;
            font-size: 12px;
            color: #aaaaaa;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           CHAT SAYFASI
           ═══════════════════════════════════════════════════════════════════ */
        #userInfoLabel {
            font-size: 16px;
            font-weight: bold;
            color: #00d4ff;
            padding: 10px;
        }
        
        #connectionStatusLabel {
            font-size: 14px;
            padding: 10px;
            color: #ff6b6b;
        }
        
        #logoutButton {
            background-color: #e94560;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 8px 15px;
            font-weight: bold;
        }
        
        #logoutButton:hover {
            background-color: #d63050;
        }
        
        #chatDisplay {
            background-color: #16213e;
            border: 2px solid #0f3460;
            border-radius: 10px;
            color: #ffffff;
            font-family: 'Consolas', monospace;
            font-size: 13px;
            padding: 10px;
        }
        
        #messageEdit {
            background-color: #16213e;
            border: 2px solid #0f3460;
            border-radius: 10px;
            color: #ffffff;
            font-size: 14px;
            padding: 12px;
        }
        
        #messageEdit:focus {
            border: 2px solid #00d4ff;
        }
        
        #sendButton {
            background-color: #00d4ff;
            color: #1a1a2e;
            font-weight: bold;
            border: none;
            border-radius: 10px;
            padding: 12px 25px;
            font-size: 14px;
        }
        
        #sendButton:hover {
            background-color: #00b8e6;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           ADMİN PANELİ
           ═══════════════════════════════════════════════════════════════════ */
        #adminPanel {
            background-color: #16213e;
            border: 2px solid #0f3460;
            border-radius: 10px;
            color: #ffffff;
            font-size: 14px;
        }
        
        #adminTabs {
            background-color: transparent;
        }
        
        #adminTabs::pane {
            border: 1px solid #0f3460;
            border-radius: 5px;
            background-color: #1a1a2e;
        }
        
        #adminTabs::tab-bar {
            alignment: center;
        }
        
        QTabBar::tab {
            background-color: #0f3460;
            color: #888888;
            padding: 8px 12px;
            margin: 2px;
            border-radius: 5px;
        }
        
        QTabBar::tab:selected {
            background-color: #00d4ff;
            color: #1a1a2e;
            font-weight: bold;
        }
        
        #sectionTitle {
            font-size: 16px;
            font-weight: bold;
            color: #00d4ff;
            padding: 10px 0;
        }
        
        #adminPanel QLineEdit, #adminPanel QSpinBox, #adminPanel QComboBox {
            background-color: #1a1a2e;
            border: 1px solid #0f3460;
            border-radius: 5px;
            padding: 8px;
            color: #ffffff;
        }
        
        #adminPanel QLineEdit:focus {
            border: 1px solid #00d4ff;
        }
        
        #userListWidget {
            background-color: #1a1a2e;
            border: 1px solid #0f3460;
            border-radius: 5px;
            color: #ffffff;
        }
        
        #userListWidget::item {
            padding: 8px;
            border-bottom: 1px solid #0f3460;
        }
        
        #userListWidget::item:selected {
            background-color: #0f3460;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           BUTONLAR
           ═══════════════════════════════════════════════════════════════════ */
        #adminButton {
            background-color: #0f3460;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        
        #adminButton:hover {
            background-color: #00d4ff;
            color: #1a1a2e;
        }
        
        #successButton {
            background-color: #00bf63;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        
        #successButton:hover {
            background-color: #00a855;
        }
        
        #warningButton {
            background-color: #f39c12;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        
        #warningButton:hover {
            background-color: #e08e0b;
        }
        
        #dangerButton {
            background-color: #e94560;
            color: white;
            border: none;
            border-radius: 8px;
            padding: 10px 20px;
            font-weight: bold;
        }
        
        #dangerButton:hover {
            background-color: #d63050;
        }
        
        #criticalButton {
            background-color: #8b0000;
            color: white;
            border: 2px solid #ff0000;
            border-radius: 8px;
            padding: 15px 30px;
            font-weight: bold;
            font-size: 14px;
        }
        
        #criticalButton:hover {
            background-color: #a00000;
            border: 2px solid #ff3333;
        }
        
        #dangerTitle {
            font-size: 18px;
            font-weight: bold;
            color: #ff6b6b;
            padding: 10px;
        }
        
        #warningLabel {
            color: #f39c12;
            font-size: 13px;
            padding: 10px;
            background-color: rgba(243, 156, 18, 0.1);
            border-radius: 5px;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           CHECKBOX
           ═══════════════════════════════════════════════════════════════════ */
        QCheckBox {
            color: #ffffff;
            spacing: 8px;
        }
        
        QCheckBox::indicator {
            width: 18px;
            height: 18px;
            border-radius: 3px;
            border: 2px solid #0f3460;
            background-color: #1a1a2e;
        }
        
        QCheckBox::indicator:checked {
            background-color: #00d4ff;
            border-color: #00d4ff;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           STATUS BAR
           ═══════════════════════════════════════════════════════════════════ */
        QStatusBar {
            background-color: #0f3460;
            color: #888888;
            font-size: 12px;
        }
        
        /* ═══════════════════════════════════════════════════════════════════
           SCROLLBAR
           ═══════════════════════════════════════════════════════════════════ */
        QScrollBar:vertical {
            background-color: #1a1a2e;
            width: 12px;
            border-radius: 6px;
        }
        
        QScrollBar::handle:vertical {
            background-color: #0f3460;
            border-radius: 6px;
            min-height: 30px;
        }
        
        QScrollBar::handle:vertical:hover {
            background-color: #00d4ff;
        }
        
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )";
    
    setStyleSheet(styleSheet);
}

// ═══════════════════════════════════════════════════════════════════════════
//                         LOGIN SLOTLARI
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::onLoginClicked()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString address = serverAddressEdit->text().trimmed() + ":" + QString::number(grpcPortSpin->value());
    
    // Validasyon
    if (username.isEmpty() || password.isEmpty())
    {
        loginStatusLabel->setText("❌ Kullanıcı adı ve şifre gerekli!");
        return;
    }
    
    loginStatusLabel->setText("⏳ Giriş yapılıyor...");
    loginButton->setEnabled(false);
    registerButton->setEnabled(false);
    
    // Async login - ayrı thread'de çalışacak
    connect(&loginWatcher, &QFutureWatcher<LoginResult>::finished, 
            this, &MainWindow::onLoginFinished, Qt::UniqueConnection);
    
    QFuture<LoginResult> future = QtConcurrent::run([address, username, password]() -> LoginResult {
        LoginResult result;
        
        qDebug() << "[Login Thread] Basladi:" << address;
        
        // gRPC kanalı oluştur
        auto channel = grpc::CreateChannel(address.toStdString(), grpc::InsecureChannelCredentials());
        auto stub = auth::v1::AuthService::NewStub(channel);
        
        // Login isteği
        auth::v1::LoginRequest request;
        request.set_username(username.toStdString());
        request.set_password(password.toStdString());
        
        auth::v1::LoginResponse response;
        grpc::ClientContext context;
        context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));
        
        qDebug() << "[Login Thread] RPC cagriliyor...";
        grpc::Status status = stub->Login(&context, request, &response);
        
        if (status.ok())
        {
            if (response.success())
            {
                result.success = true;
                result.token = QString::fromStdString(response.token());
                result.username = username;
                result.permission = static_cast<ClientPermission>(response.permission());
                qDebug() << "[Login Thread] Basarili!";
            }
            else
            {
                result.errorMessage = QString::fromStdString(response.error_message());
                qDebug() << "[Login Thread] Sunucu hatasi:" << result.errorMessage;
            }
        }
        else
        {
            result.errorMessage = QString::fromStdString(status.error_message());
            qDebug() << "[Login Thread] gRPC hatasi:" << result.errorMessage;
        }
        
        return result;
    });
    
    loginWatcher.setFuture(future);
}

void MainWindow::onLoginFinished()
{
    LoginResult result = loginWatcher.result();
    
    loginButton->setEnabled(true);
    registerButton->setEnabled(true);
    
    if (result.success)
    {
        // Başarılı giriş
        currentToken = result.token;
        currentUsername = result.username;
        currentPermission = result.permission;
        
        // gRPC kanalı ve stub'ları oluştur (UI thread'de)
        QString address = serverAddressEdit->text().trimmed() + ":" + QString::number(grpcPortSpin->value());
        grpcChannel = grpc::CreateChannel(address.toStdString(), grpc::InsecureChannelCredentials());
        authStub = auth::v1::AuthService::NewStub(grpcChannel);
        adminStub = auth::v1::AdminService::NewStub(grpcChannel);
        
        loginStatusLabel->setText("✅ Giriş başarılı!");
        
        // TCP bağlantısını kur
        connectToTcpServer();
        
        // Chat sayfasına geç
        stackedWidget->setCurrentWidget(chatPage);
        
        // Kullanıcı bilgisini güncelle
        userInfoLabel->setText(QString("👤 %1 [%2]").arg(currentUsername).arg(permissionToString(currentPermission)));
        
        // Yetki seviyesine göre UI güncelle
        updatePermissionUi();
        
        // Kullanıcı durumlarını yükle ve otomatik yenilemeyi başlat
        // grpcGetAllUsersStatus() tüm kullanıcıları gösterir (online/offline)
        grpcGetAllUsersStatus();
        statusRefreshTimer->start(5000);
        
        // ADMIN veya MODERATOR ise ComboBox'ları da güncelle (yetki değiştirme, ban vb. için)
        if (currentPermission == ClientPermission::ADMIN || 
            currentPermission == ClientPermission::MODERATOR)
        {
            loadActiveUsers();
        }
        
        statusBar()->showMessage("Giriş başarılı - " + currentUsername);
    }
    else
    {
        loginStatusLabel->setText("❌ " + (result.errorMessage.isEmpty() ? "Giriş başarısız!" : result.errorMessage));
    }
}

// Kayıt ol tıklandığında
void MainWindow::onRegisterClicked()
{
    QString username = usernameEdit->text().trimmed();
    QString password = passwordEdit->text();
    QString email = emailEdit->text().trimmed();

    // Basit validasyon
    if (username.isEmpty() || password.isEmpty())
    {
        loginStatusLabel->setText("❌ Kayıt için kullanıcı adı ve şifre gerekli!");
        return;
    }

    if (username.length() < 3)
    {
        loginStatusLabel->setText("❌ Kullanıcı adı en az 3 karakter olmalı!");
        return;
    }

    if (password.length() < 6)
    {
        loginStatusLabel->setText("❌ Şifre en az 6 karakter olmalı!");
        return;
    }

    loginStatusLabel->setText("⏳ Kayıt yapılıyor...");
    registerButton->setEnabled(false);

    if (grpcRegister(username, password, email))
    {
        loginStatusLabel->setText("✅ Kayıt başarılı! Şimdi giriş yapabilirsiniz.");
        QMessageBox::information(this, "Kayıt", "Kayıt başarılı! Giriş yapabilirsiniz.");
    }
    else
    {
        loginStatusLabel->setText("❌ Kayıt başarısız! Farklı bir kullanıcı adı deneyin.");
    }

    registerButton->setEnabled(true);
}

void MainWindow::onLogoutClicked()
{
    // TCP bağlantısını kapat
    disconnectFromTcpServer();
    
    // Otomatik yenileme timer'ını durdur
    statusRefreshTimer->stop();
    
    // Oturum verilerini temizle
    currentToken.clear();
    currentUsername.clear();
    currentPermission = ClientPermission::BANNED;
    
    // Chat'i temizle
    chatDisplay->clear();
    
    // Kullanıcı durum listelerini temizle
    onlineUsersList->clear();
    offlineUsersList->clear();
    onlineCountLabel->setText("🟢 Online: 0");
    offlineCountLabel->setText("🔴 Offline: 0");
    totalCountLabel->setText("📊 Toplam: 0");
    
    // Login sayfasına dön
    stackedWidget->setCurrentWidget(loginPage);
    loginStatusLabel->clear();
    
    statusBar()->showMessage("Çıkış yapıldı.");
}

// ═══════════════════════════════════════════════════════════════════════════
//                         CHAT SLOTLARI
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::onSendMessageClicked()
{
    QString message = messageEdit->text().trimmed();
    
    if (message.isEmpty())
        return;
    
    // GUEST mesaj gönderemez (sunucu tarafında da kontrol var)
    if (currentPermission == ClientPermission::GUEST)
    {
        appendSystemMessage("⚠️ GUEST kullanıcılar mesaj gönderemez!");
        return;
    }
    
    // TCP üzerinden mesaj gönder
    if (tcpSocket && tcpSocket->isOpen())
    {
        QByteArray data = message.toUtf8() + "\n";
        tcpSocket->write(data);
        tcpSocket->flush();
        
        // Kendi mesajımızı gösterme (echo olarak görünmesin)
        
        messageEdit->clear();
    }
    else
    {
        appendSystemMessage("❌ Sunucuya bağlı değilsiniz!");
    }
}

void MainWindow::onTcpConnected()
{
    isConnected = true;
    connectionStatusLabel->setText("🟢 Bağlı");
    connectionStatusLabel->setStyleSheet("color: #00bf63;");
    
    appendSystemMessage("✅ TCP sunucusuna bağlandı!");
    
    // Token'ı gönder (handshake) - kısa bir gecikme ile server'ın hazır olmasını bekle
    if (!currentToken.isEmpty())
    {
        // Socket'in tamamen hazır olması için kısa bir gecikme
        QTimer::singleShot(200, this, [this]() {
            if (tcpSocket && tcpSocket->state() == QAbstractSocket::ConnectedState && !currentToken.isEmpty())
            {
                QByteArray tokenData = currentToken.toUtf8() + "\n";
                qint64 bytesWritten = tcpSocket->write(tokenData);
                
                if (!tcpSocket->waitForBytesWritten(3000))
                {
                    appendSystemMessage("❌ Token gönderilemedi: " + tcpSocket->errorString());
                    return;
                }
                
                if (bytesWritten == -1)
                {
                    appendSystemMessage("❌ Token gönderilemedi: " + tcpSocket->errorString());
                }
                else if (bytesWritten != tokenData.size())
                {
                    appendSystemMessage("⚠️ Token kısmen gönderildi: " + QString::number(bytesWritten) + "/" + QString::number(tokenData.size()));
                }
                else
                {
                    // Token başarıyla gönderildi
                    qDebug() << "[TCP] Token başarıyla gönderildi:" << bytesWritten << "bytes";
                }
            }
            else
            {
                appendSystemMessage("⚠️ Socket hazır değil veya token bulunamadı!");
            }
        });
    }
    else
    {
        appendSystemMessage("⚠️ Token bulunamadı, TCP handshake yapılamadı!");
    }
    
    statusBar()->showMessage("TCP bağlantısı kuruldu.");
}

void MainWindow::onTcpDisconnected()
{
    isConnected = false;
    connectionStatusLabel->setText("🔴 Bağlı Değil");
    connectionStatusLabel->setStyleSheet("color: #ff6b6b;");
    
    appendSystemMessage("⚠️ TCP bağlantısı kesildi!");
    
    statusBar()->showMessage("TCP bağlantısı kesildi.");
}

void MainWindow::onTcpReadyRead()
{
    // Sunucudan gelen veriyi oku
    while (tcpSocket->canReadLine())
    {
        QByteArray data = tcpSocket->readLine();
        QString message = QString::fromUtf8(data).trimmed();
        
        if (!message.isEmpty())
        {
            // Yetki güncelleme kontrolü (sadece kendi yetkimiz değiştiyse)
            if (message.contains("PERM_UPDATE:"))
            {
                int permIndex = message.indexOf("PERM_UPDATE:");
                if (permIndex != -1)
                {
                    QString permStr = message.mid(permIndex + 12).split(" ").first();
                    bool ok;
                    int newPerm = permStr.toInt(&ok);
                    if (ok && newPerm >= 0 && newPerm <= 4)
                    {
                        ClientPermission oldPerm = currentPermission;
                        currentPermission = static_cast<ClientPermission>(newPerm);
                        userInfoLabel->setText(QString("👤 %1 [%2]").arg(currentUsername).arg(permissionToString(currentPermission)));
                        
                        // UI'ı güncelle (butonları doğru şekilde enable/disable et)
                        updatePermissionUi();
                        
                        // Sadece yetki gerçekten değiştiyse mesaj göster
                        if (oldPerm != currentPermission)
                        {
                            appendSystemMessage("✅ Yetkiniz güncellendi: " + permissionToString(currentPermission));
                        }
                    }
                }
            }
            
            // Mesaj tipine göre renklendir
            if (message.startsWith("[OK]"))
            {
                appendMessage("✅ " + message, "#00bf63");
            }
            else if (message.startsWith("[ERR]"))
            {
                appendMessage("❌ " + message, "#ff6b6b");
            }
            else if (message.startsWith("[SISTEM]"))
            {
                appendMessage("📢 " + message, "#f39c12");
            }
            else if (message.startsWith("[SUNUCU]"))
            {
                appendMessage("📩 " + message, "#ffffff");
            }
            else
            {
                appendMessage(message, "#aaaaaa");
            }
        }
    }
}

void MainWindow::onTcpError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    appendSystemMessage("❌ TCP Hatası: " + tcpSocket->errorString());
    statusBar()->showMessage("TCP Hatası: " + tcpSocket->errorString());
}

// ═══════════════════════════════════════════════════════════════════════════
//                         ADMİN SLOTLARI
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::onChangePermissionClicked()
{
    // ComboBox'tan kullanıcı adını al (data değeri)
    QString target = permTargetCombo->currentData().toString();
    
    // Eğer data boşsa, kullanıcı elle yazmış olabilir
    if (target.isEmpty())
    {
        target = permTargetCombo->currentText().trimmed();
        // "-- Kullanıcı seçin --" gibi placeholder'ı temizle
        if (target.startsWith("--")) target.clear();
    }
    
    int newPerm = permLevelCombo->currentData().toInt();
    
    if (target.isEmpty())
    {
        QMessageBox::warning(this, "Hata", "Hedef kullanıcı seçin veya yazın!");
        return;
    }
    
    if (grpcChangePermission(target, newPerm))
    {
        appendSystemMessage("✅ " + target + " kullanıcısının yetkisi değiştirildi.");
        permTargetCombo->setCurrentIndex(0);
        
        // ComboBox'ları güncelle (sadece ComboBox'lar, userListWidget değil)
        // userListWidget zaten grpcGetAllUsersStatus() ile otomatik güncelleniyor
        loadActiveUsers();
        
        // Butonları tekrar aktif et (eğer admin ise)
        updatePermissionUi();
    }
    else
    {
        appendSystemMessage("❌ Yetki değiştirme başarısız!");
    }
}

void MainWindow::onBanUserClicked()
{
    // ComboBox'tan kullanıcı adını al
    QString target = banTargetCombo->currentData().toString();
    if (target.isEmpty())
    {
        target = banTargetCombo->currentText().trimmed();
        if (target.startsWith("--")) target.clear();
    }
    
    QString reason = banReasonEdit->text().trimmed();
    int duration = banDurationSpin->value();
    
    if (target.isEmpty())
    {
        QMessageBox::warning(this, "Hata", "Hedef kullanıcı seçin veya yazın!");
        return;
    }
    
    if (reason.isEmpty())
        reason = "Sebep belirtilmedi";
    
    if (grpcBanUser(target, reason, duration))
    {
        appendSystemMessage("🚫 " + target + " banlandı. Sebep: " + reason);
        banTargetCombo->setCurrentIndex(0);
        banReasonEdit->clear();
        loadActiveUsers(); // Listeyi yenile
    }
    else
    {
        appendSystemMessage("❌ Banlama başarısız!");
    }
}

void MainWindow::onUnbanUserClicked()
{
    // ComboBox'tan kullanıcı adını al
    QString target = banTargetCombo->currentData().toString();
    if (target.isEmpty())
    {
        target = banTargetCombo->currentText().trimmed();
        if (target.startsWith("--")) target.clear();
    }
    
    if (target.isEmpty())
    {
        QMessageBox::warning(this, "Hata", "Hedef kullanıcı seçin veya yazın!");
        return;
    }
    
    if (grpcUnbanUser(target))
    {
        appendSystemMessage("✅ " + target + " kullanıcısının banı kaldırıldı.");
        banTargetCombo->setCurrentIndex(0);
        loadActiveUsers(); // Listeyi yenile
    }
    else
    {
        appendSystemMessage("❌ Ban kaldırma başarısız!");
    }
}

void MainWindow::onKickUserClicked()
{
    // ComboBox'tan kullanıcı adını al
    QString target = kickTargetCombo->currentData().toString();
    if (target.isEmpty())
    {
        target = kickTargetCombo->currentText().trimmed();
        if (target.startsWith("--")) target.clear();
    }
    
    QString reason = kickReasonEdit->text().trimmed();
    
    if (target.isEmpty())
    {
        QMessageBox::warning(this, "Hata", "Hedef kullanıcı seçin veya yazın!");
        return;
    }
    
    if (reason.isEmpty())
        reason = "Sebep belirtilmedi";
    
    if (grpcKickUser(target, reason))
    {
        appendSystemMessage("🦵 " + target + " atıldı. Sebep: " + reason);
        kickTargetCombo->setCurrentIndex(0);
        kickReasonEdit->clear();
        loadActiveUsers(); // Listeyi yenile
    }
    else
    {
        appendSystemMessage("❌ Kick başarısız!");
    }
}

void MainWindow::onBroadcastClicked()
{
    QString message = broadcastEdit->text().trimmed();
    bool isSystem = systemMsgCheck->isChecked();
    
    if (message.isEmpty())
    {
        QMessageBox::warning(this, "Hata", "Duyuru mesajı gerekli!");
        return;
    }
    
    if (grpcBroadcast(message, isSystem))
    {
        appendSystemMessage("📢 Duyuru yayınlandı: " + message);
        broadcastEdit->clear();
    }
    else
    {
        appendSystemMessage("❌ Duyuru başarısız!");
    }
}

void MainWindow::onRefreshUsersClicked()
{
    userListWidget->clear();
    
    if (grpcListUsers())
    {
        appendSystemMessage("🔄 Kullanıcı listesi yenilendi.");
        // ComboBox'ları da güncelle
        loadActiveUsers();
    }
    else
    {
        appendSystemMessage("❌ Kullanıcı listesi alınamadı!");
    }
}

void MainWindow::onTerminateAllClicked()
{
    // Onay iste
    QMessageBox::StandardButton reply = QMessageBox::warning(
        this,
        "⚠️ Tehlikeli İşlem",
        "TÜM OTURUMLARI KAPATMAK ÜZERESİNİZ!\n\n"
        "Bu işlem geri alınamaz ve tüm kullanıcılar\n"
        "sunucudan atılacaktır.\n\n"
        "Devam etmek istiyor musunuz?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );
    
    if (reply == QMessageBox::Yes)
    {
        if (grpcTerminateAll("Admin tarafından kapatıldı"))
        {
            appendSystemMessage("☠️ Tüm oturumlar kapatıldı!");
        }
        else
        {
            appendSystemMessage("❌ İşlem başarısız!");
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YARDIMCI METODLAR
// ═══════════════════════════════════════════════════════════════════════════

void MainWindow::connectToTcpServer()
{
    QString address = serverAddressEdit->text().trimmed();
    int port = tcpPortSpin->value();
    
    if (address.isEmpty())
    {
        appendSystemMessage("❌ Sunucu adresi boş!");
        return;
    }
    
    // Eğer zaten bağlıysa önce kapat
    if (tcpSocket && tcpSocket->state() != QAbstractSocket::UnconnectedState)
    {
        tcpSocket->abort();
        tcpSocket->waitForDisconnected(1000);
    }
    
    appendSystemMessage("⏳ TCP sunucusuna bağlanılıyor: " + address + ":" + QString::number(port));
    
    // Bağlantıyı başlat (async - waitForConnected kullanmıyoruz, signal/slot ile handle ediyoruz)
    tcpSocket->connectToHost(address, port);
}

void MainWindow::disconnectFromTcpServer()
{
    if (tcpSocket && tcpSocket->isOpen())
    {
        tcpSocket->close();
    }
}

void MainWindow::appendMessage(const QString& msg, const QString& color)
{
    QString timestamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    QString html = QString("<span style='color: #666666;'>[%1]</span> <span style='color: %2;'>%3</span>")
                   .arg(timestamp)
                   .arg(color)
                   .arg(msg.toHtmlEscaped());
    
    chatDisplay->append(html);
    
    // Otomatik scroll
    QScrollBar* scrollBar = chatDisplay->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void MainWindow::appendSystemMessage(const QString& msg)
{
    appendMessage(msg, "#f39c12");
}

void MainWindow::updatePermissionUi()
{
    // ADMIN ve MODERATOR admin panelini görebilir
    bool canAccessAdmin = (currentPermission == ClientPermission::ADMIN || 
                          currentPermission == ClientPermission::MODERATOR);
    
    adminPanel->setVisible(canAccessAdmin);
    
    // Sadece ADMIN yetki değiştirebilir
    bool isAdmin = (currentPermission == ClientPermission::ADMIN);
    changePermButton->setEnabled(isAdmin);
    permLevelCombo->setEnabled(isAdmin);
    terminateAllButton->setEnabled(isAdmin);
    
    // GUEST mesaj gönderemez
    bool canSendMessage = (currentPermission != ClientPermission::GUEST);
    sendButton->setEnabled(canSendMessage);
    messageEdit->setEnabled(canSendMessage);
    
    if (!canSendMessage)
    {
        messageEdit->setPlaceholderText("GUEST kullanıcılar mesaj gönderemez");
    }
}

QString MainWindow::permissionToString(ClientPermission perm)
{
    switch (perm)
    {
        case ClientPermission::ADMIN: return "ADMIN";
        case ClientPermission::MODERATOR: return "MODERATOR";
        case ClientPermission::USER: return "USER";
        case ClientPermission::GUEST: return "GUEST";
        case ClientPermission::BANNED: return "BANNED";
        default: return "UNKNOWN";
    }
}

void MainWindow::loadActiveUsers()
{
    qDebug() << "[loadActiveUsers] Fonksiyon çağrıldı";
    
    if (!adminStub) {
        qDebug() << "[loadActiveUsers] adminStub NULL - fonksiyondan çıkılıyor";
        return;
    }
    
    qDebug() << "[loadActiveUsers] Token:" << currentToken;
    
    auth::v1::ListUsersRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_online_or_offline(OnlineOrOfflineCheck::BOTH_OF_THEM);
    request.set_with_banned_person(true);
    
    auth::v1::ListUsersResponse response;
    grpc::ClientContext context;
    // DEADLINE EKLENDI - Donma sorununu çözer
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    qDebug() << "[loadActiveUsers] gRPC çağrısı yapılıyor...";
    grpc::Status status = adminStub->ListActiveUsers(&context, request, &response);
    
    qDebug() << "[loadActiveUsers] gRPC sonucu - OK:" << status.ok() << "Success:" << response.success();
    qDebug() << "[loadActiveUsers] Kullanıcı sayısı:" << response.users_size();
    qDebug() << "[loadActiveUsers] Mesaj:" << QString::fromStdString(response.message());
    
    if (status.ok() && response.success())
    {
        // Tüm ComboBox'ları güncelle
        populateUserComboBox(permTargetCombo);
        populateUserComboBox(banTargetCombo);
        populateUserComboBox(kickTargetCombo);
        
        // Kullanıcıları ComboBox'lara ekle
        for (const auto& user : response.users())
        {
            QString username = QString::fromStdString(user.username());
            
            // Yetki seviyesini string'e çevir
            QString permStr;
            switch(user.permission())
            {
                case auth::v1::ADMIN: permStr = "👑 ADMIN"; break;
                case auth::v1::MODERATOR: permStr = "🛡️ MOD"; break;
                case auth::v1::USER: permStr = "👤 USER"; break;
                case auth::v1::GUEST: permStr = "👁️ GUEST"; break;
                case auth::v1::BANNED: permStr = "🚫 BANNED"; break;
                default: permStr = "❓ UNKNOWN"; break;
            }
            
            QString displayText = QString("%1 [%2]%3")
                .arg(username)
                .arg(permStr)
                .arg(user.is_online() ? " 🟢" : " 🔴");
            
            qDebug() << "[loadActiveUsers] Kullanıcı ekleniyor:" << displayText;
            
            permTargetCombo->addItem(displayText, username);
            banTargetCombo->addItem(displayText, username);
            kickTargetCombo->addItem(displayText, username);
        }
        
        // Mesaj gösterme (otomatik yenileme sırasında rahatsız edici olur)
        // appendSystemMessage("✅ Kullanıcı listesi yüklendi (" + QString::number(response.users_size()) + " kullanıcı)");
    }
    else
    {
        qDebug() << "[loadActiveUsers] HATA - gRPC başarısız veya response.success() false";
        if (!status.ok()) {
            qDebug() << "[loadActiveUsers] gRPC hata:" << QString::fromStdString(status.error_message());
        }
    }
}

void MainWindow::populateUserComboBox(QComboBox* combo)
{
    if (!combo) return;
    
    // Mevcut metni koru
    QString currentText = combo->currentText();
    
    // Listeyi temizle
    combo->clear();
    
    // Placeholder ekle
    combo->addItem("-- Kullanıcı seçin --", "");
    combo->setCurrentIndex(0);
    
    // Eski metni geri yükle (kullanıcı yazıyorsa)
    if (!currentText.isEmpty() && currentText != "-- Kullanıcı seçin --")
    {
        combo->setEditText(currentText);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         gRPC METODLARI
// ═══════════════════════════════════════════════════════════════════════════

bool MainWindow::grpcLogin(const QString& username, const QString& password)
{
    QString address = serverAddressEdit->text().trimmed() + ":" + QString::number(grpcPortSpin->value());
    qDebug() << "[Login] Baglanti kuruluyor:" << address;
    
    // Qt'nin donmaması için event loop'u çalıştır
    QApplication::processEvents();
    
    // Kanal argümanları - hızlı bağlantı için
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 1000);
    args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
    args.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 100);
    args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 1000);
    
    auto channel = grpc::CreateCustomChannel(address.toStdString(), 
                                             grpc::InsecureChannelCredentials(), 
                                             args);
    auto stub = auth::v1::AuthService::NewStub(channel);
    
    auth::v1::LoginRequest request;
    request.set_username(username.toStdString());
    request.set_password(password.toStdString());
    
    auth::v1::LoginResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    qDebug() << "[Login] RPC cagriliyor...";
    QApplication::processEvents();
    
    grpc::Status status = stub->Login(&context, request, &response);
    
    QApplication::processEvents();
    
    if (status.ok())
    {
        if (response.success())
        {
            grpcChannel = channel;
            authStub = auth::v1::AuthService::NewStub(grpcChannel);
            adminStub = auth::v1::AdminService::NewStub(grpcChannel);
            
            currentToken = QString::fromStdString(response.token());
            currentUsername = username;
            currentPermission = static_cast<ClientPermission>(response.permission());
            qDebug() << "[Login] Giris basarili - Kullanici:" << username;
            return true;
        }
        else
        {
            qDebug() << "[Login] Sunucu hatasi:" << QString::fromStdString(response.error_message());
            loginStatusLabel->setText("❌ " + QString::fromStdString(response.error_message()));
        }
    }
    else
    {
        qDebug() << "[Login] gRPC hatasi:" << QString::fromStdString(status.error_message());
        loginStatusLabel->setText("❌ Bağlantı hatası: " + QString::fromStdString(status.error_message()));
    }
    
    return false;
}

bool MainWindow::grpcRegister(const QString& username, const QString& password, const QString& email)
{
    // Her kayıt denemesinde yeni kanal ve stub oluştur
    QString address = serverAddressEdit->text().trimmed() + ":" + QString::number(grpcPortSpin->value());
    qDebug() << "[Register] Baglanti kuruluyor:" << address;
    
    auto channel = grpc::CreateChannel(address.toStdString(), grpc::InsecureChannelCredentials());
    auto stub = auth::v1::AuthService::NewStub(channel);

    // Register isteği
    auth::v1::RegisterRequest request;
    request.set_username(username.toStdString());
    request.set_password(password.toStdString());
    request.set_email(email.toStdString());

    auth::v1::RegisterResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(10));

    qDebug() << "[Register] RPC cagriliyor...";
    grpc::Status status = stub->Register(&context, request, &response);

    if (status.ok())
    {
        if (response.success())
        {
            qDebug() << "[Register] Kayit basarili - ID:" << QString::fromStdString(response.user_id());
            return true;
        }
        else
        {
            qDebug() << "[Register] Sunucu hatasi:" << QString::fromStdString(response.message());
            loginStatusLabel->setText("❌ " + QString::fromStdString(response.message()));
        }
    }
    else
    {
        qDebug() << "[Register] gRPC hatasi:" << QString::fromStdString(status.error_message());
        loginStatusLabel->setText("❌ Bağlantı hatası: " + QString::fromStdString(status.error_message()));
    }

    return false;
}

bool MainWindow::grpcChangePermission(const QString& target, int newPerm)
{
    if (!adminStub) return false;
    
    auth::v1::ChangePermissionRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_target_username(target.toStdString());
    request.set_new_permission(static_cast<auth::v1::PermissionLevel>(newPerm));
    
    auth::v1::ChangePermissionResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->ChangeUserPermission(&context, request, &response);
    
    return status.ok() && response.success();
}

bool MainWindow::grpcBanUser(const QString& target, const QString& reason, int duration)
{
    if (!adminStub) return false;
    
    auth::v1::BanUserRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_target_username(target.toStdString());
    request.set_reason(reason.toStdString());
    request.set_duration_minutes(duration);
    
    auth::v1::BanUserResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->BanUser(&context, request, &response);
    
    return status.ok() && response.success();
}

bool MainWindow::grpcUnbanUser(const QString& target)
{
    if (!adminStub) return false;
    
    auth::v1::UnbanUserRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_target_username(target.toStdString());
    
    auth::v1::UnbanUserResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->UnbanUser(&context, request, &response);
    
    return status.ok() && response.success();
}

bool MainWindow::grpcKickUser(const QString& target, const QString& reason)
{
    if (!adminStub) return false;
    
    auth::v1::KickUserRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_target_username(target.toStdString());
    request.set_reason(reason.toStdString());
    
    auth::v1::KickUserResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->KickUser(&context, request, &response);
    
    return status.ok() && response.success();
}

bool MainWindow::grpcBroadcast(const QString& message, bool isSystem)
{
    if (!adminStub) return false;
    
    auth::v1::BroadcastRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_message(message.toStdString());
    request.set_is_system_message(isSystem);
    
    auth::v1::BroadcastResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->BroadcastMessage(&context, request, &response);
    
    return status.ok() && response.success();
}

bool MainWindow::grpcListUsers()
{
    if (!adminStub) return false;
    
    auth::v1::ListUsersRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_online_or_offline(OnlineOrOfflineCheck::BOTH_OF_THEM);
    request.set_with_banned_person(true);
    
    auth::v1::ListUsersResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->ListActiveUsers(&context, request, &response);
    
    if (status.ok() && response.success())
    {
        // Kullanıcı listesini doldur
        for (const auto& user : response.users())
        {
            // Yetki seviyesini string'e çevir
            QString permStr;
            switch(user.permission())
            {
                case auth::v1::ADMIN: permStr = "👑 ADMIN"; break;
                case auth::v1::MODERATOR: permStr = "🛡️ MOD"; break;
                case auth::v1::USER: permStr = "👤 USER"; break;
                case auth::v1::GUEST: permStr = "👁️ GUEST"; break;
                case auth::v1::BANNED: permStr = "🚫 BANNED"; break;
                default: permStr = "❓ UNKNOWN"; break;
            }
            
            QString itemText = QString("%1 [%2] %3")
                .arg(QString::fromStdString(user.username()))
                .arg(permStr)
                .arg(user.is_online() ? "🟢" : "🔴");
            
            userListWidget->addItem(itemText);
        }
        return true;
    }
    
    return false;
}

bool MainWindow::grpcTerminateAll(const QString& reason)
{
    if (!adminStub) return false;
    
    auth::v1::TerminateAllRequest request;
    request.set_admin_token(currentToken.toStdString());
    request.set_reason(reason.toStdString());
    
    auth::v1::TerminateAllResponse response;
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = adminStub->TerminateAllSessions(&context, request, &response);
    
    return status.ok() && response.success();
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI DURUMU gRPC METODU
// ═══════════════════════════════════════════════════════════════════════════
bool MainWindow::grpcGetAllUsersStatus()
{
    if (!authStub) return false;
    
    auth::v1::AllUsersStatusRequest request;
    request.set_token(currentToken.toStdString());
    
    auth::v1::AllUsersStatusResponse response;
    grpc::ClientContext context;
    // DEADLINE EKLENDI - Donma sorununu çözer
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));
    
    grpc::Status status = authStub->GetAllUsersStatus(&context, request, &response);
    
    if (status.ok() && response.success())
    {
        // Listeleri temizle
        onlineUsersList->clear();
        offlineUsersList->clear();
        userListWidget->clear();  // Aktif kullanıcılar listesini de temizle
        
        // Online kullanıcıları ekle
        for (int i = 0; i < response.online_users_size(); ++i)
        {
            const auto& user = response.online_users(i);
            QString displayText = QString::fromStdString(user.username());
            
            // Yetki seviyesini de göster
            QString permStr;
            switch(user.permission())
            {
                case auth::v1::ADMIN: permStr = "👑 ADMIN"; break;
                case auth::v1::MODERATOR: permStr = "🛡️ MOD"; break;
                case auth::v1::USER: permStr = "👤 USER"; break;
                case auth::v1::GUEST: permStr = "👁️ GUEST"; break;
                case auth::v1::BANNED: permStr = "🚫 BANNED"; break;
            }
            
            displayText += " [" + permStr + "]";
            onlineUsersList->addItem(displayText);
            
            // userListWidget'a da ekle (online)
            userListWidget->addItem(displayText + " 🟢");
        }
        
        // Offline kullanıcıları ekle
        for (int i = 0; i < response.offline_users_size(); ++i)
        {
            const auto& user = response.offline_users(i);
            QString displayText = QString::fromStdString(user.username());
            
            // Yetki seviyesini de göster
            QString permStr;
            switch(user.permission())
            {
                case auth::v1::ADMIN: permStr = "👑 ADMIN"; break;
                case auth::v1::MODERATOR: permStr = "🛡️ MOD"; break;
                case auth::v1::USER: permStr = "👤 USER"; break;
                case auth::v1::GUEST: permStr = "👁️ GUEST"; break;
                case auth::v1::BANNED: permStr = "🚫 BANNED"; break;
            }
            displayText += " [" + permStr + "]";
            
            // Son görülme zamanı
            QString lastSeen = QString::fromStdString(user.last_seen());
            if (!lastSeen.isEmpty())
            {
                displayText += " (Son: " + lastSeen + ")";
            }
            
            offlineUsersList->addItem(displayText);
            
            // userListWidget'a da ekle (offline)
            userListWidget->addItem(displayText + " 🔴");
        }
        
        // Sayaçları güncelle
        onlineCountLabel->setText(QString("🟢 Online: %1").arg(response.online_count()));
        offlineCountLabel->setText(QString("🔴 Offline: %1").arg(response.offline_count()));
        totalCountLabel->setText(QString("📊 Toplam: %1").arg(response.total_count()));
        
        return true;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI DURUMU SLOTLARI
// ═══════════════════════════════════════════════════════════════════════════
void MainWindow::onRefreshUserStatusClicked()
{
    if (!grpcGetAllUsersStatus())
    {
        appendSystemMessage("⚠️ Kullanıcı durumları alınamadı");
    }
    else
    {
        appendSystemMessage("✅ Kullanıcı durumları güncellendi");
    }
}

void MainWindow::onStatusTimerTimeout()
{
    // Sessizce güncelle (mesaj gösterme)
    // grpcGetAllUsersStatus() zaten userListWidget'ı da güncelliyor
    grpcGetAllUsersStatus();
    
    // ADMIN veya MODERATOR ise ComboBox'ları da güncelle (yetki değiştirme, ban vb. için)
    if (currentPermission == ClientPermission::ADMIN || 
        currentPermission == ClientPermission::MODERATOR)
    {
        loadActiveUsers();
    }
}
