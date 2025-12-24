#include "ChatServer.hpp"

// ═══════════════════════════════════════════════════════════════════════════
//                         SUNUCU BAŞLATMA METODU
// ═══════════════════════════════════════════════════════════════════════════
void ChatServer::start()
{
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);

    if (fd == -1)
        throw std::system_error(errno, std::generic_category(), "Socket olusturulamadi");

    server_socket = std::make_unique<SocketGuard>(fd);

    int opt = 1;

    sockaddr_in address {
                       .sin_family = AF_INET,
                       .sin_port   = htons(port_CH),
                       .sin_addr  = { INADDR_ANY }
    };

    if (::bind(server_socket->get(), reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "Bind hatasi");
    }

    if (::listen(server_socket->get(), 10) < 0)
    {
        throw std::system_error(errno, std::generic_category(), "lİSTEN hatasi");
    }

    is_running = true;

    std::cout << "[TCP SERVER] basladi. Port: " << port_CH << std::endl;

    acceptLoop();
}

// ═══════════════════════════════════════════════════════════════════════════
//                         SESSION YÖNETİMİ
// ═══════════════════════════════════════════════════════════════════════════
void ChatServer::registerSession(const std::string& token, ChatSession* session)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);
    active_sessions[token] = session;
    std::cout << "[ChatServer] Session kaydedildi - Token: " << token.substr(0, 8) << "..." << std::endl;
}

void ChatServer::unregisterSession(const std::string& token)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);
    active_sessions.erase(token);
    std::cout << "[ChatServer] Session kaldirildi - Token: " << token.substr(0, 8) << "..." << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         MESAJ YAYINLAMA
// ═══════════════════════════════════════════════════════════════════════════
int ChatServer::broadcastMessage(const std::string& message, bool is_system)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);
    int count = 0;
    
    std::string formatted_msg = message;
    if (!formatted_msg.empty() && formatted_msg.back() != '\n')
    {
        formatted_msg += "\n";
    }
    
    for (auto& [token, session] : active_sessions)
    {
        // GUEST kullanıcılar mesaj gönderemez ama alabilir
        // BANNED kullanıcılar zaten bağlanamaz
        
        if (session->sendMessage(formatted_msg))
        {
            count++;
        }
    }
    
    std::cout << "[ChatServer] Broadcast mesaj gonderildi - Alici sayisi: " << count << std::endl;
    return count;
}

bool ChatServer::sendPrivateMessage(const std::string& target_username, const std::string& message)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    // Eğer mesaj zaten [OZEL MESAJ] veya [SISTEM] ile başlıyorsa, tekrar ekleme
    std::string formatted_msg = message;
    if (message.substr(0, 12) != "[OZEL MESAJ]" && message.substr(0, 8) != "[SISTEM]")
    {
        formatted_msg = "[OZEL MESAJ] " + message;
    }
    
    if (!formatted_msg.empty() && formatted_msg.back() != '\n')
    {
        formatted_msg += "\n";
    }
    
    for (auto& [token, session] : active_sessions)
    {
        if (session->getUsername() == target_username)
        {
            if (session->sendMessage(formatted_msg))
            {
                std::cout << "[ChatServer] Ozel mesaj gonderildi - Hedef: " << target_username << std::endl;
                return true;
            }
        }
    }
    
    std::cout << "[ChatServer] Ozel mesaj gonderilemedi - Kullanici bulunamadi: " << target_username << std::endl;
    return false;
}

bool ChatServer::kickUser(const std::string& username, const std::string& reason)
{
    std::lock_guard<std::mutex> lock(sessions_mutex);
    
    for (auto it = active_sessions.begin(); it != active_sessions.end(); ++it)
    {
        if (it->second->getUsername() == username)
        {
            // Kullanıcıyı çıkış yaptır (session'ı sonlandır)
            // ChatSession'ın is_running flag'ini false yapmak gerekir
            std::cout << "[ChatServer] Kullanici atildi - Kullanici: " << username << ", Sebep: " << reason << std::endl;
            active_sessions.erase(it);
            return true;
        }
    }
    
    return false;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         BAĞLANTI KABUL DÖNGÜSÜ
// ═══════════════════════════════════════════════════════════════════════════
void ChatServer::acceptLoop()
{
    while(is_running)
    {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        int client_fd = ::accept(server_socket->get(), reinterpret_cast<struct sockaddr*>(&client_addr), &addr_len);

        // sunucuyu ayakta tutma amacıyla yapılıyor
        if (client_fd < 0) {perror("Accept failed"); continue;} 

        // YETKİ SİSTEMİ: Her bağlantı için yetki denetimi yapılacak
        std::thread([client_fd, this]() {
            ChatSession session(client_fd, this->token_manager, this);
            session.run();
        }).detach();
    }
}
