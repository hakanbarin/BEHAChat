#include "DataBaseManager.hpp"
#include <iostream>
#include <functional>  // std::hash için

// ═══════════════════════════════════════════════════════════════════════════
//                         YARDIMCI FONKSİYONLAR
// ═══════════════════════════════════════════════════════════════════════════

// Basit şifre hash'leme (gerçek uygulamada bcrypt veya argon2 kullanın)
static std::string hashPassword(const std::string& password)
{
    std::hash<std::string> hasher;
    return std::to_string(hasher(password));
}

static Permission intToPermission(int perm)
{
    switch(perm)
    {
        case 0: return Permission::ADMIN;
        case 1: return Permission::MODERATOR;
        case 2: return Permission::USER;
        case 3: return Permission::GUEST;
        case 4: return Permission::BANNED;
        default: return Permission::USER;
    }
}

static int permissionToInt(Permission perm)
{
    return static_cast<int>(perm);
}

// ═══════════════════════════════════════════════════════════════════════════
//                         CONSTRUCTOR / DESTRUCTOR
// ═══════════════════════════════════════════════════════════════════════════
DataBaseManager::DataBaseManager() : is_connected(false)
{
    std::cout << "[DataBaseManager] ==========================================" << std::endl;
    std::cout << "[DataBaseManager] PostgreSQL baglantisi kuruluyor..." << std::endl;
    
    try
    {
        // PostgreSQL bağlantı stringi
        // Ortam değişkenlerinden de alınabilir
        std::string conn_str = "host=localhost port=5432 dbname=secure_chat user=postgres password=1234";
        
        std::cout << "[DataBaseManager] Connection string: " << conn_str << std::endl;
        
        conn = std::make_unique<pqxx::connection>(conn_str);
        
        if (conn->is_open())
        {
            is_connected = true;
            std::cout << "[DataBaseManager] BASARILI! PostgreSQL baglandi - DB: " << conn->dbname() << std::endl;
            
            // Tablo var mı kontrol et
            try {
                pqxx::work txn(*conn);
                auto result = txn.exec("SELECT COUNT(*) FROM users");
                txn.commit();
                std::cout << "[DataBaseManager] 'users' tablosu mevcut - Kayitli kullanici: " 
                          << result[0][0].as<int>() << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "[DataBaseManager] UYARI: 'users' tablosu bulunamadi!" << std::endl;
                std::cerr << "[DataBaseManager] Lutfen database/schema/*.sql dosyalarini calistirin." << std::endl;
            }
        }
        else
        {
            std::cerr << "[DataBaseManager] HATA: Baglanti acik degil!" << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] BAGLANTI HATASI: " << e.what() << std::endl;
        std::cerr << "[DataBaseManager] PostgreSQL sunucusunun calistigini kontrol edin:" << std::endl;
        std::cerr << "[DataBaseManager]   sudo systemctl status postgresql" << std::endl;
        std::cerr << "[DataBaseManager]   sudo systemctl start postgresql" << std::endl;
        is_connected = false;
    }
    
    std::cout << "[DataBaseManager] is_connected = " << (is_connected ? "true" : "false") << std::endl;
    std::cout << "[DataBaseManager] ==========================================" << std::endl;
}

