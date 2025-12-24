#ifndef DATABASEMANAGER_HPP
#define DATABASEMANAGER_HPP

#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <mutex>
#include "TokenManager.hpp"  // Permission enum için

// ═══════════════════════════════════════════════════════════════════════════
//                         YARDIMCI YAPILAR (Structs)
// ═══════════════════════════════════════════════════════════════════════════

// Database'den gelen kullanıcı bilgisi (TokenManager'daki UserInfo'dan farklı)
struct DbUserInfo {
    int id;
    std::string username;
    Permission permission;
    bool is_online;
    std::string created_at;
    std::string email;
    std::string last_login;
    std::string last_seen;
};

// Log kaydı
struct LogEntry {
    int id;
    std::string action;
    std::string details;
    std::string ip_address;
    std::string created_at;
};

// ═══════════════════════════════════════════════════════════════════════════
//                         DATABASE MANAGER SINIFI
// ═══════════════════════════════════════════════════════════════════════════

class DataBaseManager {
private:
    std::unique_ptr<pqxx::connection> conn;  // PostgreSQL bağlantısı
    bool is_connected;
    mutable std::mutex db_mutex;  // Thread-safety için mutex

public:
    // Constructor & Destructor
    DataBaseManager();
    ~DataBaseManager();
    
    // Bağlantı kontrolü
    bool isConnected() const { return is_connected; }

    // ───────────────────────────────────────────────────────────────────────
    // KULLANICI İŞLEMLERİ (users tablosu)
    // ───────────────────────────────────────────────────────────────────────
    
    // Kullanıcı oluştur - başarılıysa user_id döndürür
    std::string createUser(const std::string& username, const std::string& password, 
                           const std::string& email, Permission permission);
    
    // Kullanıcı var mı kontrolü
    bool userExists(const std::string& username);
    
    // Kullanıcı doğrulama (giriş için)
    bool validateUser(const std::string& username, const std::string& password);
    
    // Kullanıcı yetkisi al
    Permission getUserPermission(const std::string& username);
    
    // Kullanıcı ID'sini al (username'den)
    int getUserId(const std::string& username);
    
    // Tüm kullanıcıları getir
    std::vector<DbUserInfo> getAllUsers();
    
    // Toplam kullanıcı sayısı
    int getTotalUserCount();
    
    // Yetki değiştir
    bool changePermission(const std::string& username, Permission new_permission);

    // ───────────────────────────────────────────────────────────────────────
    // ONLINE/OFFLINE DURUM İŞLEMLERİ
    // ───────────────────────────────────────────────────────────────────────
    
    // Kullanıcı online durumunu güncelle
    bool setUserOnlineStatus(const std::string& username, bool is_online);
    
    // Son giriş zamanını güncelle
    bool updateLastLogin(const std::string& username);
    
    // Son görülme zamanını güncelle
    bool updateLastSeen(const std::string& username);
    
    // Online kullanıcıları getir
    std::vector<DbUserInfo> getOnlineUsers();
    
    // Offline kullanıcıları getir
    std::vector<DbUserInfo> getOfflineUsers();
    
    // Kullanıcı online mi kontrol et
    bool isUserOnline(const std::string& username);

    // ───────────────────────────────────────────────────────────────────────
    // TOKEN İŞLEMLERİ (tokens tablosu)
    // ───────────────────────────────────────────────────────────────────────
    bool saveToken(const std::string& token, int user_id, Permission permission, const std::string& ip_address);
    std::pair<bool, int> validateToken(const std::string& token);
    bool deleteToken(const std::string& token);

    // ───────────────────────────────────────────────────────────────────────
    // BAN İŞLEMLERİ (bans tablosu)
    // ───────────────────────────────────────────────────────────────────────
    bool banUser(const std::string& username, int banned_by_id, const std::string& reason, int duration_minutes);
    bool unbanUser(const std::string& username);
    bool isUserBanned(const std::string& username);

    // ───────────────────────────────────────────────────────────────────────
    // LOG İŞLEMLERİ (session_logs tablosu)
    // ───────────────────────────────────────────────────────────────────────
    bool logActivity(int user_id, const std::string& action, const std::string& details, const std::string& ip_address);
    std::vector<LogEntry> getUserLogs(int user_id, int limit = 100);

    // ───────────────────────────────────────────────────────────────────────
    // MESAJ İŞLEMLERİ (messages tablosu)
    // ───────────────────────────────────────────────────────────────────────
    
    // Mesaj kaydet
    int64_t saveMessage(int sender_id, const std::string& sender_username, 
                       const std::string& message_text, Permission sender_permission,
                       bool is_system = false, bool is_private = false,
                       int recipient_id = -1, const std::string& recipient_username = "");
    
    // Mesaj geçmişi getir
    struct MessageInfo {
        int64_t id;
        int sender_id;
        std::string sender_username;
        std::string message_text;
        Permission sender_permission;
        std::string created_at;
        bool is_system;
        bool is_private;
        int recipient_id;
        std::string recipient_username;
    };
    
    std::vector<MessageInfo> getMessageHistory(int limit = 50, int64_t before_message_id = -1);
    std::vector<MessageInfo> getPrivateMessages(int user1_id, int user2_id, int limit = 50);
};

#endif // DATABASEMANAGER_HPPs