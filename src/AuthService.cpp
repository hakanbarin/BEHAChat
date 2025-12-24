#include "AuthService.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

// ═══════════════════════════════════════════════════════════════════════════
//                         YARDIMCI FONKSİYONLAR
// ═══════════════════════════════════════════════════════════════════════════
static std::string getCurrentTimestamp()
{
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// ═══════════════════════════════════════════════════════════════════════════
//                         STREAM BİLDİRİMİ
// ═══════════════════════════════════════════════════════════════════════════
void AuthServiceImp::notifyAllStreams(const std::string& username, bool is_online)
{
    std::lock_guard<std::mutex> lock(stream_mutex);
    
    UserStatusUpdate update;
    update.set_username(username);
    update.set_is_online(is_online);
    update.set_total_online(token_manager.getOnlineUserCount());
    update.set_timestamp(getCurrentTimestamp());
    
    // Tüm aktif stream'lere gönder
    auto it = active_streams.begin();
    while (it != active_streams.end())
    {
        if (*it && (*it)->Write(update))
        {
            ++it;
        }
        else
        {
            // Yazma başarısız - stream kapanmış olabilir
            it = active_streams.erase(it);
        }
    }
    
    std::cout << "[AuthService] Status guncelleme yayinlandi - " 
              << username << " " << (is_online ? "ONLINE" : "OFFLINE") 
              << " | Aktif stream: " << active_streams.size() << std::endl;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         LOGIN METODU
// Kullanıcı adı ve şifre ile giriş yapılır
// Başarılı olursa UserInfo oluşturulur ve token döndürülür
// ═══════════════════════════════════════════════════════════════════════════
Status AuthServiceImp::Login(ServerContext* context, const LoginRequest* request, LoginResponse* response)
{
    std::string user = request->username();
    std::string password = request->password();

    std::cout << "[gRPC Auth] Login istegi - Kullanici: " << user << std::endl;

    // ÖNCE hardcoded kullanıcıları kontrol et (hızlı test için)
    // ADMIN KULLANICISI
    if (user == "admin" && password == "admin123")
    {
        UserInfo tokenInfo = token_manager.createSession(user, Permission::ADMIN);
        
        response->set_success(true);
        response->set_token(tokenInfo.token);
        response->set_permission(PermissionLevel::ADMIN);
        
        std::cout << "[gRPC Auth] ADMIN giris basarili - Token: " << tokenInfo.token << std::endl;
        return Status::OK;
    }
    
    // MODERATOR KULLANICISI
    if (user == "moderator" && password == "mod456")
    {
        UserInfo tokenInfo = token_manager.createSession(user, Permission::MODERATOR);
        
        response->set_success(true);
        response->set_token(tokenInfo.token);
        response->set_permission(PermissionLevel::MODERATOR);
        
        std::cout << "[gRPC Auth] MODERATOR giris basarili" << std::endl;
        return Status::OK;
    }
    
    // NORMAL KULLANICI
    if (user == "user" && password == "user789")
    {
        UserInfo tokenInfo = token_manager.createSession(user, Permission::USER);
        
        response->set_success(true);
        response->set_token(tokenInfo.token);
        response->set_permission(PermissionLevel::USER);
        
        std::cout << "[gRPC Auth] USER giris basarili" << std::endl;
        return Status::OK;
    }
    
    // KONUK KULLANICISI
    if (user == "guest" && password == "guest999")
    {
        UserInfo tokenInfo = token_manager.createSession(user, Permission::GUEST);
        
        response->set_success(true);
        response->set_token(tokenInfo.token);
        response->set_permission(PermissionLevel::GUEST);
        
        std::cout << "[gRPC Auth] GUEST giris basarili" << std::endl;
        return Status::OK;
    }

    // Database'den kullanıcıyı kontrol et
    if (db_manager.validateUser(user, password))
    {
        Permission perm = db_manager.getUserPermission(user);
        
        // BANNED kontrolü
        if (perm == Permission::BANNED)
        {
            response->set_success(false);
            response->set_error_message("Bu hesap banlanmis");
            response->set_permission(PermissionLevel::BANNED);
            return Status::OK;
        }
        
        UserInfo tokenInfo = token_manager.createSession(user, perm);
        
        response->set_success(true);
        response->set_token(tokenInfo.token);
        
        PermissionLevel protoPerm;
        switch(perm)
        {
            case Permission::ADMIN:     protoPerm = PermissionLevel::ADMIN; break;
            case Permission::MODERATOR: protoPerm = PermissionLevel::MODERATOR; break;
            case Permission::USER:      protoPerm = PermissionLevel::USER; break;
            case Permission::GUEST:     protoPerm = PermissionLevel::GUEST; break;
            default:                    protoPerm = PermissionLevel::USER; break;
        }
        response->set_permission(protoPerm);
        
        std::cout << "[gRPC Auth] DB giris basarili - Kullanici: " << user 
                  << ", Yetki: " << static_cast<int>(perm) << " -> " << static_cast<int>(protoPerm) << std::endl;
        return Status::OK;
    }
    
    // GİRİŞ BAŞARISIZ
    response->set_success(false);
    response->set_error_message("Hatali kullanici adi veya sifre");
    response->set_permission(PermissionLevel::BANNED);
    
    std::cout << "[gRPC Auth] Basarisiz giris - Kullanici: " << user << std::endl;
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         REGISTER METODU
// Yeni kullanıcı kaydı yapar ve database'e ekler
// ═══════════════════════════════════════════════════════════════════════════
Status AuthServiceImp::Register(ServerContext* context, const RegisterRequest* request, RegisterResponse* response)
{
    std::string username = request->username();
    std::string password = request->password();
    std::string email = request->email();

    std::cout << "\n[gRPC Auth] ==========================================" << std::endl;
    std::cout << "[gRPC Auth] REGISTER istegi alindi" << std::endl;
    std::cout << "[gRPC Auth] Username: " << username << std::endl;
    std::cout << "[gRPC Auth] Email: " << email << std::endl;
    std::cout << "[gRPC Auth] Password length: " << password.length() << std::endl;

    // Kullanıcı adı kontrolü
    if (username.empty() || username.length() < 3)
    {
        response->set_success(false);
        response->set_message("Kullanici adi en az 3 karakter olmali");
        return Status::OK;
    }

    // Şifre kontrolü
    if (password.empty() || password.length() < 6)
    {
        response->set_success(false);
        response->set_message("Sifre en az 6 karakter olmali");
        return Status::OK;
    }

    // Kullanıcı zaten var mı?
    if (db_manager.userExists(username))
    {
        response->set_success(false);
        response->set_message("Bu kullanici adi zaten kullaniliyor");
        return Status::OK;
    }

    // Database'e ekle
    std::string user_id = db_manager.createUser(username, password, email, Permission::USER);

    if (!user_id.empty())
    {
        response->set_success(true);
        response->set_message("Kayit basarili! Giris yapabilirsiniz.");
        response->set_user_id(user_id);
        
        std::cout << "[gRPC Auth] Kayit basarili - Kullanici: " << username 
                  << ", ID: " << user_id << std::endl;
    }
    else
    {
        response->set_success(false);
        response->set_message("Kayit sirasinda bir hata olustu - Database baglantisini kontrol edin");
        
        std::cerr << "[gRPC Auth] KAYIT HATASI - createUser bos string dondurdu!" << std::endl;
        std::cerr << "[gRPC Auth] Database baglantisi veya tablo sorunu olabilir." << std::endl;
    }

    std::cout << "[gRPC Auth] ==========================================\n" << std::endl;
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         STREAM USER STATUS
// Client bağlandığında tüm kullanıcı durumlarını ve değişiklikleri gönderir
// ═══════════════════════════════════════════════════════════════════════════
Status AuthServiceImp::StreamUserStatus(ServerContext* context, const UserStatusRequest* request, 
                                        ServerWriter<UserStatusUpdate>* writer)
{
    std::cout << "[AuthService] Yeni status stream baglantisi" << std::endl;

    // Stream'i listeye ekle
    {
        std::lock_guard<std::mutex> lock(stream_mutex);
        active_streams.push_back(writer);
    }

    // İlk olarak mevcut tüm online kullanıcıları gönder
    auto users = token_manager.getAllActiveUsers();
    for (const auto& user : users)
    {
        UserStatusUpdate update;
        update.set_username(user.username);
        update.set_is_online(user.is_online);
        update.set_total_online(token_manager.getOnlineUserCount());
        update.set_timestamp(getCurrentTimestamp());
        
        if (!writer->Write(update))
        {
            // Yazma başarısız - stream kapandı
            break;
        }
    }

    // Stream açık kaldığı sürece bekle
    // Client bağlantıyı kesene kadar bloklanır
    while (!context->IsCancelled())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stream'i listeden çıkar
    {
        std::lock_guard<std::mutex> lock(stream_mutex);
        active_streams.erase(
            std::remove(active_streams.begin(), active_streams.end(), writer),
            active_streams.end()
        );
    }

    std::cout << "[AuthService] Status stream baglantisi kapandi" << std::endl;
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         GET ONLINE COUNT
// Anlık online kullanıcı sayısını döndürür
// ═══════════════════════════════════════════════════════════════════════════
Status AuthServiceImp::GetOnlineCount(ServerContext* context, const OnlineCountRequest* request, 
                                      OnlineCountResponse* response)
{
    response->set_online_count(token_manager.getOnlineUserCount());
    response->set_total_registered(db_manager.getTotalUserCount());
    
    std::cout << "[AuthService] Online sayisi istendi - Online: " << response->online_count() 
              << ", Toplam: " << response->total_registered() << std::endl;
    
    return Status::OK;
}

// ═══════════════════════════════════════════════════════════════════════════
//                         GET ALL USERS STATUS
// Tüm kullanıcıların online/offline durumunu döndürür
// HERKESİN görebileceği bir liste - Admin yetkisi gerekmez
// ═══════════════════════════════════════════════════════════════════════════
Status AuthServiceImp::GetAllUsersStatus(ServerContext* context, const AllUsersStatusRequest* request,
                                         AllUsersStatusResponse* response)
{
    std::cout << "[AuthService] Tum kullanici durumlari istendi" << std::endl;
    
    try
    {
        // TokenManager'dan gerçek zamanlı online kullanıcıları al
        auto activeTokens = token_manager.getAllActiveUsers();
        std::set<std::string> onlineUsernames;
        for (const auto& user : activeTokens)
        {
            onlineUsernames.insert(user.username);
        }
        
        // Veritabanından TÜM kullanıcıları al
        auto dbUsers = db_manager.getAllUsers();
        
        int onlineCount = 0;
        int offlineCount = 0;
        
        // Her kullanıcı için online/offline durumunu belirle
        for (const auto& dbUser : dbUsers)
        {
            // Online/Offline durumunu TokenManager'dan kontrol et (gerçek zamanlı)
            bool isOnline = onlineUsernames.count(dbUser.username) > 0;
            
            // Permission dönüşümü
            PermissionLevel permLevel;
            switch(dbUser.permission)
            {
                case Permission::ADMIN:     permLevel = PermissionLevel::ADMIN; break;
                case Permission::MODERATOR: permLevel = PermissionLevel::MODERATOR; break;
                case Permission::USER:      permLevel = PermissionLevel::USER; break;
                case Permission::GUEST:     permLevel = PermissionLevel::GUEST; break;
                case Permission::BANNED:    permLevel = PermissionLevel::BANNED; break;
                default:                    permLevel = PermissionLevel::USER; break;
            }
            
            // Online veya offline listesine ekle
            UserStatusInfo* info;
            if (isOnline)
            {
                info = response->add_online_users();
                onlineCount++;
            }
            else
            {
                info = response->add_offline_users();
                offlineCount++;
            }
            
            info->set_username(dbUser.username);
            info->set_is_online(isOnline);
            info->set_permission(permLevel);
            info->set_last_seen(dbUser.last_seen);
            info->set_created_at(dbUser.created_at);
            info->set_email(dbUser.email);
        }
        
        // Sayaçları ayarla
        response->set_success(true);
        response->set_message("Basarili");
        response->set_online_count(onlineCount);
        response->set_offline_count(offlineCount);
        response->set_total_count(onlineCount + offlineCount);
        
        std::cout << "[AuthService] Kullanici durumlari gonderildi - Online: " 
                  << onlineCount << ", Offline: " << offlineCount << std::endl;
    }
    catch (const std::exception& e)
    {
        response->set_success(false);
        response->set_message(std::string("Hata: ") + e.what());
        std::cerr << "[AuthService] GetAllUsersStatus hatasi: " << e.what() << std::endl;
    }
    
    return Status::OK;
}