DataBaseManager::~DataBaseManager()
{
    if (conn && conn->is_open())
    {
        conn->close();
        std::cout << "[DataBaseManager] Baglanti kapatildi" << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI OLUŞTURMA
// ═══════════════════════════════════════════════════════════════════════════
std::string DataBaseManager::createUser(const std::string& username, const std::string& password, 
                                        const std::string& email, Permission permission)
{
    std::cout << "[DataBaseManager] createUser cagrildi - Username: " << username << std::endl;
    
    if (!is_connected) 
    {
        std::cerr << "[DataBaseManager] HATA: Database baglantisi yok! is_connected=false" << std::endl;
        return "";
    }
    
    if (!conn || !conn->is_open())
    {
        std::cerr << "[DataBaseManager] HATA: Connection objesi gecersiz!" << std::endl;
        return "";
    }
    
    try
    {
        std::cout << "[DataBaseManager] Transaction baslatiliyor..." << std::endl;
        pqxx::work txn(*conn);
        
        std::string password_hash = hashPassword(password);
        std::cout << "[DataBaseManager] Sifre hashlendi" << std::endl;
        
        std::cout << "[DataBaseManager] INSERT sorgusu calistiriliyor..." << std::endl;
        auto result = txn.exec_params(
            "INSERT INTO users (username, password_hash, email, permission, is_online, created_at) "
            "VALUES ($1, $2, $3, $4, false, NOW()) RETURNING id",
            username, password_hash, email, permissionToInt(permission)
        );
        
        std::cout << "[DataBaseManager] Commit yapiliyor..." << std::endl;
        txn.commit();
        
        if (!result.empty())
        {
            std::string user_id = result[0][0].as<std::string>();
            std::cout << "[DataBaseManager] BASARILI! Kullanici olusturuldu - ID: " << user_id 
                      << ", Username: " << username << std::endl;
            return user_id;
        }
        else
        {
            std::cerr << "[DataBaseManager] HATA: INSERT sonucu bos!" << std::endl;
        }
    }
    catch (const pqxx::sql_error& e)
    {
        std::cerr << "[DataBaseManager] SQL HATASI: " << e.what() << std::endl;
        std::cerr << "[DataBaseManager] Query: " << e.query() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] Kullanici olusturma hatasi: " << e.what() << std::endl;
    }
    
    return "";
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI VAR MI KONTROLÜ
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::userExists(const std::string& username)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT COUNT(*) FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        return result[0][0].as<int>() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] userExists hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI DOĞRULAMA
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::validateUser(const std::string& username, const std::string& password)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        std::string password_hash = hashPassword(password);
        std::cout << "[DataBaseManager] validateUser - username=" << username 
                  << ", hash=" << password_hash << std::endl;
        
        auto result = txn.exec_params(
            "SELECT id, password_hash FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        if (!result.empty())
        {
            std::string stored_hash = result[0][1].as<std::string>();
            bool match = (stored_hash == password_hash);
            std::cout << "[DataBaseManager] validateUser - stored_hash=" << stored_hash 
                      << ", match=" << (match?"true":"false") << std::endl;
            return match;
        }
        
        std::cout << "[DataBaseManager] validateUser - kullanici bulunamadi" << std::endl;
        return false;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] validateUser hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI YETKİSİ AL
// ═══════════════════════════════════════════════════════════════════════════
Permission DataBaseManager::getUserPermission(const std::string& username)
{
    if (!is_connected) return Permission::GUEST;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT permission FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        if (!result.empty())
        {
            return intToPermission(result[0][0].as<int>());
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getUserPermission hatasi: " << e.what() << std::endl;
    }
    
    return Permission::GUEST;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI ID'SİNİ AL
// ═══════════════════════════════════════════════════════════════════════════
int DataBaseManager::getUserId(const std::string& username)
{
    if (!is_connected) return -1;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT id FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        if (!result.empty())
        {
            return result[0][0].as<int>();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getUserId hatasi: " << e.what() << std::endl;
    }
    
    return -1;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TÜM KULLANICILARI GETİR
// ═══════════════════════════════════════════════════════════════════════════
std::vector<DbUserInfo> DataBaseManager::getAllUsers()
{
    std::vector<DbUserInfo> users;
    
    if (!is_connected) return users;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec(
            "SELECT id, username, permission, is_online, created_at, COALESCE(email, '') "
            "FROM users ORDER BY id"
        );
        
        txn.commit();
        
        for (const auto& row : result)
        {
            DbUserInfo info;
            info.id = row[0].as<int>();
            info.username = row[1].as<std::string>();
            info.permission = intToPermission(row[2].as<int>());
            info.is_online = row[3].as<bool>();
            info.created_at = row[4].as<std::string>();
            info.email = row[5].as<std::string>();
            
            users.push_back(info);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getAllUsers hatasi: " << e.what() << std::endl;
    }
    
    return users;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TOPLAM KULLANICI SAYISI
// ═══════════════════════════════════════════════════════════════════════════
int DataBaseManager::getTotalUserCount()
{
    if (!is_connected) return 0;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec("SELECT COUNT(*) FROM users");
        
        txn.commit();
        
        return result[0][0].as<int>();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getTotalUserCount hatasi: " << e.what() << std::endl;
    }
    
    return 0;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ DEĞİŞTİR
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::changePermission(const std::string& username, Permission new_permission)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "UPDATE users SET permission = $1 WHERE username = $2",
            permissionToInt(new_permission), username
        );
        
        txn.commit();
        
        return result.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] changePermission hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN KAYDETME
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::saveToken(const std::string& token, int user_id, Permission permission, 
                                const std::string& ip_address)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        txn.exec_params(
            "INSERT INTO tokens (token, user_id, permission, ip_address, created_at) "
            "VALUES ($1, $2, $3, $4, NOW())",
            token, user_id, permissionToInt(permission), ip_address
        );
        
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] saveToken hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN DOĞRULAMA
// ═══════════════════════════════════════════════════════════════════════════
std::pair<bool, int> DataBaseManager::validateToken(const std::string& token)
{
    if (!is_connected) return {false, -1};
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT user_id FROM tokens WHERE token = $1",
            token
        );
        
        txn.commit();
        
        if (!result.empty())
        {
            return {true, result[0][0].as<int>()};
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] validateToken hatasi: " << e.what() << std::endl;
    }
    
    return {false, -1};
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN SİLME
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::deleteToken(const std::string& token)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "DELETE FROM tokens WHERE token = $1",
            token
        );
        
        txn.commit();
        return result.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] deleteToken hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI BANLAMA
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::banUser(const std::string& username, int banned_by_id, 
                              const std::string& reason, int duration_minutes)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        // Önce kullanıcı yetkisini BANNED yap
        txn.exec_params(
            "UPDATE users SET permission = $1 WHERE username = $2",
            permissionToInt(Permission::BANNED), username
        );
        
        // Ban kaydı ekle
        txn.exec_params(
            "INSERT INTO bans (username, banned_by_id, reason, duration_minutes, created_at) "
            "VALUES ($1, $2, $3, $4, NOW())",
            username, banned_by_id, reason, duration_minutes
        );
        
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] banUser hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         BAN KALDIRMA
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::unbanUser(const std::string& username)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        // Kullanıcı yetkisini USER yap
        txn.exec_params(
            "UPDATE users SET permission = $1 WHERE username = $2",
            permissionToInt(Permission::USER), username
        );
        
        // Ban kaydını sil
        txn.exec_params(
            "DELETE FROM bans WHERE username = $1",
            username
        );
        
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] unbanUser hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI BANLI MI KONTROLÜ
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::isUserBanned(const std::string& username)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT permission FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        if (!result.empty())
        {
            return result[0][0].as<int>() == permissionToInt(Permission::BANNED);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] isUserBanned hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         AKTİVİTE LOGLAMA
