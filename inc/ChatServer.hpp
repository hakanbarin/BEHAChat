#pragma once

#include <iostream>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>
#include <string>
#include "ChatSession.hpp"
#include "TokenManager.hpp"

// CHAT SUNUCUSU SINIFI
// TCP soketler üzerinden sohbet uygulaması sağlar
// Her bağlantı için ayrı bir ChatSession (thread) oluşturur
// Gerçek zamanlı mesaj yayınlama desteği eklenmiştir
class ChatServer
{
private:
    // Sunucu soketini yönetir (RAII)
    std::unique_ptr<SocketGuard> server_socket;
    
    // Sunucunun dinleme yaptığı port
    int port_CH;
    
    // YETKİ SİSTEMİ: Token yöneticisi (yetki denetimi için)
    TokenManager &token_manager;
    
    // Sunucunun çalışma durumunu kontrol eder
    bool is_running;

    // Aktif session'ları takip et (token -> ChatSession*)
    std::unordered_map<std::string, ChatSession*> active_sessions;
    std::mutex sessions_mutex;

    // Private metodlar
    void acceptLoop();

public:
    // CONSTRUCTOR
    ChatServer(int port, TokenManager &tm) 
        : port_CH(port), 
          token_manager(tm), 
          is_running(false) 
    {}

    // SUNUCU BAŞLATMA METODU
    void start();

    // Session yönetimi
    void registerSession(const std::string& token, ChatSession* session);
    void unregisterSession(const std::string& token);

    // Mesaj yayınlama (tüm aktif kullanıcılara)
    int broadcastMessage(const std::string& message, bool is_system = false);
    
    // Özel mesaj gönderme (belirli bir kullanıcıya)
    bool sendPrivateMessage(const std::string& target_username, const std::string& message);
    
    // Kullanıcıyı zorla çıkış yaptır (Kick)
    bool kickUser(const std::string& username, const std::string& reason);
};