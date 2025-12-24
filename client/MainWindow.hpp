// ═══════════════════════════════════════════════════════════════════════════
//                         ANA PENCERE HEADER DOSYASI
// MainWindow sınıfı - Uygulamanın ana penceresi
// ═══════════════════════════════════════════════════════════════════════════

#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QMainWindow>
#include <QStackedWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QComboBox>
#include <QSpinBox>
#include <QStatusBar>
#include <QTimer>
#include <QThread>
#include <QCheckBox>
#include <QFutureWatcher>
#include <QtConcurrent>

// gRPC ve Protobuf
#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"

// TCP Socket
#include <QTcpSocket>

// Proto'dan gelen enum'ları kullan
using auth::v1::OnlineOrOfflineCheck;

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ SEVİYESİ ENUM
// ═══════════════════════════════════════════════════════════════════════════
enum class ClientPermission {
    ADMIN = 0,
    MODERATOR = 1,
    USER = 2,
    GUEST = 3,
    BANNED = 4
};

// Login sonucu için struct
struct LoginResult {
    bool success = false;
    QString token;
    QString username;
    ClientPermission permission = ClientPermission::BANNED;
    QString errorMessage;
};

// ═══════════════════════════════════════════════════════════════════════════
//                         ANA PENCERE SINIFI
// ═══════════════════════════════════════════════════════════════════════════
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // Constructor ve Destructor
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // ─────────────────────────────────────────────────────────────────────────
    // LOGIN SLOTLARI
    // ─────────────────────────────────────────────────────────────────────────
    void onLoginClicked();              // Giriş butonuna tıklandığında
    void onLoginFinished();             // Async login tamamlandığında
    void onRegisterClicked();           // Kayıt ol butonuna tıklandığında
    void onLogoutClicked();             // Çıkış butonuna tıklandığında

    // ─────────────────────────────────────────────────────────────────────────
    // CHAT SLOTLARI
    // ─────────────────────────────────────────────────────────────────────────
    void onSendMessageClicked();        // Mesaj gönder butonuna tıklandığında
    void onTcpConnected();              // TCP bağlantısı kurulduğunda
    void onTcpDisconnected();           // TCP bağlantısı kesildiğinde
    void onTcpReadyRead();              // TCP'den veri geldiğinde
    void onTcpError(QAbstractSocket::SocketError error);  // TCP hatası

    // ─────────────────────────────────────────────────────────────────────────
    // ADMİN SLOTLARI
    // ─────────────────────────────────────────────────────────────────────────
    void onChangePermissionClicked();   // Yetki değiştir
    void onBanUserClicked();            // Kullanıcı banla
    void onUnbanUserClicked();          // Ban kaldır
    void onKickUserClicked();           // Kullanıcı at
    void onBroadcastClicked();          // Broadcast mesaj gönder
    void onRefreshUsersClicked();       // Kullanıcı listesini yenile
    void onTerminateAllClicked();       // Tüm oturumları kapat

    // ─────────────────────────────────────────────────────────────────────────
    // KULLANICI DURUMU SLOTLARI
    // ─────────────────────────────────────────────────────────────────────────
    void onRefreshUserStatusClicked();  // Kullanıcı durumlarını yenile
    void onStatusTimerTimeout();        // Otomatik yenileme timer

