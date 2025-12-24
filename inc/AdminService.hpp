#pragma once

#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"
#include "TokenManager.hpp"
#include "DataBaseManager.hpp"
#include <functional>
#include <vector>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// ═══════════════════════════════════════════════════════════════════════════
//                         ADMİN SERVİSİ IMPLEMENTASYONU
// Admin yetkisi gerektiren tüm gRPC işlemlerini yönetir
// ═══════════════════════════════════════════════════════════════════════════

using auth::v1::AdminService;
using auth::v1::PermissionLevel;
using auth::v1::ChangePermissionRequest;
using auth::v1::ChangePermissionResponse;
using auth::v1::BanUserRequest;
using auth::v1::BanUserResponse;
using auth::v1::UnbanUserRequest;
using auth::v1::UnbanUserResponse;
using auth::v1::BroadcastRequest;
using auth::v1::BroadcastResponse;
using auth::v1::PrivateMessageRequest;
using auth::v1::PrivateMessageResponse;
using auth::v1::ListUsersRequest;
using auth::v1::ListUsersResponse;
using auth::v1::GetUserInfoRequest;
using auth::v1::GetUserInfoResponse;
using ProtoUserInfo = auth::v1::UserInfo;  // İsim çakışması önlemek için alias
using auth::v1::KickUserRequest;
using auth::v1::KickUserResponse;
using auth::v1::TerminateAllRequest;
using auth::v1::TerminateAllResponse;
using grpc::ServerContext;
using grpc::Status;

using BroadcastCallback = std::function<int(const std::string& message, bool is_system)>;
using PrivateMessageCallback = std::function<bool(const std::string& username, const std::string& message)>;
using KickCallback = std::function<bool(const std::string& username, const std::string& reason)>;
using PermissionChangeCallback = std::function<void(const std::string& username, Permission new_permission)>;

class AdminServiceImpl final : public AdminService::Service
{
private:
    TokenManager& token_manager;
    DataBaseManager& db_manager;
    BroadcastCallback broadcast_callback;
    PrivateMessageCallback private_message_callback;
    KickCallback kick_callback;
    PermissionChangeCallback permission_change_callback;

    // Private yardımcı metodlar
    std::string getCurrentTimeString();
    PermissionLevel toProtoPermission(Permission perm);
    Permission fromProtoPermission(PermissionLevel perm);
    bool validateAdminToken(const std::string& token, Permission requiredPermission, std::string& errorMsg, std::optional<UserInfo>& outUserInfo);

public:
    explicit AdminServiceImpl(TokenManager& tm, DataBaseManager& db) 
        : token_manager(tm),
          db_manager(db),
          broadcast_callback(nullptr),
          private_message_callback(nullptr),
          kick_callback(nullptr),
          permission_change_callback(nullptr)
    {}

    void setBroadcastCallback(BroadcastCallback cb) { broadcast_callback = cb; }
    void setPrivateMessageCallback(PrivateMessageCallback cb) { private_message_callback = cb; }
    void setKickCallback(KickCallback cb) { kick_callback = cb; }
    void setPermissionChangeCallback(PermissionChangeCallback cb) { permission_change_callback = cb; }

    // RPC Metodları
    Status ChangeUserPermission(ServerContext* context, 
                                const ChangePermissionRequest* request,
                                ChangePermissionResponse* response) override;

    Status BanUser(ServerContext* context,
                   const BanUserRequest* request,
                   BanUserResponse* response) override;

    Status UnbanUser(ServerContext* context,
                     const UnbanUserRequest* request,
                     UnbanUserResponse* response) override;

    Status BroadcastMessage(ServerContext* context,
                            const BroadcastRequest* request,
                            BroadcastResponse* response) override;

    Status SendPrivateMessage(ServerContext* context,
                              const PrivateMessageRequest* request,
                              PrivateMessageResponse* response) override;

    Status ListActiveUsers(ServerContext* context,
                           const ListUsersRequest* request,
                           ListUsersResponse* response) override;

    Status GetUserInfo(ServerContext* context,
                       const GetUserInfoRequest* request,
                       GetUserInfoResponse* response) override;

    Status KickUser(ServerContext* context,
                    const KickUserRequest* request,
                    KickUserResponse* response) override;

    Status TerminateAllSessions(ServerContext* context,
                                 const TerminateAllRequest* request,
                                 TerminateAllResponse* response) override;
};
