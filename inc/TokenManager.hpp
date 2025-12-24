#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <random>
#include <iostream>
#include <optional>
#include <functional>

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ SEVİYELERİ ENUM'U
// Proto dosyasıyla senkronize edilmiş enum
// Farklı kullanıcı rollerine göre izinleri tanımlıyoruz
// ═══════════════════════════════════════════════════════════════════════════
enum class Permission {
    ADMIN = 0,           // Tüm işlemleri yapabilir
    MODERATOR = 1,       // Chat moderasyonu yapabilir
    USER = 2,            // Normal chat işlemleri
    GUEST = 3,           // Sadece okuma
    BANNED = 4           // Hiç bir şey yapamaz
};

// ═══════════════════════════════════════════════════════════════════════════
//                         UserInfo YAPISI
// Tüm token işlemleri bu yapı üzerinden yapılır
// ═══════════════════════════════════════════════════════════════════════════
struct UserInfo
{
    std::string token;            // Token string'i
    std::string username;         // Kullanıcı adı
    Permission permission;        // Yetki seviyesi
    bool is_online = true;        // Online durumu
    
    bool isEmpty() const { return token.empty(); }
    bool isValid() const { return !token.empty() && permission != Permission::BANNED; }
};

// ═══════════════════════════════════════════════════════════════════════════
//                         Callback Tipleri
// ═══════════════════════════════════════════════════════════════════════════
using OnUserStatusChangeCallback = std::function<void(const std::string& username, bool is_online)>;

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN YÖNETİCİSİ SINIFI
// Tüm oturum ve yetki işlemlerini yönetir
// ═══════════════════════════════════════════════════════════════════════════
class TokenManager
{
private:
    std::unordered_map<std::string, UserInfo> active_tokens;  // token -> UserInfo
    mutable std::mutex mutex;
    
    // Status değişikliği callback'i
    OnUserStatusChangeCallback on_status_change;
    
    std::string generateTokenString();
    
public:
    // ═══════════════════════════════════════════════════════════════════════════
    //                         AKTİF KULLANICI BİLGİLERİ STRUCT
    // ═══════════════════════════════════════════════════════════════════════════
    struct ActiveUserInfo {
        std::string token;
        std::string username;
        Permission permission;
        bool is_online;
        std::string last_activity;
    };

    // Callback ayarlama
    void setOnStatusChangeCallback(OnUserStatusChangeCallback cb) { on_status_change = cb; }

    // OTURUM OLUŞTURMA - Token döndürür
    UserInfo createSession(const std::string& username, Permission permission);

    // YETKİ SEVİYESİ AYARLAMA
    void setPermission(const std::string& token, Permission newPermission);

    // TOKEN GEÇERLİLİK KONTROLÜ
    bool isValid(const std::string& token) const;

    // YETKİ KONTROL METODU
    bool hasPermission(const std::string& token, Permission requiredPermission) const;

    // OTURUM SİLME
    void removeSession(const std::string& token);

    // TOKEN BİLGİSİ ALMA
    std::optional<UserInfo> getTokenInfo(const std::string& token) const;
    
    // USERNAME'DEN TOKEN BULMA
    std::optional<UserInfo> getTokenInfoByUsername(const std::string& username) const;

    // TÜM AKTİF KULLANICILARI LİSTELEME
    std::vector<ActiveUserInfo> getAllActiveUsers() const;

    // AKTİF KULLANICI SAYISI
    size_t getActiveUserCount() const;
    
    // ONLINE KULLANICI SAYISI
    size_t getOnlineUserCount() const;

    // TÜM OTURUMLARI SONLANDIRMA
    int terminateAll();
    
    // BELİRLİ TOKEN HARİÇ TÜMÜNÜ SONLANDIR
    int terminateAllExcept(const std::string& exceptToken);

    // KULLANICI ONLINE KONTROLÜ
    bool isUserOnline(const std::string& username) const;
    
    // ADMIN KONTROLÜ
    bool isAdmin(const std::string& token) const;
};
