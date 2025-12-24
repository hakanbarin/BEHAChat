#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <array>
#include <vector>
#include <system_error>
#include <unistd.h>
#include <sys/socket.h>
#include "TokenManager.hpp"

// SOKET YÖNETICISI SINIFI
// RAII prensibi kullanır
class SocketGuard
{
    int fd_;

public:
    explicit SocketGuard(int fd) : fd_(fd) {}

    ~SocketGuard()
    {
        if (fd_ != -1)
        {
            close(fd_);
            std::cout << "[Socket] Baglanti temizlendi (FD: " << fd_ << ")" << std::endl;
        }
    }

    SocketGuard(const SocketGuard&) = delete;
    SocketGuard& operator=(const SocketGuard&) = delete;

    int get() const { return fd_; }
};

// Forward declaration
class ChatServer;

class ChatSession
{
private:
    std::unique_ptr<SocketGuard> socket;
    TokenManager& token_manager;
    ChatServer* chat_server;  // ChatServer referansı (mesaj yayınlama için)
    bool is_running;
    
    // Oturum bilgisi - UserInfo yapısı ile tutulur
    UserInfo session_info;

    // Private metodlar
    bool sendMsg(const std::string_view& msg);
    void sendPermissionDenied(const std::string& reason = "");
    bool handleHandShake();
    void handleChatLoop();

public:
    ChatSession(int socket_fd, TokenManager& tm, ChatServer* server = nullptr) 
        : socket(std::make_unique<SocketGuard>(socket_fd)), 
          token_manager(tm), 
          chat_server(server),
          is_running(true),
          session_info{}  // Default başlatıcı - boş UserInfo
    {}

    void run();
    
    // Session bilgilerine erişim
    const UserInfo& getSessionInfo() const { return session_info; }
    const std::string& getUsername() const { return session_info.username; }
    
    // Mesaj gönderme (ChatServer'dan çağrılır)
    bool sendMessage(const std::string& message);
};
