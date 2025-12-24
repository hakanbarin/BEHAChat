#include "TokenManager.hpp"
#include <algorithm>
#include <chrono>

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN STRING ÜRETİMİ
// ═══════════════════════════════════════════════════════════════════════════
std::string TokenManager::generateTokenString()
{
    thread_local std::mt19937 generator(std::random_device{}());
    static const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::uniform_int_distribution<> distribution(0, sizeof(alphanum) - 2);
    
    std::string token(32, '\0');
    std::generate_n(token.begin(), token.size(), 
                    [&](){ return alphanum[distribution(generator)]; });
    
    return token;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         OTURUM OLUŞTURMA
// ═══════════════════════════════════════════════════════════════════════════
UserInfo TokenManager::createSession(const std::string& username, Permission permission)
{
    std::lock_guard<std::mutex> lock(mutex);

    std::string token_str = generateTokenString();
    
    UserInfo person;
    person.token = token_str;
    person.username = username;
    person.permission = permission;
    person.is_online = true;

    active_tokens[token_str] = person;

    std::cout << "[TokenManager] Oturum olusturuldu - Token: " << token_str 
              << ", Kullanici: " << username 
              << ", Yetki: " << static_cast<int>(permission) << std::endl;

    // Status callback'i DEVRE DIŞI - deadlock'a neden oluyordu
    // if (on_status_change) {
    //     on_status_change(username, true);
    // }

    return person;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ SEVİYESİ AYARLAMA
// ═══════════════════════════════════════════════════════════════════════════
void TokenManager::setPermission(const std::string& token, Permission newPermission)
{
    std::lock_guard<std::mutex> lock(mutex);

    auto it = active_tokens.find(token); 
    
    if (it != active_tokens.end())
    {
        it->second.permission = newPermission;
        std::cout << "[TokenManager] Yetki degistirildi - Kullanici: " << it->second.username
                  << ", Yeni yetki: " << static_cast<int>(newPermission) << std::endl;
    }
    else 
    {
        std::cout << "[TokenManager] Token bulunamadi: " << token << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN GEÇERLİLİK KONTROLÜ
// ═══════════════════════════════════════════════════════════════════════════
bool TokenManager::isValid(const std::string& token) const
{
    std::lock_guard<std::mutex> lock(mutex);
    return active_tokens.contains(token);
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ KONTROL METODU
// ═══════════════════════════════════════════════════════════════════════════
bool TokenManager::hasPermission(const std::string& token, Permission requiredPermission) const
{
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = active_tokens.find(token);
    if (it == active_tokens.end())
        return false;
    
    return it->second.permission <= requiredPermission;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         OTURUM SİLME
// ═══════════════════════════════════════════════════════════════════════════
void TokenManager::removeSession(const std::string& token)
{
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = active_tokens.find(token);
    
    if (it != active_tokens.end())
    {
        std::string deletedUser = it->second.username;
        active_tokens.erase(it);
        
        std::cout << "[TokenManager] Oturum silindi - Kullanici: " << deletedUser << std::endl;
        
        // Status callback'i DEVRE DIŞI - deadlock'a neden oluyordu
        // if (on_status_change) {
        //     on_status_change(deletedUser, false);
        // }
    }
    else
    {
        std::cout << "[TokenManager] Silinecek oturum bulunamadi." << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TOKEN BİLGİSİ ALMA
// ═══════════════════════════════════════════════════════════════════════════
std::optional<UserInfo> TokenManager::getTokenInfo(const std::string& token) const
{
    if (token.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex);

    auto it = active_tokens.find(token);
    
    if (it != active_tokens.end())
    {
        return it->second;
    }

    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         USERNAME'DEN TOKEN BULMA
// ═══════════════════════════════════════════════════════════════════════════
std::optional<UserInfo> TokenManager::getTokenInfoByUsername(const std::string& username) const
{
    if (username.empty()) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(mutex);

    for (const auto& [token, info] : active_tokens)
    {
        if (info.username == username)
        {
            return info;
        }
    }

    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TÜM AKTİF KULLANICILARI LİSTELEME
// ═══════════════════════════════════════════════════════════════════════════
std::vector<TokenManager::ActiveUserInfo> TokenManager::getAllActiveUsers() const
{
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<ActiveUserInfo> users;
    
    for (const auto& [token, info] : active_tokens)
    {
        ActiveUserInfo activeInfo;
        activeInfo.token = token;
        activeInfo.username = info.username;
        activeInfo.permission = info.permission;
        activeInfo.is_online = info.is_online;
        activeInfo.last_activity = "Cevrimici";
        
        users.push_back(activeInfo);
    }
    
    return users;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         AKTİF KULLANICI SAYISI
// ═══════════════════════════════════════════════════════════════════════════
size_t TokenManager::getActiveUserCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    return active_tokens.size();
}

// ═══════════════════════════════════════════════════════════════════════════
//                         ONLINE KULLANICI SAYISI
// ═══════════════════════════════════════════════════════════════════════════
size_t TokenManager::getOnlineUserCount() const
{
    std::lock_guard<std::mutex> lock(mutex);
    size_t count = 0;
    for (const auto& [token, info] : active_tokens)
    {
        if (info.is_online) count++;
    }
    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TÜM OTURUMLARI SONLANDIRMA
// ═══════════════════════════════════════════════════════════════════════════
int TokenManager::terminateAll()
{
    std::lock_guard<std::mutex> lock(mutex);
    
    int count = active_tokens.size();
    
    // Callback'leri çağır
    if (on_status_change) {
        for (const auto& [token, info] : active_tokens) {
            on_status_change(info.username, false);
        }
    }
    
    active_tokens.clear();

    std::cout << "[TokenManager] Tum oturumlar sonlandirildi: " << count << std::endl;
    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         BELİRLİ TOKEN HARİÇ TÜMÜNÜ SONLANDIR
// ═══════════════════════════════════════════════════════════════════════════
int TokenManager::terminateAllExcept(const std::string& exceptToken)
{
    std::lock_guard<std::mutex> lock(mutex);
    
    int count = 0;
    std::vector<std::string> tokensToRemove;
    
    for (const auto& [token, info] : active_tokens)
    {
        if (token != exceptToken)
        {
            tokensToRemove.push_back(token);
            if (on_status_change) {
                on_status_change(info.username, false);
            }
        }
    }
    
    for (const auto& token : tokensToRemove)
    {
        active_tokens.erase(token);
        count++;
    }
    
    std::cout << "[TokenManager] " << count << " oturum sonlandirildi" << std::endl;
    return count;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI ONLINE KONTROLÜ
// ═══════════════════════════════════════════════════════════════════════════
bool TokenManager::isUserOnline(const std::string& username) const
{
    std::lock_guard<std::mutex> lock(mutex);
    
    for (const auto& [token, info] : active_tokens)
    {
        if (info.username == username && info.is_online)
        {
            return true;
        }
    }
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         ADMIN KONTROLÜ
// ═══════════════════════════════════════════════════════════════════════════
bool TokenManager::isAdmin(const std::string& token) const
{
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = active_tokens.find(token);
    if (it == active_tokens.end())
        return false;
    
    return it->second.permission == Permission::ADMIN;
} 
