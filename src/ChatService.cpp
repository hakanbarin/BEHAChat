#include "ChatService.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include "auth.pb.h"

// ═══════════════════════════════════════════════════════════════════════════
//                         YARDIMCI METODLAR
// ═══════════════════════════════════════════════════════════════════════════

std::string ChatServiceImpl::getCurrentTimeString()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

PermissionLevel ChatServiceImpl::toProtoPermission(Permission perm)
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

bool ChatServiceImpl::validateToken(const std::string& token, std::optional<UserInfo>& outUserInfo)
{
    auto userInfo = token_manager.getTokenInfo(token);
    
    if (!userInfo || !userInfo->isValid())
    {
        return false;
    }

    outUserInfo = userInfo;
    return true;
}

void ChatServiceImpl::broadcastToAll(const ChatMessage& message, const std::string& exclude_token)
{
    std::lock_guard<std::mutex> lock(streams_mutex);
    
    for (auto& [token, stream] : active_streams)
    {
        // Kendi mesajını tekrar gönderme
        if (!exclude_token.empty() && token == exclude_token)
        {
            continue;
        }
        
        stream->Write(message);
    }
}

void ChatServiceImpl::sendToUser(const std::string& target_username, const ChatMessage& message)
{
    std::lock_guard<std::mutex> lock(streams_mutex);
    
    // Hedef kullanıcıyı bul
    for (auto& [token, stream] : active_streams)
    {
        auto userInfo = token_manager.getTokenInfo(token);
        if (userInfo && userInfo->username == target_username)
        {
            stream->Write(message);
            return;
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         CHAT STREAM RPC'Sİ (BIDIRECTIONAL)
// ═══════════════════════════════════════════════════════════════════════════
Status ChatServiceImpl::ChatStream(ServerContext* context,
                                  ServerReaderWriter<ChatMessage, ChatMessage>* stream)
{
    std::cout << "[ChatService] Yeni chat stream baglantisi" << std::endl;
    
    std::string user_token;
    std::optional<UserInfo> userInfo;
    bool authenticated = false;
    
    // İlk mesaj token içermeli (authentication)
    ChatMessage first_message;
    if (!stream->Read(&first_message))
    {
        std::cout << "[ChatService] Stream okunamadi (baglanti kapandi)" << std::endl;
        return Status::OK;
    }
    
    user_token = first_message.token();
    if (!validateToken(user_token, userInfo))
    {
        std::cout << "[ChatService] Gecersiz token" << std::endl;
        ChatMessage error_msg;
        error_msg.set_message("ERR Gecersiz token");
        error_msg.set_is_system(true);
        stream->Write(error_msg);
        return Status::OK;
    }
    
    authenticated = true;
    std::cout << "[ChatService] Kullanici dogrulandi: " << userInfo->username << std::endl;
    
    // BANNED kullanıcı kontrolü
    if (userInfo->permission == Permission::BANNED)
    {
        ChatMessage error_msg;
        error_msg.set_message("ERR Yasakli kullanici");
        error_msg.set_is_system(true);
        stream->Write(error_msg);
        return Status::OK;
    }
    
    // Stream'i kaydet
    {
        std::lock_guard<std::mutex> lock(streams_mutex);
        active_streams[user_token] = stream;
    }
    
    // Mesaj geçmişini gönder (son 20 mesaj)
    auto history = db_manager.getMessageHistory(20);
    for (const auto& msg_info : history)
    {
        ChatMessage history_msg;
        history_msg.set_username(msg_info.sender_username);
        history_msg.set_message(msg_info.message_text);
        history_msg.set_timestamp(msg_info.created_at);
        history_msg.set_permission(toProtoPermission(msg_info.sender_permission));
        history_msg.set_is_system(msg_info.is_system);
        history_msg.set_is_private(false);
        history_msg.set_message_id(msg_info.id);
        stream->Write(history_msg);
    }
    
    // Hoş geldin mesajı
    ChatMessage welcome_msg;
    welcome_msg.set_message("[SISTEM] Chat'e baglandiniz. Mesajlariniz tum kullanicilara gonderilecek.");
    welcome_msg.set_is_system(true);
    welcome_msg.set_timestamp(getCurrentTimeString());
    stream->Write(welcome_msg);
    
    // Mesaj okuma döngüsü
    ChatMessage incoming_message;
    while (stream->Read(&incoming_message))
    {
        // Token kontrolü (her mesajda)
        if (incoming_message.token() != user_token)
        {
            // Token değişmiş, yeniden doğrula
            std::optional<UserInfo> newUserInfo;
            if (!validateToken(incoming_message.token(), newUserInfo))
            {
                ChatMessage error_msg;
                error_msg.set_message("ERR Token gecersiz oldu");
                error_msg.set_is_system(true);
                stream->Write(error_msg);
                break;
            }
            userInfo = newUserInfo;
            user_token = incoming_message.token();
        }
        
        // GUEST kullanıcılar mesaj gönderemez
        if (userInfo->permission == Permission::GUEST)
        {
            ChatMessage error_msg;
            error_msg.set_message("ERR GUEST kullanicilar mesaj gonderemez");
            error_msg.set_is_system(true);
            stream->Write(error_msg);
            continue;
        }
        
        // Mesaj içeriği boş mu kontrol et
        if (incoming_message.message().empty())
        {
            continue;
        }
        
        // Mesajı hazırla
        ChatMessage outgoing_message;
        outgoing_message.set_username(userInfo->username);
        outgoing_message.set_message(incoming_message.message());
        outgoing_message.set_timestamp(getCurrentTimeString());
        outgoing_message.set_permission(toProtoPermission(userInfo->permission));
        outgoing_message.set_is_system(false);
        outgoing_message.set_is_private(incoming_message.is_private());
        
        // Kullanıcı ID'sini al
        int sender_id = db_manager.getUserId(userInfo->username);
        
        if (incoming_message.is_private() && !incoming_message.target_username().empty())
        {
            // Özel mesaj
            outgoing_message.set_target_username(incoming_message.target_username());
            outgoing_message.set_is_private(true);
            
            // Hedef kullanıcının ID'sini bul
            int recipient_id = db_manager.getUserId(incoming_message.target_username());
            
            // Veritabanına kaydet
            int64_t message_id = db_manager.saveMessage(
                sender_id,
                userInfo->username,
                incoming_message.message(),
                userInfo->permission,
                false,  // is_system
                true,   // is_private
                recipient_id,
                incoming_message.target_username()
            );
            outgoing_message.set_message_id(message_id);
            
            // Hedef kullanıcıya gönder
            sendToUser(incoming_message.target_username(), outgoing_message);
            
            // Gönderene de gönder (onay için)
            stream->Write(outgoing_message);
        }
        else
        {
            // Genel mesaj - tüm kullanıcılara yayınla
            outgoing_message.set_is_private(false);
            
            // Veritabanına kaydet
            int64_t message_id = db_manager.saveMessage(
                sender_id,
                userInfo->username,
                incoming_message.message(),
                userInfo->permission,
                false,  // is_system
                false   // is_private
            );
            outgoing_message.set_message_id(message_id);
            
            // Tüm kullanıcılara yayınla (kendi mesajını gönderme)
            broadcastToAll(outgoing_message, user_token);
        }
        
        std::cout << "[ChatService] Mesaj yayinlandi - Kullanici: " << userInfo->username 
                  << ", Mesaj: " << incoming_message.message().substr(0, 50) << std::endl;
    }
    
    // Stream kapanınca kaydı kaldır
    {
        std::lock_guard<std::mutex> lock(streams_mutex);
        active_streams.erase(user_token);
    }
    
    std::cout << "[ChatService] Chat stream kapandi - Kullanici: " << userInfo->username << std::endl;
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         MESAJ GEÇMİŞİ RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status ChatServiceImpl::GetMessageHistory(ServerContext* context,
                                         const MessageHistoryRequest* request,
                                         MessageHistoryResponse* response)
{
    std::cout << "[ChatService] GetMessageHistory istegi alindi" << std::endl;
    
    // Token doğrulama
    std::optional<UserInfo> userInfo;
    if (!validateToken(request->token(), userInfo))
    {
        response->set_success(false);
        response->set_message("Gecersiz token");
        return Status::OK;
    }
    
    int limit = request->limit() > 0 ? request->limit() : 50;
    int64_t before_id = request->before_message_id() > 0 ? request->before_message_id() : -1;
    
    // Mesaj geçmişini al
    auto messages = db_manager.getMessageHistory(limit, before_id);
    
    // Response'a dönüştür
    for (const auto& msg_info : messages)
    {
        ChatMessage* msg = response->add_messages();
        msg->set_username(msg_info.sender_username);
        msg->set_message(msg_info.message_text);
        msg->set_timestamp(msg_info.created_at);
        msg->set_permission(toProtoPermission(msg_info.sender_permission));
        msg->set_is_system(msg_info.is_system);
        msg->set_is_private(msg_info.is_private);
        msg->set_message_id(msg_info.id);
    }
    
    response->set_success(true);
    response->set_message("Mesaj gecmisi getirildi");
    response->set_total_count(static_cast<int32_t>(messages.size()));
    
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         ÖZEL MESAJ RPC'Sİ
// ═══════════════════════════════════════════════════════════════════════════
Status ChatServiceImpl::SendPrivateMessage(ServerContext* context,
                                          const UserPrivateMessageRequest* request,
                                          UserPrivateMessageResponse* response)
{
    std::cout << "[ChatService] SendPrivateMessage istegi alindi" << std::endl;
    
    // Token doğrulama
    std::optional<UserInfo> userInfo;
    if (!validateToken(request->token(), userInfo))
    {
        response->set_success(false);
        response->set_message("Gecersiz token");
        return Status::OK;
    }
    
    // GUEST kullanıcılar mesaj gönderemez
    if (userInfo->permission == Permission::GUEST)
    {
        response->set_success(false);
        response->set_message("GUEST kullanicilar mesaj gonderemez");
        return Status::OK;
    }
    
    // Kullanıcı ID'sini al
    int sender_id = db_manager.getUserId(userInfo->username);
    int recipient_id = db_manager.getUserId(request->target_username());
    
    // Mesaj hazırla
    ChatMessage private_msg;
    private_msg.set_username(userInfo->username);
    private_msg.set_message(request->message());
    private_msg.set_timestamp(getCurrentTimeString());
    private_msg.set_permission(toProtoPermission(userInfo->permission));
    private_msg.set_is_system(false);
    private_msg.set_is_private(true);
    private_msg.set_target_username(request->target_username());
    
    // Veritabanına kaydet
    int64_t message_id = db_manager.saveMessage(
        sender_id,
        userInfo->username,
        request->message(),
        userInfo->permission,
        false,  // is_system
        true,   // is_private
        recipient_id,
        request->target_username()
    );
    private_msg.set_message_id(message_id);
    
    // Hedef kullanıcıya gönder
    sendToUser(request->target_username(), private_msg);
    
    response->set_success(true);
    response->set_message("Ozel mesaj gonderildi");
    
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ DEĞİŞİKLİĞİ BİLDİRİMİ
// ═══════════════════════════════════════════════════════════════════════════
void ChatServiceImpl::notifyPermissionChange(const std::string& username, Permission new_permission)
{
    std::lock_guard<std::mutex> lock(streams_mutex);
    
    // Kullanıcının token'ını bul
    auto userInfo = token_manager.getTokenInfoByUsername(username);
    if (!userInfo || !userInfo->isValid())
    {
        return; // Kullanıcı aktif değil
    }
    
    // Kullanıcının stream'ini bul
    auto it = active_streams.find(userInfo->token);
    if (it == active_streams.end())
    {
        return; // Stream bulunamadı
    }
    
    // Yetki güncelleme mesajı gönder (özel format: PERM_UPDATE:new_permission)
    ChatMessage permission_msg;
    permission_msg.set_message("[SISTEM] Yetkiniz guncellendi: " + 
                               std::to_string(static_cast<int>(new_permission)) + 
                               " | PERM_UPDATE:" + std::to_string(static_cast<int>(new_permission)));
    permission_msg.set_timestamp(getCurrentTimeString());
    permission_msg.set_permission(toProtoPermission(new_permission));
    permission_msg.set_is_system(true);
    permission_msg.set_is_private(false);
    
    it->second->Write(permission_msg);
    
    std::cout << "[ChatService] Yetki guncelleme bildirimi gonderildi - Kullanici: " 
              << username << ", Yeni yetki: " << static_cast<int>(new_permission) << std::endl;
}

