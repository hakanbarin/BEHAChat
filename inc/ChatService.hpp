#pragma once

#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"
#include "auth.pb.h"
#include "TokenManager.hpp"
#include "DataBaseManager.hpp"
#include <mutex>
#include <unordered_map>
#include <memory>
#include <thread>
#include <queue>
#include <atomic>

using auth::v1::ChatService;
using auth::v1::ChatMessage;
using auth::v1::MessageHistoryRequest;
using auth::v1::MessageHistoryResponse;
using auth::v1::UserPrivateMessageRequest;
using auth::v1::UserPrivateMessageResponse;
using auth::v1::PermissionLevel;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerReaderWriter;

// ═══════════════════════════════════════════════════════════════════════════
//                         CHAT SERVİSİ IMPLEMENTASYONU
// Bidirectional streaming ile gerçek zamanlı mesajlaşma
// ═══════════════════════════════════════════════════════════════════════════

class ChatServiceImpl final : public ChatService::Service
{
private:
    TokenManager& token_manager;
    DataBaseManager& db_manager;
    
    // Aktif chat stream'lerini takip et (token -> stream writer)
    std::unordered_map<std::string, ServerReaderWriter<ChatMessage, ChatMessage>*> active_streams;
    std::mutex streams_mutex;
    
    // Mesaj kuyruğu (her kullanıcı için)
    struct MessageQueue {
        std::queue<ChatMessage> messages;
        std::mutex mutex;
    };
    std::unordered_map<std::string, std::unique_ptr<MessageQueue>> message_queues;
    
    // Yardımcı metodlar
    std::string getCurrentTimeString();
    PermissionLevel toProtoPermission(Permission perm);
    bool validateToken(const std::string& token, std::optional<UserInfo>& outUserInfo);
    void broadcastToAll(const ChatMessage& message, const std::string& exclude_token = "");
    void sendToUser(const std::string& target_username, const ChatMessage& message);

public:
    explicit ChatServiceImpl(TokenManager& tm, DataBaseManager& db) 
        : token_manager(tm),
          db_manager(db)
    {}

    // RPC Metodları
    Status ChatStream(ServerContext* context,
                     ServerReaderWriter<ChatMessage, ChatMessage>* stream) override;

    Status GetMessageHistory(ServerContext* context,
                            const MessageHistoryRequest* request,
                            MessageHistoryResponse* response) override;

    Status SendPrivateMessage(ServerContext* context,
                             const UserPrivateMessageRequest* request,
                             UserPrivateMessageResponse* response) override;
    
    // Yetki değişikliği bildirimi (AdminService'den çağrılır)
    void notifyPermissionChange(const std::string& username, Permission new_permission);
};

