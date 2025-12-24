-- ====================================================================
-- OTURUM AKTİVİTE KAYITLARI (SESSION ACTIVITY LOGS)
-- ====================================================================
-- Bu dosya kullanıcı aktivitelerini kaydeden log (günlük) tablosunu oluşturur
--
-- LOG NEDİR?
-- Log, sistemde olan her şeyi kayıt altına alma işlemidir
-- Türkçe'de "kayıt tutma", "günlük tutma" anlamına gelir
-- Tıpkı bir geminin seyir defteri gibi - her olay yazılır
--
-- NEDEN LOG TUTUYORUZ?
-- 1. GÜVENLİK: Kim ne zaman ne yaptı? Şüpheli aktivite var mı?
-- 2. HATA AYIKLAMA: Bir sorun olduğunda ne olduğunu görebilmek
-- 3. İSTATİSTİK: Kullanıcı davranışlarını analiz etmek
-- 4. UYUMLULUK: Yasal gereklilikler (KVKK, GDPR vb.)
-- 5. DENETİM (AUDIT): Yetkili kullanıcılar ne yapıyor?
--
-- GERÇEK HAYAT ÖRNEK:
-- * Banka: Her para transferi kaydedilir
-- * E-ticaret: Her sipariş, her ödeme kaydedilir
-- * Sosyal medya: Her gönderi, her yorum kaydedilir
-- * Chat sistemi: Her giriş, çıkış, mesaj kaydedilir
--
-- BU SİSTEMDE NELER KAYDETMELIYIZ?
-- * Kullanıcı giriş yaptı (login)
-- * Kullanıcı çıkış yaptı (logout)
-- * Şifre değiştirildi
-- * Yetki değiştirildi
-- * Ban verildi/kaldırıldı
-- * Mesaj gönderildi
-- * Hatalı giriş denemesi
-- * Şüpheli aktivite tespit edildi

