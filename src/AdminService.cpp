#include "AdminService.hpp"
#include <set>

// ═══════════════════════════════════════════════════════════════════════════
//                         YARDIMCI METODLAR
// ═══════════════════════════════════════════════════════════════════════════

std::string AdminServiceImpl::getCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

PermissionLevel AdminServiceImpl::toProtoPermission(Permission perm)
{
    switch(perm)
    {
        case Permission::ADMIN:     return PermissionLevel::ADMIN;
        case Permission::MODERATOR: return PermissionLevel::MODERATOR;
        case Permission::USER:      return PermissionLevel::USER;
        case Permission::GUEST:     return PermissionLevel::GUEST;
        case Permission::BANNED:    return PermissionLevel::BANNED;
        default:                    return PermissionLevel::BANNED;
    }
}

Permission AdminServiceImpl::fromProtoPermission(PermissionLevel perm)
{
    switch(perm)
    {
        case PermissionLevel::ADMIN:     return Permission::ADMIN;
        case PermissionLevel::MODERATOR: return Permission::MODERATOR;
        case PermissionLevel::USER:      return Permission::USER;
        case PermissionLevel::GUEST:     return Permission::GUEST;
        case PermissionLevel::BANNED:    return Permission::BANNED;
        default:                         return Permission::BANNED;
    }
}