private:
    // ─────────────────────────────────────────────────────────────────────────
    // UI OLUŞTURMA METODLARI
    // ─────────────────────────────────────────────────────────────────────────
    void setupUi();                     // Ana UI kurulumu
    void setupLoginPage();              // Giriş sayfası
    void setupChatPage();               // Chat sayfası
    void setupAdminPanel();             // Admin paneli
    void setupStyles();                 // CSS stilleri
    
    // ─────────────────────────────────────────────────────────────────────────
    // YARDIMCI METODLAR
    // ─────────────────────────────────────────────────────────────────────────
    void connectToTcpServer();          // TCP sunucusuna bağlan
    void disconnectFromTcpServer();     // TCP bağlantısını kes
    void appendMessage(const QString& msg, const QString& color = "white");
    void appendSystemMessage(const QString& msg);
    void updatePermissionUi();          // Yetki seviyesine göre UI güncelle
    QString permissionToString(ClientPermission perm);
    void loadActiveUsers();             // Aktif kullanıcıları yükle
    void populateUserComboBox(QComboBox* combo); // ComboBox'a kullanıcıları ekle
    
    // ─────────────────────────────────────────────────────────────────────────
    // gRPC METODLARI
    // ─────────────────────────────────────────────────────────────────────────
    bool grpcLogin(const QString& username, const QString& password);
    bool grpcRegister(const QString& username, const QString& password, const QString& email);
    bool grpcChangePermission(const QString& target, int newPerm);
    bool grpcBanUser(const QString& target, const QString& reason, int duration);
    bool grpcUnbanUser(const QString& target);
    bool grpcKickUser(const QString& target, const QString& reason);
    bool grpcBroadcast(const QString& message, bool isSystem);
    bool grpcListUsers();
    bool grpcTerminateAll(const QString& reason);
    bool grpcGetAllUsersStatus();  // Tüm kullanıcı durumlarını al

    // ─────────────────────────────────────────────────────────────────────────
    // UI BİLEŞENLERİ
    // ─────────────────────────────────────────────────────────────────────────
    
    // Ana widget'lar
    QStackedWidget* stackedWidget;      // Login ve Chat sayfaları arasında geçiş
    QWidget* loginPage;                 // Giriş sayfası
    QWidget* chatPage;                  // Chat sayfası
    
    // Login sayfası bileşenleri
    QLineEdit* usernameEdit;            // Kullanıcı adı girişi
    QLineEdit* passwordEdit;            // Şifre girişi
    QLineEdit* emailEdit;               // E-posta (opsiyonel)
    QLineEdit* serverAddressEdit;       // Sunucu adresi
    QSpinBox* grpcPortSpin;             // gRPC port
    QSpinBox* tcpPortSpin;              // TCP port
    QPushButton* loginButton;           // Giriş butonu
    QPushButton* registerButton;        // Kayıt ol butonu
    QLabel* loginStatusLabel;           // Giriş durumu etiketi
    
    // Chat sayfası bileşenleri
    QTextEdit* chatDisplay;             // Mesaj görüntüleme alanı
    QLineEdit* messageEdit;             // Mesaj yazma alanı
    QPushButton* sendButton;            // Gönder butonu
    QPushButton* logoutButton;          // Çıkış butonu
    QLabel* connectionStatusLabel;      // Bağlantı durumu
    QLabel* userInfoLabel;              // Kullanıcı bilgisi
    
    // Admin paneli bileşenleri
    QTabWidget* adminTabs;              // Admin sekmeleri
    QGroupBox* adminPanel;              // Admin panel grubu
    
    // Yetki değiştirme
    QComboBox* permTargetCombo;         // Hedef kullanıcı (searchable dropdown)
    QComboBox* permLevelCombo;          // Yeni yetki seviyesi
    QPushButton* changePermButton;      // Değiştir butonu
    
    // Ban işlemleri
    QComboBox* banTargetCombo;          // Banlanacak kullanıcı (searchable dropdown)
    QLineEdit* banReasonEdit;           // Ban sebebi
    QSpinBox* banDurationSpin;          // Ban süresi
    QPushButton* banButton;             // Banla butonu
    QPushButton* unbanButton;           // Unban butonu
    
    // Kick işlemi
    QComboBox* kickTargetCombo;         // Atılacak kullanıcı (searchable dropdown)
    QLineEdit* kickReasonEdit;          // Atılma sebebi
    QPushButton* kickButton;            // Kick butonu
    
    // Broadcast
    QLineEdit* broadcastEdit;           // Broadcast mesajı
    QCheckBox* systemMsgCheck;          // Sistem mesajı mı?
    QPushButton* broadcastButton;       // Gönder butonu
    
    // Kullanıcı listesi
    QListWidget* userListWidget;        // Aktif kullanıcılar
    QPushButton* refreshUsersButton;    // Yenile butonu
    
    // Tehlikeli işlemler
    QPushButton* terminateAllButton;    // Tüm oturumları kapat
    
    // ─────────────────────────────────────────────────────────────────────────
    // KULLANICI DURUMLARI SEKMESİ (Herkes görebilir)
    // ─────────────────────────────────────────────────────────────────────────
    QListWidget* onlineUsersList;       // Online kullanıcılar listesi
    QListWidget* offlineUsersList;      // Offline kullanıcılar listesi
    QLabel* onlineCountLabel;           // Online sayısı etiketi
    QLabel* offlineCountLabel;          // Offline sayısı etiketi
    QLabel* totalCountLabel;            // Toplam kullanıcı sayısı
    QPushButton* refreshStatusButton;   // Yenile butonu
    QTimer* statusRefreshTimer;         // Otomatik yenileme timer
    
    // ─────────────────────────────────────────────────────────────────────────
    // BAĞLANTI VERİLERİ
    // ─────────────────────────────────────────────────────────────────────────
    QTcpSocket* tcpSocket;              // TCP soket bağlantısı
    std::shared_ptr<grpc::Channel> grpcChannel;  // gRPC kanal
    std::unique_ptr<auth::v1::AuthService::Stub> authStub;    // Auth stub
    std::unique_ptr<auth::v1::AdminService::Stub> adminStub;  // Admin stub
    QFutureWatcher<LoginResult> loginWatcher;  // Async login watcher
    
    // ─────────────────────────────────────────────────────────────────────────
    // OTURUM VERİLERİ
    // ─────────────────────────────────────────────────────────────────────────
    QString currentToken;               // Aktif oturum token'ı
    QString currentUsername;            // Aktif kullanıcı adı
    ClientPermission currentPermission; // Aktif yetki seviyesi
    bool isConnected;                   // Bağlantı durumu
};

#endif // MAINWINDOW_HPP