-- ====================================================================
-- 1. SESSION_LOGS TABLOSUNU OLUŞTUR
-- ====================================================================
CREATE TABLE IF NOT EXISTS session_logs (
    -- IF NOT EXISTS: Tablo varsa hata verme, yoksa oluştur
    
    -- ----------------------------------------------------------------
    -- SÜTUN 1: Log ID (Benzersiz Log Kaydı Kimliği)
    -- ----------------------------------------------------------------
    id SERIAL PRIMARY KEY,
    -- SERIAL: Otomatik artan sayı (1, 2, 3, 4, ...)
    --         Her yeni log kaydı için PostgreSQL otomatik numara verir
    -- PRIMARY KEY: Bu tablonun ana anahtarı
    --              * Benzersiz (unique) - her log'un kendine özel ID'si
    --              * Boş olamaz (not null)
    --              * Hızlı arama için otomatik indeks oluşur
    --
    -- NEDEN GEREK: Milyonlarca log kaydı olabilir
    --              Her kaydı benzersiz şekilde tanımlayabilmeliyiz
    --              Belirli bir log'u bulmak için ID kullanırız
    
    -- ----------------------------------------------------------------
    -- SÜTUN 2: Kullanıcı ID'si (Kim bu işlemi yaptı?)
    -- ----------------------------------------------------------------
    user_id INT REFERENCES users(id) ON DELETE SET NULL,
    -- INT: Tam sayı - users tablosundaki id ile eşleşir
    -- REFERENCES users(id): users tablosuna foreign key ilişkisi
    --                       İLİŞKİ: Her log bir kullanıcıya aittir
    --                       (Ama kullanıcı olmayan loglar da olabilir)
    --
    -- NOT NULL YOK: Bu alan NULL olabilir
    -- NEDEN NULL OLABİLİR:
    -- * Sistem loglari: Kullanıcı yok, sistem otomatik yaptı
    -- * Anonim aktiviteler: Henüz giriş yapmamış ziyaretçi
    -- * Başarısız giriş denemeleri: Kullanıcı bulunamadı
    --
    -- FOREIGN KEY İLİŞKİSİ:
    --   users tablosu        session_logs tablosu
    --   +-----------+        +----------+
    --   | id | name |        | user_id  | --> users.id'ye işaret eder
    --   +-----------+        +----------+
    --   | 1  | ahmet|   <--- | 1        | (ahmet'in logu)
    --   | 2  | mehmet|  <--- | 2        | (mehmet'in logu)
    --   +-----------+        | NULL     | (sistem logu)
    --                        | 999      | (silinmiş kullanıcı)
    --                        +----------+
    --
    -- ON DELETE SET NULL: Kullanıcı silinirse, user_id = NULL olur
    --                     Log kaydı kalır ama sahibi "bilinmiyor" olur
    --                     MANTIK: Geçmiş kayıtlar önemli, kullanıcı silinse bile
    --                            loglar silinmemeli (audit trail)
    --                     Alternatif: CASCADE kullanırsak kullanıcı silinince
    --                                logları da silinir (istemiyoruz)
    
    -- ----------------------------------------------------------------
    -- SÜTUN 3: Aksiyon (Ne Yapıldı?)
    -- ----------------------------------------------------------------
    action VARCHAR(100) NOT NULL,
    -- VARCHAR(100): Maksimum 100 karakterlik metin
    --               Kısa, öz aksiyon açıklaması için yeterli
    -- NOT NULL: Boş olamaz, mutlaka bir aksiyon belirtilmeli
    --           "Ne oldu?" sorusunun cevabı
    --
    -- ÖRNEK AKSIYONLAR:
    -- * "LOGIN" - Kullanıcı giriş yaptı
    -- * "LOGOUT" - Kullanıcı çıkış yaptı
    -- * "PASSWORD_CHANGE" - Şifre değiştirildi
    -- * "PERMISSION_CHANGE" - Yetki değiştirildi
    -- * "USER_BANNED" - Kullanıcı banlandı
    -- * "USER_UNBANNED" - Ban kaldırıldı
    -- * "MESSAGE_SENT" - Mesaj gönderildi
    -- * "MESSAGE_DELETED" - Mesaj silindi
    -- * "LOGIN_FAILED" - Başarısız giriş denemesi
    -- * "SUSPICIOUS_ACTIVITY" - Şüpheli aktivite
    -- * "TOKEN_CREATED" - Token oluşturuldu
    -- * "TOKEN_EXPIRED" - Token süresi doldu
    --
    -- NEDEN STANDART DEĞERLER:
    -- * Filtreleme kolaylaşır: SELECT * FROM session_logs WHERE action = 'LOGIN'
    -- * İstatistik yapılabilir: SELECT action, COUNT(*) FROM session_logs GROUP BY action
    -- * Karışıklık önlenir: "login", "Login", "Giriş yapıldı" gibi farklı yazımlar olmaz
    --
    -- C++ KODUNDA:
    -- const string ACTION_LOGIN = "LOGIN";
    -- const string ACTION_LOGOUT = "LOGOUT";
    -- const string ACTION_PASSWORD_CHANGE = "PASSWORD_CHANGE";
    -- ...
    -- LogActivity(user_id, ACTION_LOGIN, "User logged in successfully");
    
    -- ----------------------------------------------------------------
    -- SÜTUN 4: Detaylar (Ek Bilgiler)
    -- ----------------------------------------------------------------
    details TEXT,
    -- TEXT: Sınırsız uzunlukta metin
    --       Aksiyonla ilgili detaylı açıklama
    -- NOT NULL YOK: Opsiyonel, bazı loglar için detay gerekmeyebilir
    --
    -- ÖRNEK DETAYLAR:
    -- * action="LOGIN", details="Login from IP 192.168.1.100, Chrome browser"
    -- * action="PASSWORD_CHANGE", details="Password changed by user request"
    -- * action="USER_BANNED", details="Banned by admin (user_id=1) for spam. Duration: 24 hours"
    -- * action="LOGIN_FAILED", details="Invalid password for username: ahmet"
    -- * action="MESSAGE_SENT", details="Message sent to chat room #general (message_id=12345)"
    -- * action="PERMISSION_CHANGE", details="Permission changed from USER to MODERATOR by admin"
    --
    -- JSON FORMAT (Opsiyonel):
    -- Detayları JSON olarak saklayabilirsiniz:
    -- details='{"ip": "192.168.1.100", "browser": "Chrome", "os": "Windows 10"}'
    -- PostgreSQL'de JSON sorguları yapılabilir:
    -- SELECT * FROM session_logs WHERE details::json->>'ip' = '192.168.1.100'
    --
    -- NEDEN DETAY ÖNEMLİ:
    -- * Hata ayıklama: Tam olarak ne olduğunu anlamak
    -- * Güvenlik: Şüpheli aktiviteyi analiz etmek
    -- * Kullanıcı desteği: "Neden banlandım?" sorusuna cevap vermek
    
    -- ----------------------------------------------------------------
    -- SÜTUN 5: IP Adresi (Nereden Bağlandı?)
    -- ----------------------------------------------------------------
    ip_address VARCHAR(45),
    -- VARCHAR(45): IP adresi için yeterli uzunluk
    --              IPv4: 15 karakter (örn: "192.168.100.100")
    --              IPv6: 45 karakter (örn: "2001:0db8:85a3:0000:0000:8a2e:0370:7334")
    -- NOT NULL YOK: Opsiyonel, bazı durumlarda IP bilinmeyebilir
    --
    -- NEDEN IP ADRESİ ÖNEMLI:
    -- * Güvenlik: Aynı hesaba farklı ülkelerden giriş var mı?
    -- * Spam önleme: Aynı IP'den çok fazla istek var mı?
    -- * Coğrafi analiz: Kullanıcılar nereden bağlanıyor?
    -- * Yasal gereklilik: Bazı ülkelerde IP loglama zorunlu
    --
    -- ÖRNEK SENARYO:
    -- * Ahmet her zaman Türkiye'den (212.x.x.x) bağlanıyor
    -- * Bir anda ABD'den (104.x.x.x) giriş oldu --> ŞÜPHELİ
    -- * Sistem otomatik olarak ek doğrulama isteyebilir
    --
    -- GİZLİLİK NOTU:
    -- * IP adresi kişisel veri sayılır (KVKK, GDPR)
    -- * Belirli bir süre sonra silinmeli veya anonimleştirilmeli
    -- * Kullanıcıya bilgi verilmeli: "IP adresinizi güvenlik için saklıyoruz"
    
    -- ----------------------------------------------------------------
    -- SÜTUN 6: Oluşturma Zamanı (Ne Zaman Oldu?)
    -- ----------------------------------------------------------------
    created_at TIMESTAMP DEFAULT NOW()
    -- TIMESTAMP: Tarih ve saat bilgisi
    -- DEFAULT NOW(): Log oluşturulduğunda otomatik olarak o anki zaman
    -- NOW() = CURRENT_TIMESTAMP = Şu anki tarih ve saat
    --
    -- ÖRNEK: '2024-01-15 18:30:45.123456'
    -- Bu değer hiç değişmez - olay ne zaman olduğunu gösterir
    --
    -- NEDEN KRİTİK:
    -- * Zaman çizelgesi: Olayları sırayla görebilmek
    -- * "5 dakika içinde 100 giriş denemesi" gibi analizler
    -- * "Kullanıcı son 1 saatte neler yaptı?" sorguları
    -- * Yasal gereklilik: Kayıtlar tarih-saatli olmalı
    --
    -- SAAT DİLİMİ (TIMEZONE):
    -- PostgreSQL TIMESTAMP WITH TIME ZONE kullanabilir
    -- Şu anda sadece TIMESTAMP kullanıyoruz (sunucu saati)
    -- 00_init.sql'de SET timezone = 'Europe/Istanbul' yapıldı
    -- Tüm kayıtlar İstanbul saatine göre
);

-- ====================================================================
-- 2. PERFORMANS İÇİN İNDEKSLER OLUŞTUR
-- ====================================================================

-- ----------------------------------------------------------------
-- INDEX 1: Kullanıcıya Göre Log Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_session_logs_user_id ON session_logs(user_id);
-- idx_session_logs_user_id: İndeksin ismi
-- ON session_logs(user_id): user_id sütununa indeks
--
-- NEDEN LAZIM:
-- * "Bu kullanıcı ne zaman giriş yapmış?" sorgusu
--   SELECT * FROM session_logs WHERE user_id = 5 ORDER BY created_at DESC
-- * Kullanıcının aktivite geçmişi
-- * Admin panelinde kullanıcı detay sayfası
--
-- ÖRNEK KULLANIM:
-- Ahmet'in son 10 aktivitesini göster:
-- SELECT action, details, created_at 
-- FROM session_logs 
-- WHERE user_id = 5 
-- ORDER BY created_at DESC 
-- LIMIT 10;

-- ----------------------------------------------------------------
-- INDEX 2: Aksiyona Göre Log Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_session_logs_action ON session_logs(action);
-- ON session_logs(action): action sütununa indeks
--
-- NEDEN LAZIM:
-- * Belirli tipteki logları filtrelemek için
--   SELECT * FROM session_logs WHERE action = 'LOGIN_FAILED'
-- * İstatistik: "Kaç başarısız giriş denemesi var?"
--   SELECT COUNT(*) FROM session_logs WHERE action = 'LOGIN_FAILED'
-- * Güvenlik analizi: Şüpheli aktiviteleri bulmak
--
-- ÖRNEK SENARYO:
-- Son 1 saatte 10'dan fazla başarısız giriş denemesi olan IP'ler:
-- SELECT ip_address, COUNT(*) as fail_count 
-- FROM session_logs 
-- WHERE action = 'LOGIN_FAILED' 
--   AND created_at > NOW() - INTERVAL '1 hour' 
-- GROUP BY ip_address 
-- HAVING COUNT(*) > 10;

-- ----------------------------------------------------------------
-- INDEX 3: Zamana Göre Log Arama (Composite Index)
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_session_logs_created_at ON session_logs(created_at);
-- ON session_logs(created_at): created_at sütununa indeks
--
-- NEDEN LAZIM:
-- * Zaman aralığında log sorguları çok sık yapılır
--   SELECT * FROM session_logs WHERE created_at > '2024-01-01' AND created_at < '2024-02-01'
-- * Son 24 saat, son 7 gün gibi filtreler
-- * Eski logları temizlemek için
--
-- PERFORMANS:
-- * İndeks yok: Milyonlarca logu tek tek kontrol eder (ÇOK YAVAŞ)
-- * İndeks var: Sadece tarih aralığındakileri bulur (HIZLI)

-- ----------------------------------------------------------------
-- INDEX 4: IP Adresine Göre Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_session_logs_ip ON session_logs(ip_address);
-- ON session_logs(ip_address): ip_address sütununa indeks
--
-- NEDEN LAZIM:
-- * Belirli bir IP'nin tüm aktivitelerini görmek
--   SELECT * FROM session_logs WHERE ip_address = '192.168.1.100'
-- * Spam/DDoS tespiti: Aynı IP'den çok fazla istek var mı?
-- * Güvenlik: Şüpheli IP'leri engellemek
--
-- ÖRNEK:
-- Bu IP'nin son 1 saatte kaç isteği var?
-- SELECT COUNT(*) 
-- FROM session_logs 
-- WHERE ip_address = '192.168.1.100' 
--   AND created_at > NOW() - INTERVAL '1 hour';

-- ====================================================================
-- 3. OTOMATİK ESKİ LOG TEMİZLEME FONKSİYONU
-- ====================================================================
-- Loglar zamanla çok yer kaplar, eski logları silmeliyiz

CREATE OR REPLACE FUNCTION cleanup_old_logs(days_to_keep INT DEFAULT 90)
-- CREATE OR REPLACE FUNCTION: Fonksiyon oluştur
-- cleanup_old_logs: Fonksiyonun adı
-- days_to_keep INT DEFAULT 90: Kaç günlük log tutulacak (varsayılan 90 gün = 3 ay)
RETURNS INTEGER AS $$
-- RETURNS INTEGER: Kaç satır silindiğini döndürür
-- $$ ... $$: Fonksiyon kodu
DECLARE
    deleted_count INTEGER;
    -- deleted_count: Silinen satır sayısını saklamak için değişken
BEGIN
    -- BEGIN: Fonksiyon kodu başlıyor
    
    DELETE FROM session_logs 
    WHERE created_at < NOW() - INTERVAL '1 day' * days_to_keep
    -- DELETE FROM session_logs: session_logs tablosundan sil
    -- WHERE created_at < NOW() - INTERVAL '1 day' * days_to_keep:
    --       Oluşturma tarihi, (bugün - days_to_keep gün) değerinden eski olanları
    --
    -- ÖRNEK HESAPLAMA:
    -- days_to_keep = 90
    -- Bugün = 2024-01-15
    -- NOW() - INTERVAL '1 day' * 90 = 2023-10-17
    -- created_at < 2023-10-17 olan loglar silinecek
    -- Yani 90 günden eski loglar silinir
    RETURNING id INTO deleted_count;
    -- RETURNING id: Silinen satırların id'lerini döndür
    -- INTO deleted_count: Kaç satır silindiğini say ve deleted_count'a ata
    
    -- Ama PostgreSQL RETURNING ile COUNT yapamıyor direkt
    -- Şöyle düzeltelim:
    GET DIAGNOSTICS deleted_count = ROW_COUNT;
    -- GET DIAGNOSTICS: En son işlemin istatistiklerini al
    -- ROW_COUNT: Kaç satır etkilendi?
    -- deleted_count'a ata
    
    RAISE NOTICE '% adet eski log kaydı silindi (% günden eski).', deleted_count, days_to_keep;
    -- RAISE NOTICE: Ekrana bilgi mesajı yazdır
    -- %: Placeholder (değişken yerine geçer)
    -- İlk % = deleted_count
    -- İkinci % = days_to_keep
    
    RETURN deleted_count;
    -- Silinen satır sayısını döndür
END;
$$ LANGUAGE plpgsql;
-- LANGUAGE plpgsql: PostgreSQL programlama dili

-- Bu fonksiyonu manuel çağırma:
-- SELECT cleanup_old_logs(90);  -- 90 günden eski logları sil
-- SELECT cleanup_old_logs(30);  -- 30 günden eski logları sil
-- SELECT cleanup_old_logs();    -- Varsayılan: 90 günden eski logları sil

-- CRON JOB İLE OTOMATİK ÇALIŞTIRMA:
-- Her gün gece saat 03:00'da eski logları temizle:
-- crontab -e
-- 0 3 * * * psql -U server_user -d server_db -c "SELECT cleanup_old_logs(90);"

-- ====================================================================
-- 4. İSTATİSTİK FONKSİYONU
-- ====================================================================
-- Logları özetlemek için yardımcı fonksiyon

CREATE OR REPLACE FUNCTION get_log_stats(days INT DEFAULT 7)
-- get_log_stats: Fonksiyonun adı
-- days: Kaç günlük istatistik? (varsayılan 7 gün)
RETURNS TABLE (
    action_type VARCHAR,
    total_count BIGINT,
    unique_users BIGINT,
    last_occurrence TIMESTAMP
) AS $$
-- RETURNS TABLE: Tablo formatında sonuç döndürür (birden fazla satır)
-- Her satırda: action_type, total_count, unique_users, last_occurrence
BEGIN
    RETURN QUERY
    -- RETURN QUERY: Sorgu sonucunu döndür
    SELECT 
        action::VARCHAR as action_type,
        -- action: Aksiyon tipi (LOGIN, LOGOUT, vb.)
        COUNT(*) as total_count,
        -- COUNT(*): Bu aksiyondan kaç tane var
        COUNT(DISTINCT user_id) as unique_users,
        -- COUNT(DISTINCT user_id): Kaç farklı kullanıcı bu aksiyonu yaptı
        MAX(created_at) as last_occurrence
        -- MAX(created_at): Bu aksiyonun en son ne zaman yapıldığı
    FROM session_logs
    WHERE created_at > NOW() - INTERVAL '1 day' * days
    -- Son N gün içindeki logları getir
    GROUP BY action
    -- Aksiyona göre grupla
    ORDER BY total_count DESC;
    -- En çok yapılan aksiyonu en üste al
END;
$$ LANGUAGE plpgsql;

-- Kullanım:
-- SELECT * FROM get_log_stats(7);   -- Son 7 günün istatistikleri
-- SELECT * FROM get_log_stats(30);  -- Son 30 günün istatistikleri

-- ÖRNEK ÇIKTI:
-- +------------------+-------------+--------------+---------------------+
-- | action_type      | total_count | unique_users | last_occurrence     |
-- +------------------+-------------+--------------+---------------------+
-- | LOGIN            |    1250     |     150      | 2024-01-15 18:30:00 |
-- | LOGOUT           |    1100     |     145      | 2024-01-15 18:25:00 |
-- | MESSAGE_SENT     |    8500     |     120      | 2024-01-15 18:35:00 |
-- | LOGIN_FAILED     |     45      |      15      | 2024-01-15 17:00:00 |
-- | PASSWORD_CHANGE  |      8      |       8      | 2024-01-14 10:15:00 |
-- +------------------+-------------+--------------+---------------------+

-- ====================================================================
-- 5. TABLO HAKKINDA AÇIKLAMA EKLE
-- ====================================================================
COMMENT ON TABLE session_logs IS 'Kullanıcı aktivite kayıtlarını saklayan log tablosu. Her önemli olay (giriş, çıkış, değişiklik) burada kaydedilir. Güvenlik, hata ayıklama, istatistik ve yasal uyumluluk için kullanılır.';

COMMENT ON COLUMN session_logs.id IS 'Otomatik artan benzersiz log kaydı kimliği';
COMMENT ON COLUMN session_logs.user_id IS 'İşlemi yapan kullanıcının ID''si (users.id foreign key, NULL olabilir: sistem logu veya anonim)';
COMMENT ON COLUMN session_logs.action IS 'Yapılan aksiyonun tipi (LOGIN, LOGOUT, PASSWORD_CHANGE, vb.)';
COMMENT ON COLUMN session_logs.details IS 'Aksiyon hakkında detaylı açıklama (JSON formatında olabilir)';
COMMENT ON COLUMN session_logs.ip_address IS 'İşlemin yapıldığı IP adresi (güvenlik ve coğrafi analiz için)';
COMMENT ON COLUMN session_logs.created_at IS 'Log kaydının oluşturulma tarihi ve saati';

-- ====================================================================
-- TABLO YAPISI GÖRSELLEŞTİRME
-- ====================================================================
-- Session_logs tablosu Excel'de şöyle görünür:
--
-- +-----+---------+------------------+--------------------------------+-----------------+---------------------+
-- | id  | user_id | action           | details                        | ip_address      | created_at          |
-- +-----+---------+------------------+--------------------------------+-----------------+---------------------+
-- |  1  |    5    | LOGIN            | Login successful from Chrome   | 192.168.1.100   | 2024-01-15 14:00:00 |
-- |  2  |    5    | MESSAGE_SENT     | Message sent to #general       | 192.168.1.100   | 2024-01-15 14:05:00 |
-- |  3  |   NULL  | LOGIN_FAILED     | Invalid password: ahmet        | 192.168.1.101   | 2024-01-15 14:10:00 |
-- |  4  |    7    | USER_BANNED      | Banned by admin for spam       | 192.168.1.102   | 2024-01-15 14:15:00 |
-- |  5  |    5    | LOGOUT           | User logged out                | 192.168.1.100   | 2024-01-15 14:30:00 |
-- |  6  |    1    | PERMISSION_CHG   | Changed user5 to MODERATOR     | 10.0.0.1        | 2024-01-15 14:35:00 |
-- +-----+---------+------------------+--------------------------------+-----------------+---------------------+
--         ^                                                                 ^              ^
--         |                                                                 |              |
--    Kullanıcı ID                                                       IP adresi      Ne zaman?
--    (NULL = sistem logu)
--
-- AÇIKLAMA:
-- * id=1: user_id=5 (ahmet) giriş yaptı, IP: 192.168.1.100
-- * id=2: ahmet mesaj gönderdi, aynı IP'den
-- * id=3: Başarısız giriş denemesi, user_id=NULL (giriş başarısız olduğu için)
-- * id=4: user_id=7 (mehmet) banlandı, admin tarafından
-- * id=5: ahmet çıkış yaptı
-- * id=6: admin (user_id=1) bir kullanıcının yetkisini değiştirdi

-- ====================================================================
-- İLİŞKİ DİYAGRAMI
-- ====================================================================
--
--   ┌─────────────────┐         ┌─────────────────────┐
--   │     USERS       │         │   SESSION_LOGS      │
--   ├─────────────────┤         ├─────────────────────┤
--   │ id (PK)         │◄───────┤ user_id (FK)        │  (Opsiyonel ilişki)
--   │ username        │         │ action              │
--   │ ...             │         │ details             │
--   └─────────────────┘         │ ip_address          │
--        (1)                    │ created_at          │
--                               └─────────────────────┘
--                                      (*)
--                              Bir kullanıcı birden fazla
--                              log kaydına sahip olabilir
--
-- * user_id NULL olabilir (sistem logu, anonim)
-- * ON DELETE SET NULL: Kullanıcı silinse bile loglar kalır

-- ====================================================================
-- ÖRNEK SORGULAR (Nasıl Kullanılır?)
-- ====================================================================

-- 1. Yeni log oluştur:
-- INSERT INTO session_logs (user_id, action, details, ip_address)
-- VALUES (5, 'LOGIN', 'Login successful from Chrome browser', '192.168.1.100');

-- 2. Kullanıcının son 10 aktivitesini göster:
-- SELECT action, details, ip_address, created_at 
-- FROM session_logs 
-- WHERE user_id = 5 
-- ORDER BY created_at DESC 
-- LIMIT 10;

-- 3. Son 24 saatteki tüm giriş başarısızlıklarını göster:
-- SELECT * FROM session_logs 
-- WHERE action = 'LOGIN_FAILED' 
--   AND created_at > NOW() - INTERVAL '24 hours'
-- ORDER BY created_at DESC;

-- 4. En aktif kullanıcıları bul (son 7 gün):
-- SELECT user_id, u.username, COUNT(*) as activity_count 
-- FROM session_logs sl 
-- JOIN users u ON sl.user_id = u.id 
-- WHERE created_at > NOW() - INTERVAL '7 days' 
-- GROUP BY user_id, u.username 
-- ORDER BY activity_count DESC 
-- LIMIT 10;

-- 5. Belirli bir IP'nin tüm aktivitelerini göster:
-- SELECT * FROM session_logs 
-- WHERE ip_address = '192.168.1.100' 
-- ORDER BY created_at DESC;

-- 6. Aksiyon tipine göre istatistik:
-- SELECT action, COUNT(*) as count 
-- FROM session_logs 
-- WHERE created_at > NOW() - INTERVAL '30 days' 
-- GROUP BY action 
-- ORDER BY count DESC;

-- 7. Şüpheli IP'leri bul (çok fazla başarısız giriş):
-- SELECT ip_address, COUNT(*) as fail_count 
-- FROM session_logs 
-- WHERE action = 'LOGIN_FAILED' 
--   AND created_at > NOW() - INTERVAL '1 hour' 
-- GROUP BY ip_address 
-- HAVING COUNT(*) > 5 
-- ORDER BY fail_count DESC;

-- 8. Kullanıcının giriş geçmişi (son 10 giriş):
-- SELECT created_at, ip_address 
-- FROM session_logs 
-- WHERE user_id = 5 AND action = 'LOGIN' 
-- ORDER BY created_at DESC 
-- LIMIT 10;

-- ====================================================================
-- C++ KODUNDA KULLANIM
-- ====================================================================
-- DatabaseManager sınıfında log metodu:
--
-- void LogActivity(int user_id, const string& action, const string& details, const string& ip_address) {
--     pqxx::work txn(conn);
--     
--     if (user_id > 0) {
--         // Normal kullanıcı logu
--         txn.exec_params(
--             "INSERT INTO session_logs (user_id, action, details, ip_address) "
--             "VALUES ($1, $2, $3, $4)",
--             user_id, action, details, ip_address
--         );
--     } else {
--         // Sistem logu (user_id yok)
--         txn.exec_params(
--             "INSERT INTO session_logs (user_id, action, details, ip_address) "
--             "VALUES (NULL, $1, $2, $3)",
--             action, details, ip_address
--         );
--     }
--     
--     txn.commit();
-- }
--
-- // Kullanım örnekleri:
-- LogActivity(5, "LOGIN", "Login successful", "192.168.1.100");
-- LogActivity(5, "MESSAGE_SENT", "Message to #general", "192.168.1.100");
-- LogActivity(0, "SYSTEM_STARTUP", "Server started", "127.0.0.1");  // Sistem logu
-- LogActivity(5, "LOGOUT", "User logged out", "192.168.1.100");

-- ====================================================================
-- GÜVENLİK VE GİZLİLİK ÖNEMLİ NOTLAR
-- ====================================================================
-- 1. KİŞİSEL VERİ KORUMA (KVKK/GDPR):
--    * IP adresi kişisel veridir, izin alınmalı
--    * Belirli süre sonra silinmeli veya anonimleştirilmeli
--    * Kullanıcılar kendi loglarını görebilmeli
--
-- 2. ŞİFRE ASLA LOGLANMAZ:
--    * Logda asla şifre, token, kredi kartı bilgisi olmamalı
--    * details alanına hassas bilgi yazılmamalı
--    * "Password changed" DOĞRU, "Password changed to 123456" YANLIŞ
--
-- 3. LOG BOYUTU:
--    * Loglar çok yer kaplar, düzenli temizlenmeli
--    * Önemli loglar yedeklenmeli (backup)
--    * Eski loglar arşivlenebilir (ayrı tablo/veritabanı)
--
-- 4. PERFORMANS:
--    * Her istekte log yazılır, veritabanı yükü artar
--    * Asenkron loglama kullanılabilir (queue sistemi)
--    * Kritik olmayan loglar toplu yazılabilir (batch insert)
--
-- 5. SAAT DİLİMİ:
--    * Tüm loglar aynı timezone'da olmalı (sunucu saati)
--    * Gösterirken kullanıcının timezone'una çevrilebilir
--    * UTC kullanımı önerilir (evrensel standart)

-- ====================================================================
-- SON NOTLAR
-- ====================================================================
-- * FOREIGN KEY ile users'a bağlı ama NULL olabilir
-- * ON DELETE SET NULL: Kullanıcı silinse bile loglar kalır
-- * TEXT: Sınırsız uzunlukta detay yazılabilir
-- * TIMESTAMP: created_at otomatik oluşur (DEFAULT NOW())
-- * INDEX: user_id, action, created_at, ip_address için performans
-- * Eski logları silmek için cleanup_old_logs() fonksiyonu
-- * İstatistik için get_log_stats() fonksiyonu
-- * CRON job ile otomatik temizlik yapılabilir
-- * Güvenlik ve gizlilik kurallarına dikkat edilmeli