// ═══════════════════════════════════════════════════════════════════════════
bool DataBaseManager::logActivity(int user_id, const std::string& action, 
                                  const std::string& details, const std::string& ip_address)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        txn.exec_params(
            "INSERT INTO session_logs (user_id, action, details, ip_address, created_at) "
            "VALUES ($1, $2, $3, $4, NOW())",
            user_id, action, details, ip_address
        );
        
        txn.commit();
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] logActivity hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI LOGLARINI GETİR
// ═══════════════════════════════════════════════════════════════════════════
std::vector<LogEntry> DataBaseManager::getUserLogs(int user_id, int limit)
{
    std::vector<LogEntry> logs;
    
    if (!is_connected) return logs;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT id, action, details, ip_address, created_at "
            "FROM session_logs WHERE user_id = $1 ORDER BY created_at DESC LIMIT $2",
            user_id, limit
        );
        
        txn.commit();
        
        for (const auto& row : result)
        {
            LogEntry entry;
            entry.id = row[0].as<int>();
            entry.action = row[1].as<std::string>();
            entry.details = row[2].as<std::string>();
            entry.ip_address = row[3].as<std::string>();
            entry.created_at = row[4].as<std::string>();
            
            logs.push_back(entry);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getUserLogs hatasi: " << e.what() << std::endl;
    }
    
    return logs;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         ONLINE/OFFLINE DURUM İŞLEMLERİ
// ═══════════════════════════════════════════════════════════════════════════

bool DataBaseManager::setUserOnlineStatus(const std::string& username, bool is_online)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "UPDATE users SET is_online = $1 WHERE username = $2",
            is_online, username
        );
        
        txn.commit();
        
        std::cout << "[DataBaseManager] " << username << " durumu: " 
                  << (is_online ? "ONLINE" : "OFFLINE") << std::endl;
        
        return result.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] setUserOnlineStatus hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

bool DataBaseManager::updateLastLogin(const std::string& username)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "UPDATE users SET last_login = NOW() WHERE username = $1",
            username
        );
        
        txn.commit();
        return result.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] updateLastLogin hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

bool DataBaseManager::updateLastSeen(const std::string& username)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "UPDATE users SET last_seen = NOW() WHERE username = $1",
            username
        );
        
        txn.commit();
        return result.affected_rows() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] updateLastSeen hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

