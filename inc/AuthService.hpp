#pragma once

#include <grpcpp/grpcpp.h>
#include "auth.grpc.pb.h"
#include "TokenManager.hpp"
#include "DataBaseManager.hpp"
#include <mutex>
#include <vector>
#include <condition_variable>

// Kod kalabalığını önlemek için using tanımları
using auth::v1::AuthService;
using auth::v1::LoginRequest;
using auth::v1::LoginResponse;
using auth::v1::RegisterRequest;
using auth::v1::RegisterResponse;
using auth::v1::UserStatusRequest;
using auth::v1::UserStatusUpdate;
using auth::v1::OnlineCountRequest;
using auth::v1::OnlineCountResponse;
using auth::v1::AllUsersStatusRequest;
using auth::v1::AllUsersStatusResponse;
using auth::v1::UserStatusInfo;
using auth::v1::PermissionLevel; 
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

class AuthServiceImp final : public AuthService::Service
{
private:
    TokenManager& token_manager;
    DataBaseManager& db_manager;
    
    // Stream yönetimi için
    mutable std::mutex stream_mutex;
    std::vector<ServerWriter<UserStatusUpdate>*> active_streams;
    std::condition_variable stream_cv;
    
    // Status değişikliğini tüm stream'lere bildir
    void notifyAllStreams(const std::string& username, bool is_online);

public:
    AuthServiceImp(TokenManager& tm, DataBaseManager& db) 
        : token_manager(tm), db_manager(db) 
    {
        // TokenManager callback'ini ayarla
        token_manager.setOnStatusChangeCallback(
            [this](const std::string& username, bool is_online) {
                notifyAllStreams(username, is_online);
            }
        );
    }

    // LOGIN METODU
    Status Login(ServerContext* context, const LoginRequest* request, LoginResponse* response) override;
    
    // REGISTER METODU - Yeni kullanıcı kaydı
    Status Register(ServerContext* context, const RegisterRequest* request, RegisterResponse* response) override;
    
    // STREAM USER STATUS - Gerçek zamanlı durum güncellemeleri
    Status StreamUserStatus(ServerContext* context, const UserStatusRequest* request, 
                           ServerWriter<UserStatusUpdate>* writer) override;
    
    // GET ONLINE COUNT - Anlık online kullanıcı sayısı
    Status GetOnlineCount(ServerContext* context, const OnlineCountRequest* request, 
                         OnlineCountResponse* response) override;
    
    // GET ALL USERS STATUS - Tüm kullanıcıların online/offline durumu (herkes görebilir)
    Status GetAllUsersStatus(ServerContext* context, const AllUsersStatusRequest* request,
                            AllUsersStatusResponse* response) override;
};
