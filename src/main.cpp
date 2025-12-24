// ═══════════════════════════════════════════════════════════════════════════
//                         SECURE CHAT SERVER - MAIN
// gRPC Authentication + Admin Service ve TCP Chat Server
// ═══════════════════════════════════════════════════════════════════════════

#include <iostream>
#include <thread>
#include <grpcpp/grpcpp.h>

// Proje header'ları
#include "TokenManager.hpp"
#include "DataBaseManager.hpp"
#include "AuthService.hpp"
#include "AdminService.hpp"
#include "ChatService.hpp"
#include "ChatServer.hpp"

// ─────────────────────────────────────────────────────────────────────────
// PORT AYARLARI
// ─────────────────────────────────────────────────────────────────────────
constexpr int GRPC_PORT = 50051;      // gRPC sunucu portu (Auth + Admin)
constexpr int TCP_PORT = 5000;         // TCP Chat sunucu portu

// ─────────────────────────────────────────────────────────────────────────
// gRPC SUNUCU BAŞLATMA FONKSİYONU
// AuthService, AdminService ve ChatService'i aynı sunucuda çalıştırır
// ─────────────────────────────────────────────────────────────────────────
void runGrpcServer(TokenManager& token_manager, DataBaseManager& db_manager, 
                   AdminServiceImpl& admin_service, ChatServer& chat_server, ChatServiceImpl& chat_service)
{
    // Sunucu adresi
    std::string server_address = "0.0.0.0:" + std::to_string(GRPC_PORT);
    
    // Auth Service instance (TokenManager ve DataBaseManager referansları ile)
    AuthServiceImp auth_service(token_manager, db_manager);
    
    // Chat Service instance (main'de oluşturuldu, referans olarak geçirilecek)
    // Not: ChatService instance'ı main'de oluşturuldu, burada sadece referans alıyoruz
    
    // gRPC Server Builder
    grpc::ServerBuilder builder;
    
    // Dinlenecek adresi ekle (güvenlik yok - geliştirme ortamı)
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    
    // Servisleri kaydet
    builder.RegisterService(&auth_service);      // Login/Register servisi
    builder.RegisterService(&admin_service);     // Admin servisi
    builder.RegisterService(&chat_service);      // Chat servisi (gerçek zamanlı mesajlaşma - referans)
    
    // Sunucuyu oluştur ve başlat
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    
    std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    std::cout << "[gRPC Server] Baslatildi - Port: " << GRPC_PORT << std::endl;
    std::cout << "  ├─ AuthService  : Login, Register, UserStatus" << std::endl;
    std::cout << "  ├─ AdminService : Yetki, ban, broadcast islemleri" << std::endl;
    std::cout << "  └─ ChatService  : Gerçek zamanlı mesajlaşma (bidirectional streaming)" << std::endl;
    std::cout << "═══════════════════════════════════════════════════════════════" << std::endl;
    
    // Sunucu kapanana kadar bekle
    server->Wait();
}

// ─────────────────────────────────────────────────────────────────────────
// TCP CHAT SUNUCU BAŞLATMA FONKSİYONU
// ─────────────────────────────────────────────────────────────────────────
void runTcpServer(ChatServer& chat_server)
{
    try
    {
        chat_server.start();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[TCP Server] Hata: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────
// MAIN FONKSİYONU
// ─────────────────────────────────────────────────────────────────────────
int main()
{
    std::cout << std::endl;
    std::cout << "╔═══════════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║                                                               ║" << std::endl;
    std::cout << "║              🔐 SECURE CHAT SERVER v2.0 🔐                    ║" << std::endl;
    std::cout << "║                                                               ║" << std::endl;
    std::cout << "║  Ozellikler:                                                  ║" << std::endl;
    std::cout << "║    ✓ gRPC Authentication (Login)                              ║" << std::endl;
    std::cout << "║    ✓ User Registration (Database)                             ║" << std::endl;
    std::cout << "║    ✓ Real-time Online Status Stream                           ║" << std::endl;
    std::cout << "║    ✓ Admin Service (Yetki, Ban, Broadcast)                    ║" << std::endl;
    std::cout << "║    ✓ Real-time Chat (gRPC Bidirectional Streaming)           ║" << std::endl;
    std::cout << "║    ✓ TCP Socket Chat                                          ║" << std::endl;
    std::cout << "║    ✓ Permission-Based Access Control                          ║" << std::endl;
    std::cout << "║                                                               ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << std::endl;

    // Database Manager instance
    DataBaseManager db_manager;
    if (!db_manager.isConnected())
    {
        std::cout << "[WARNING] Database baglantisi kurulamadi - Sadece hardcoded kullanicilar aktif" << std::endl;
    }

    // Paylaşılan Token Manager instance
    TokenManager token_manager;
    
    // Admin Service instance (callback'ler için erişim gerekli)
    AdminServiceImpl admin_service(token_manager, db_manager);
    
    // ChatServer instance (callback'ler için)
    ChatServer chat_server(TCP_PORT, token_manager);
    
    // ChatService instance (callback'ler için)
    ChatServiceImpl chat_service(token_manager, db_manager);
    
    // AdminService callback'lerini ChatServer'a bağla
    admin_service.setBroadcastCallback([&chat_server](const std::string& msg, bool is_system) {
        return chat_server.broadcastMessage(msg, is_system);
    });
    
    admin_service.setPrivateMessageCallback([&chat_server](const std::string& username, const std::string& msg) {
        return chat_server.sendPrivateMessage(username, msg);
    });
    
    admin_service.setKickCallback([&chat_server](const std::string& username, const std::string& reason) {
        return chat_server.kickUser(username, reason);
    });
    
    // AdminService yetki değişikliği callback'ini ChatService ve ChatServer'a bağla
    admin_service.setPermissionChangeCallback([&chat_service, &chat_server](const std::string& username, Permission new_perm) {
        // gRPC stream üzerinden bildirim gönder
        chat_service.notifyPermissionChange(username, new_perm);
        
        // TCP üzerinden sadece ilgili kullanıcıya bildirim gönder (broadcast değil, private message)
        std::string perm_msg = "[SISTEM] Yetkiniz guncellendi: " + 
                              std::to_string(static_cast<int>(new_perm)) + 
                              " | PERM_UPDATE:" + std::to_string(static_cast<int>(new_perm)) + "\n";
        chat_server.sendPrivateMessage(username, perm_msg);
    });
    
    // gRPC sunucusunu ayrı thread'de başlat
    std::thread grpc_thread([&token_manager, &db_manager, &admin_service, &chat_server, &chat_service]() {
        runGrpcServer(token_manager, db_manager, admin_service, chat_server, chat_service);
    });
    
    // Ana thread'de TCP sunucusunu başlat
    runTcpServer(chat_server);
    
    // gRPC thread'inin bitmesini bekle (normalde sonsuz döngü)
    grpc_thread.join();
    
    return 0;
}