bool AdminServiceImpl::validateAdminToken(const std::string& token, Permission requiredPermission, std::string& errorMsg, std::optional<UserInfo>& outUserInfo)
{
    // Token'a ait kullanıcı bilgisini al
    auto userInfo = token_manager.getTokenInfo(token);
    
    if (!userInfo || !userInfo->isValid())
    {
        errorMsg = "Gecersiz veya suresi dolmus token";
        return false;
    }

    // Yetki yeterli mi?
    if (!token_manager.hasPermission(token, requiredPermission))
    {
        errorMsg = "Yetersiz yetki. Gerekli: " + std::to_string(static_cast<int>(requiredPermission)) 
                 + ", Mevcut: " + std::to_string(static_cast<int>(userInfo->permission));
        return false;
    }

    outUserInfo = userInfo;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ DEĞİŞTİRME RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::ChangeUserPermission(ServerContext* context, 
                            const ChangePermissionRequest* request,
                            ChangePermissionResponse* response)
{
    std::cout << "[AdminService] ChangeUserPermission istegi alindi - Hedef: " << request->target_username() << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::ADMIN, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    // Önce database'den kullanıcı var mı kontrol et
    if (!db_manager.userExists(request->target_username()))
    {
        response->set_success(false);
        response->set_message("Kullanici bulunamadi: " + request->target_username());
        return Status::OK;
    }

    // Eski yetkiyi database'den al
    Permission oldPerm = db_manager.getUserPermission(request->target_username());
    Permission newPerm = fromProtoPermission(request->new_permission());
    
    // Database'de yetkiyi güncelle
    if (!db_manager.changePermission(request->target_username(), newPerm))
    {
        response->set_success(false);
        response->set_message("Database hatasi: Yetki degistirilemedi");
        return Status::OK;
    }
    
    // Eğer kullanıcı online ise, TokenManager'daki yetkisini de güncelle
    auto targetInfo = token_manager.getTokenInfoByUsername(request->target_username());
    if (targetInfo && targetInfo->isValid())
    {
        token_manager.setPermission(targetInfo->token, newPerm);
    }

    // ChatService'e yetki değişikliği bildirimi gönder
    if (permission_change_callback)
    {
        permission_change_callback(request->target_username(), newPerm);
    }

    response->set_success(true);
    response->set_message("Yetki degistirildi: " + request->target_username());
    response->set_old_permission(toProtoPermission(oldPerm));
    response->set_new_permission(request->new_permission());

    std::cout << "[AdminService] " << request->target_username() 
              << " yetkisi degistirildi: " << static_cast<int>(oldPerm) 
              << " -> " << static_cast<int>(newPerm) << std::endl;

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KULLANICI BANLAMA RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::BanUser(ServerContext* context,
               const BanUserRequest* request,
               BanUserResponse* response)
{
    std::cout << "[AdminService] BanUser istegi - Hedef: " << request->target_username() << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    // Önce database'den kullanıcı var mı kontrol et
    if (!db_manager.userExists(request->target_username()))
    {
        response->set_success(false);
        response->set_message("Kullanici bulunamadi: " + request->target_username());
        return Status::OK;
    }

    // Mevcut yetkiyi database'den al
    Permission targetPerm = db_manager.getUserPermission(request->target_username());

    // Admin banlanamaz
    if (targetPerm == Permission::ADMIN)
    {
        response->set_success(false);
        response->set_message("ADMIN kullanicilar banlanamaz!");
        return Status::OK;
    }

    // Moderator sadece USER ve GUEST banlayabilir
    if (adminInfo->permission == Permission::MODERATOR && targetPerm <= Permission::MODERATOR)
    {
        response->set_success(false);
        response->set_message("MODERATOR sadece USER ve GUEST kullanicilari banlayabilir");
        return Status::OK;
    }

    // Database'de ban yap
    if (!db_manager.changePermission(request->target_username(), Permission::BANNED))
    {
        response->set_success(false);
        response->set_message("Database hatasi: Ban islemi basarisiz");
        return Status::OK;
    }
    
    // Eğer kullanıcı online ise, TokenManager'daki yetkisini de güncelle ve kick et
    auto targetInfo = token_manager.getTokenInfoByUsername(request->target_username());
    if (targetInfo && targetInfo->isValid())
    {
        token_manager.setPermission(targetInfo->token, Permission::BANNED);
        
        if (kick_callback)
        {
            kick_callback(targetInfo->username, "Banlandiniz: " + request->reason());
        }
    }
    
    std::string banInfo = "Ban sebebi: " + request->reason() 
                        + " | Sure: " + std::to_string(request->duration_minutes()) + " dk"
                        + " | Tarih: " + getCurrentTimeString();
    
    std::cout << "[AdminService] " << request->target_username() << " BANLANDI - " << banInfo << std::endl;

    response->set_success(true);
    response->set_message("Kullanici banlandi: " + request->target_username() + " - " + request->reason());

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         BAN KALDIRMA RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::UnbanUser(ServerContext* context,
                 const UnbanUserRequest* request,
                 UnbanUserResponse* response)
{
    std::cout << "[AdminService] UnbanUser istegi - Hedef: " << request->target_username() << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    // Önce database'den kullanıcı var mı kontrol et
    if (!db_manager.userExists(request->target_username()))
    {
        response->set_success(false);
        response->set_message("Kullanici bulunamadi: " + request->target_username());
        return Status::OK;
    }

    // Mevcut yetkiyi database'den al
    Permission targetPerm = db_manager.getUserPermission(request->target_username());

    if (targetPerm != Permission::BANNED)
    {
        response->set_success(false);
        response->set_message("Kullanici zaten banli degil: " + request->target_username());
        return Status::OK;
    }

    // Database'de ban kaldır (USER yetkisi ver)
    if (!db_manager.changePermission(request->target_username(), Permission::USER))
    {
        response->set_success(false);
        response->set_message("Database hatasi: Unban islemi basarisiz");
        return Status::OK;
    }
    
    // Eğer kullanıcı online ise, TokenManager'daki yetkisini de güncelle
    auto targetInfo = token_manager.getTokenInfoByUsername(request->target_username());
    if (targetInfo && targetInfo->isValid())
    {
        token_manager.setPermission(targetInfo->token, Permission::USER);
    }

    std::cout << "[AdminService] " << request->target_username() << " BANI KALDIRILDI" << std::endl;

    response->set_success(true);
    response->set_message("Ban kaldirildi: " + request->target_username());

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         BROADCAST MESAJ RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::BroadcastMessage(ServerContext* context,
                        const BroadcastRequest* request,
                        BroadcastResponse* response)
{
    std::cout << "[AdminService] BroadcastMessage istegi alindi" << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    if (!broadcast_callback)
    {
        response->set_success(false);
        response->set_message("Broadcast sistemi hazir degil");
        return Status::OK;
    }

    std::string formattedMessage;
    if (request->is_system_message())
    {
        formattedMessage = "[SISTEM] " + request->message();
    }
    else
    {
        formattedMessage = "[DUYURU - " + adminInfo->username + "] " + request->message();
    }

    int recipients = broadcast_callback(formattedMessage, request->is_system_message());

    std::cout << "[AdminService] Broadcast gonderildi - Alici sayisi: " << recipients << std::endl;

    response->set_success(true);
    response->set_message("Mesaj yayinlandi");
    response->set_recipients_count(recipients);

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         ÖZEL MESAJ RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::SendPrivateMessage(ServerContext* context,
                          const PrivateMessageRequest* request,
                          PrivateMessageResponse* response)
{
    std::cout << "[AdminService] SendPrivateMessage istegi - Hedef: " << request->target_username() << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    if (!private_message_callback)
    {
        response->set_success(false);
        response->set_message("Ozel mesaj sistemi hazir degil");
        return Status::OK;
    }

    std::string formattedMessage = "[OZEL - " + adminInfo->username + "] " + request->message();

    bool sent = private_message_callback(request->target_username(), formattedMessage);

    if (sent)
    {
        response->set_success(true);
        response->set_message("Mesaj gonderildi: " + request->target_username());
        std::cout << "[AdminService] Ozel mesaj gonderildi" << std::endl;
    }
    else
    {
        response->set_success(false);
        response->set_message("Mesaj gonderilemedi - Kullanici cevrimdisi olabilir");
    }

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         AKTİF KULLANICI LİSTELEME RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::ListActiveUsers(ServerContext* context,
                       const ListUsersRequest* request,
                       ListUsersResponse* response)
{
    std::cout << "[AdminService] ListActiveUsers istegi alindi" << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    // Database'den TÜM kullanıcıları al (online/offline fark etmez)
    auto dbUsers = db_manager.getAllUsers();
    
    // Filtreleme: online_or_offline parametresine göre
    auto activeTokens = token_manager.getAllActiveUsers();
    std::set<std::string> onlineUsernames;
    for (const auto& user : activeTokens)
    {
        onlineUsernames.insert(user.username);
    }

    for (const auto& dbUser : dbUsers)
    {
        // Online/Offline durumunu belirle
        bool isOnline = onlineUsernames.count(dbUser.username) > 0;
        
        // Filtreleme kurallarını kontrol et
        if (request->online_or_offline() == auth::v1::OnlineOrOfflineCheck::ONLY_ONLINE && !isOnline)
            continue;
        if (request->online_or_offline() == auth::v1::OnlineOrOfflineCheck::ONLY_OFFLINE && isOnline)
            continue;
        
        // Banlıları filtreleme
        if (!request->with_banned_person() && dbUser.permission == Permission::BANNED)
            continue;
        
        // Response'a ekle
        ProtoUserInfo* info = response->add_users();
        info->set_username(dbUser.username);
        info->set_permission(toProtoPermission(dbUser.permission));
        info->set_is_online(isOnline);
        info->set_last_activity(dbUser.last_seen);
    }

    response->set_success(true);
    response->set_message("Toplam " + std::to_string(response->users_size()) + " kullanici listelendi");

    std::cout << "[AdminService] " << response->users_size() << " kullanici listelendi" << std::endl;

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TEK KULLANICI BİLGİSİ RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::GetUserInfo(ServerContext* context,
                   const GetUserInfoRequest* request,
                   GetUserInfoResponse* response)
{
    std::cout << "[AdminService] GetUserInfo istegi - Hedef: " << request->target_username() << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    auto targetInfo = token_manager.getTokenInfoByUsername(request->target_username());

    if (!targetInfo || !targetInfo->isValid())
    {
        response->set_success(false);
        response->set_message("Kullanici bulunamadi: " + request->target_username());
        return Status::OK;
    }

    ProtoUserInfo* info = response->mutable_user();
    info->set_username(targetInfo->username);
    info->set_permission(toProtoPermission(targetInfo->permission));
    info->set_is_online(targetInfo->is_online);
    info->set_last_activity(getCurrentTimeString());

    response->set_success(true);
    response->set_message("Kullanici bilgisi alindi");

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         KICK RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::KickUser(ServerContext* context,
                const KickUserRequest* request,
                KickUserResponse* response)
{
    std::cout << "[AdminService] KickUser istegi - Hedef: " << request->target_username() << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::MODERATOR, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    auto targetInfo = token_manager.getTokenInfoByUsername(request->target_username());

    if (!targetInfo || !targetInfo->isValid())
    {
        response->set_success(false);
        response->set_message("Kullanici bulunamadi veya cevrimdisi: " + request->target_username());
        return Status::OK;
    }

    // Admin kicklenemez
    if (targetInfo->permission == Permission::ADMIN)
    {
        response->set_success(false);
        response->set_message("ADMIN kullanicilar kicklenemez!");
        return Status::OK;
    }

    // Moderator sadece düşük yetkilileri kickleyebilir
    if (adminInfo->permission == Permission::MODERATOR && targetInfo->permission <= Permission::MODERATOR)
    {
        response->set_success(false);
        response->set_message("MODERATOR sadece USER ve GUEST kullanicilari kickleyebilir");
        return Status::OK;
    }

    if (kick_callback)
    {
        bool kicked = kick_callback(targetInfo->username, request->reason());
        if (kicked)
        {
            response->set_success(true);
            response->set_message("Kullanici atildi: " + targetInfo->username);
            std::cout << "[AdminService] " << targetInfo->username << " KICKLENDI" << std::endl;
        }
        else
        {
            response->set_success(false);
            response->set_message("Kick islemi basarisiz");
        }
    }
    else
    {
        token_manager.removeSession(targetInfo->token);
        response->set_success(true);
        response->set_message("Kullanici oturumu sonlandirildi: " + targetInfo->username);
    }

    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         TÜM OTURUMLARI SONLANDIRMA RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status AdminServiceImpl::TerminateAllSessions(ServerContext* context,
                             const TerminateAllRequest* request,
                             TerminateAllResponse* response)
{
    std::cout << "[AdminService] TerminateAllSessions istegi alindi" << std::endl;

    std::optional<UserInfo> adminInfo;
    std::string errorMsg;
    if (!validateAdminToken(request->admin_token(), Permission::ADMIN, errorMsg, adminInfo))
    {
        response->set_success(false);
        response->set_message(errorMsg);
        return Status::OK;
    }

    if (broadcast_callback)
    {
        std::string msg = "[SISTEM] Sunucu bakim modu - Tum oturumlar sonlandiriliyor. Sebep: " + request->reason();
        broadcast_callback(msg, true);
    }

    // Admin token'ı hariç tümünü sonlandır
    int terminated = token_manager.terminateAllExcept(adminInfo->token);

    response->set_success(true);
    response->set_message("Tum oturumlar sonlandirildi");
    response->set_terminated_count(terminated);

    std::cout << "[AdminService] " << terminated << " oturum sonlandirildi - Sebep: " << request->reason() << std::endl;

    return Status::OK;
}