std::vector<DbUserInfo> DataBaseManager::getOnlineUsers()
{
    std::vector<DbUserInfo> users;
    
    if (!is_connected) return users;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec(
            "SELECT id, username, permission, is_online, "
            "COALESCE(TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS'), '') as created_at, "
            "COALESCE(email, '') as email, "
            "COALESCE(TO_CHAR(last_login, 'YYYY-MM-DD HH24:MI:SS'), '') as last_login, "
            "COALESCE(TO_CHAR(last_seen, 'YYYY-MM-DD HH24:MI:SS'), '') as last_seen "
            "FROM users WHERE is_online = true ORDER BY username"
        );
        
        txn.commit();
        
        for (const auto& row : result)
        {
            DbUserInfo info;
            info.id = row[0].as<int>();
            info.username = row[1].as<std::string>();
            info.permission = intToPermission(row[2].as<int>());
            info.is_online = row[3].as<bool>();
            info.created_at = row[4].as<std::string>();
            info.email = row[5].as<std::string>();
            info.last_login = row[6].as<std::string>();
            info.last_seen = row[7].as<std::string>();
            
            users.push_back(info);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getOnlineUsers hatasi: " << e.what() << std::endl;
    }
    
    return users;
}

std::vector<DbUserInfo> DataBaseManager::getOfflineUsers()
{
    std::vector<DbUserInfo> users;
    
    if (!is_connected) return users;
    
    std::lock_guard<std::mutex> lock(db_mutex);
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec(
            "SELECT id, username, permission, is_online, "
            "COALESCE(TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS'), '') as created_at, "
            "COALESCE(email, '') as email, "
            "COALESCE(TO_CHAR(last_login, 'YYYY-MM-DD HH24:MI:SS'), '') as last_login, "
            "COALESCE(TO_CHAR(last_seen, 'YYYY-MM-DD HH24:MI:SS'), '') as last_seen "
            "FROM users WHERE is_online = false ORDER BY last_seen DESC NULLS LAST"
        );
        
        txn.commit();
        
        for (const auto& row : result)
        {
            DbUserInfo info;
            info.id = row[0].as<int>();
            info.username = row[1].as<std::string>();
            info.permission = intToPermission(row[2].as<int>());
            info.is_online = row[3].as<bool>();
            info.created_at = row[4].as<std::string>();
            info.email = row[5].as<std::string>();
            info.last_login = row[6].as<std::string>();
            info.last_seen = row[7].as<std::string>();
            
            users.push_back(info);
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getOfflineUsers hatasi: " << e.what() << std::endl;
    }
    
    return users;
}

