#include "ChatSession.hpp"
#include "ChatServer.hpp"
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <cstring>

// ═══════════════════════════════════════════════════════════════════════════
//                         MESAJ GÖNDERME
// ═══════════════════════════════════════════════════════════════════════════
bool ChatSession::sendMsg(const std::string_view& msg)
{
    ssize_t bytes_sent = ::send(socket->get(), msg.data(), msg.length(), MSG_NOSIGNAL);
    return bytes_sent != -1;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         YETKİ REDDİ MESAJI
// ═══════════════════════════════════════════════════════════════════════════
void ChatSession::sendPermissionDenied(const std::string& reason)
{
    std::string error_msg = "[ERR] Yetkisiz Erisim";
    if (!reason.empty())
    {
        error_msg += " - " + reason;
    }
    error_msg += "\n";
    sendMsg(error_msg);
}

// ═══════════════════════════════════════════════════════════════════════════
//                         OTURUM ÇALIŞTIRMA
// ═══════════════════════════════════════════════════════════════════════════
void ChatSession::run()
{
    if (!handleHandShake())
    {
        return;
    }
    handleChatLoop();
}

// ═══════════════════════════════════════════════════════════════════════════
//                         EL SIKIŞ METODU
// ═══════════════════════════════════════════════════════════════════════════
bool ChatSession::handleHandShake()
{
    std::array<char, 1024> buffer{};

    // Socket'in hazır olmasını bekle (select ile timeout kontrolü)
    fd_set readfds;
    struct timeval timeout;
    FD_ZERO(&readfds);
    FD_SET(socket->get(), &readfds);
    timeout.tv_sec = 15;  // 15 saniye timeout (client'ın bağlanması ve token göndermesi için yeterli süre)
    timeout.tv_usec = 0;
    
    int select_result = select(socket->get() + 1, &readfds, nullptr, nullptr, &timeout);
    
    if (select_result <= 0)
    {
        if (select_result == 0)
        {
            std::cout << "[ChatSession] Handshake: Timeout - Token beklenirken zaman asimi (15 saniye)" << std::endl;
            sendMsg("ERR Handshake timeout\n");
        }
        else
        {
            std::cout << "[ChatSession] Handshake: Select hatasi - " << strerror(errno) << std::endl;
            sendMsg("ERR Handshake hatasi\n");
        }
        return false;
    }
    
    // Socket okunabilir, token'ı al
    ssize_t bytes_read = ::recv(socket->get(), buffer.data(), buffer.size() - 1, 0);
    
    if (bytes_read <= 0)
    {
        if (bytes_read == 0)
        {
            std::cout << "[ChatSession] Handshake: Baglanti kapatildi (graceful close)" << std::endl;
        }
        else
        {
            std::cout << "[ChatSession] Handshake: Recv hatasi - " << strerror(errno) << std::endl;
        }
        return false;
    }

    std::string raw_token(buffer.data(), bytes_read);

    // Trim
    while (!raw_token.empty() && std::isspace(raw_token.back()))
    {
        raw_token.pop_back();
    }
    
    if (raw_token.empty())
    {
        sendMsg("ERR Bos token gonderildi\n");
        return false;
    }

    // Token string'den UserInfo al
    auto token_info = token_manager.getTokenInfo(raw_token);
    
    // Token geçerli mi kontrol et
    if (!token_info || token_info->isEmpty())
    {
        sendMsg("ERR Gecersiz token\n");
        std::cout << "[ChatSession] Handshake: Gecersiz token" << std::endl;
        return false;
    }

    session_info = *token_info;

    // BANNED kullanıcı kontrolü
    if (session_info.permission == Permission::BANNED)
    {
        sendMsg("ERR Yasakli kullanici\n");
        std::cout << "[ChatSession] Handshake: BANNED kullanici giris denemesi" << std::endl;
        return false;
    }

    // ChatServer'a kayıt ol
    if (chat_server)
    {
        chat_server->registerSession(session_info.token, this);
    }

    // Yetki seviyesi string'e çevir
    std::string permission_name;
    switch(session_info.permission)
    {
        case Permission::ADMIN:     permission_name = "ADMIN"; break;
        case Permission::MODERATOR: permission_name = "MODERATOR"; break;
        case Permission::USER:      permission_name = "USER"; break;
        case Permission::GUEST:     permission_name = "GUEST"; break;
        default:                    permission_name = "UNKNOWN";
    }

    std::string success_msg = "[OK] Giris basarili - Yetki: " + permission_name + "\n";
    sendMsg(success_msg);
    
    std::cout << "[ChatSession] Handshake basarili - Kullanici: " << session_info.username 
              << ", Yetki: " << permission_name << std::endl;
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         CHAT DÖNGÜSÜ
// ═══════════════════════════════════════════════════════════════════════════
void ChatSession::handleChatLoop()
{
    std::array<char, 4096> buffer{};

    while (is_running)
    {
        buffer.fill(0);
        ssize_t bytes_read = ::recv(socket->get(), buffer.data(), buffer.size() - 1, 0);
        
        if (bytes_read <= 0)
        {
            std::cout << "[ChatSession] Baglanti kapandi - Kullanici: " << session_info.username << std::endl;
            is_running = false;
            
            // ChatServer'dan kaydı kaldır
            if (chat_server)
            {
                chat_server->unregisterSession(session_info.token);
            }
            
            // Oturum sonlandırıldığında token string ile sil
            token_manager.removeSession(session_info.token);
            break;
        }

        std::string_view msg_view(buffer.data(), bytes_read);

        // GUEST kullanıcılar mesaj gönderemez
        if (session_info.permission == Permission::GUEST)
        {
            sendPermissionDenied("GUEST kullanicilar mesaj gonderemez");
            std::cout << "[ChatSession] GUEST kullanici yazma denemesi" << std::endl;
            continue;
        }

        // Mesajı tüm kullanıcılara yayınla
        std::string formatted_msg;
        
        // Yetki seviyesi etiketi ekle
        switch(session_info.permission)
        {
            case Permission::ADMIN:     formatted_msg = "[ADMIN] "; break;
            case Permission::MODERATOR: formatted_msg = "[MODERATOR] "; break;
            case Permission::USER:      formatted_msg = "[USER] "; break;
            default: break;
        }

        formatted_msg += "[" + session_info.username + "] " + std::string(msg_view);

        // ChatServer üzerinden tüm kullanıcılara yayınla
        if (chat_server)
        {
            chat_server->broadcastMessage(formatted_msg, false);
        }
        else
        {
            // Fallback: Sadece gönderene echo
            sendMsg(formatted_msg);
        sendMsg("\n");
        }
        
        std::cout << "[ChatSession] [" << session_info.username << "] Mesaj yayinlandi: " 
                  << std::string(msg_view).substr(0, 50) << std::endl;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//                         MESAJ GÖNDERME (PUBLIC)
// ═══════════════════════════════════════════════════════════════════════════
bool ChatSession::sendMessage(const std::string& message)
{
    return sendMsg(message);
}