bool DataBaseManager::isUserOnline(const std::string& username)
{
    if (!is_connected) return false;
    
    try
    {
        pqxx::work txn(*conn);
        
        auto result = txn.exec_params(
            "SELECT is_online FROM users WHERE username = $1",
            username
        );
        
        txn.commit();
        
        if (!result.empty())
        {
            return result[0][0].as<bool>();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] isUserOnline hatasi: " << e.what() << std::endl;
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         MESAJ İŞLEMLERİ
// ═══════════════════════════════════════════════════════════════════════════

int64_t DataBaseManager::saveMessage(int sender_id, const std::string& sender_username, 
                                      const std::string& message_text, Permission sender_permission,
                                      bool is_system, bool is_private,
                                      int recipient_id, const std::string& recipient_username)
{
    if (!is_connected) return -1;
    
    try
    {
        pqxx::work txn(*conn);
        
        std::string query;
        if (is_private && recipient_id > 0)
        {
            query = "INSERT INTO messages (sender_id, sender_username, message_text, sender_permission, "
                    "is_system, is_private, recipient_id, recipient_username) "
                    "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id";
            
            auto result = txn.exec_params(query,
                sender_id,
                sender_username,
                message_text,
                permissionToInt(sender_permission),
                is_system,
                is_private,
                recipient_id,
                recipient_username
            );
            
            txn.commit();
            
            if (!result.empty())
            {
                return result[0][0].as<int64_t>();
            }
        }
        else
        {
            query = "INSERT INTO messages (sender_id, sender_username, message_text, sender_permission, "
                    "is_system, is_private) "
                    "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id";
            
            auto result = txn.exec_params(query,
                sender_id,
                sender_username,
                message_text,
                permissionToInt(sender_permission),
                is_system,
                is_private
            );
            
            txn.commit();
            
            if (!result.empty())
            {
                return result[0][0].as<int64_t>();
            }
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] saveMessage hatasi: " << e.what() << std::endl;
    }
    
    return -1;
}

std::vector<DataBaseManager::MessageInfo> DataBaseManager::getMessageHistory(int limit, int64_t before_message_id)
{
    std::vector<MessageInfo> messages;
    
    if (!is_connected) return messages;
    
    try
    {
        pqxx::work txn(*conn);
        
        std::string query;
        if (before_message_id > 0)
        {
            query = "SELECT id, sender_id, sender_username, message_text, sender_permission, "
                    "TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at, "
                    "is_system, is_private, recipient_id, COALESCE(recipient_username, '') as recipient_username "
                    "FROM messages "
                    "WHERE is_deleted = false AND is_private = false AND id < $1 "
                    "ORDER BY created_at DESC LIMIT $2";
            
            auto result = txn.exec_params(query, before_message_id, limit);
            
            for (const auto& row : result)
            {
                MessageInfo info;
                info.id = row[0].as<int64_t>();
                info.sender_id = row[1].as<int>();
                info.sender_username = row[2].as<std::string>();
                info.message_text = row[3].as<std::string>();
                info.sender_permission = intToPermission(row[4].as<int>());
                info.created_at = row[5].as<std::string>();
                info.is_system = row[6].as<bool>();
                info.is_private = row[7].as<bool>();
                if (!row[8].is_null())
                {
                    info.recipient_id = row[8].as<int>();
                }
                else
                {
                    info.recipient_id = -1;
                }
                info.recipient_username = row[9].as<std::string>();
                
                messages.push_back(info);
            }
        }
        else
        {
            query = "SELECT id, sender_id, sender_username, message_text, sender_permission, "
                    "TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at, "
                    "is_system, is_private, recipient_id, COALESCE(recipient_username, '') as recipient_username "
                    "FROM messages "
                    "WHERE is_deleted = false AND is_private = false "
                    "ORDER BY created_at DESC LIMIT $1";
            
            auto result = txn.exec_params(query, limit);
            
            for (const auto& row : result)
            {
                MessageInfo info;
                info.id = row[0].as<int64_t>();
                info.sender_id = row[1].as<int>();
                info.sender_username = row[2].as<std::string>();
                info.message_text = row[3].as<std::string>();
                info.sender_permission = intToPermission(row[4].as<int>());
                info.created_at = row[5].as<std::string>();
                info.is_system = row[6].as<bool>();
                info.is_private = row[7].as<bool>();
                if (!row[8].is_null())
                {
                    info.recipient_id = row[8].as<int>();
                }
                else
                {
                    info.recipient_id = -1;
                }
                info.recipient_username = row[9].as<std::string>();
                
                messages.push_back(info);
            }
        }
        
        txn.commit();
        
        // Mesajları ters çevir (en eski en başta)
        std::reverse(messages.begin(), messages.end());
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getMessageHistory hatasi: " << e.what() << std::endl;
    }
    
    return messages;
}

std::vector<DataBaseManager::MessageInfo> DataBaseManager::getPrivateMessages(int user1_id, int user2_id, int limit)
{
    std::vector<MessageInfo> messages;
    
    if (!is_connected) return messages;
    
    try
    {
        pqxx::work txn(*conn);
        
        std::string query = "SELECT id, sender_id, sender_username, message_text, sender_permission, "
                           "TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') as created_at, "
                           "is_system, is_private, recipient_id, COALESCE(recipient_username, '') as recipient_username "
                           "FROM messages "
                           "WHERE is_deleted = false AND is_private = true "
                           "AND ((sender_id = $1 AND recipient_id = $2) OR (sender_id = $2 AND recipient_id = $1)) "
                           "ORDER BY created_at ASC LIMIT $3";
        
        auto result = txn.exec_params(query, user1_id, user2_id, limit);
        
        for (const auto& row : result)
        {
            MessageInfo info;
            info.id = row[0].as<int64_t>();
            info.sender_id = row[1].as<int>();
            info.sender_username = row[2].as<std::string>();
            info.message_text = row[3].as<std::string>();
            info.sender_permission = intToPermission(row[4].as<int>());
            info.created_at = row[5].as<std::string>();
            info.is_system = row[6].as<bool>();
            info.is_private = row[7].as<bool>();
            if (!row[8].is_null())
            {
                info.recipient_id = row[8].as<int>();
            }
            else
            {
                info.recipient_id = -1;
            }
            info.recipient_username = row[9].as<std::string>();
            
            messages.push_back(info);
        }
        
        txn.commit();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[DataBaseManager] getPrivateMessages hatasi: " << e.what() << std::endl;
    }
    
    return messages;
}
