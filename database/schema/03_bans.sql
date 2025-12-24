-- ====================================================================
-- YASAKLAMA (BAN) YÖNETİM TABLOSU
-- ====================================================================
-- Bu dosya kullanıcı yasaklama (ban) işlemlerini yöneten tabloyu oluşturur
--
-- BAN NEDİR?
-- Ban, bir kullanıcının sisteme erişimini engellemek demektir
-- Türkçe'de "yasak", "engelleme" anlamına gelir
--
-- BAN TÜRLERİ:
-- 1. GEÇİCİ BAN: Belirli bir süre için (örn: 3 gün, 1 hafta)
-- 2. KALICI BAN: Süresiz, manuel olarak kaldırılana kadar
--
-- GERÇEK HAYAT ÖRNEK:
-- * Oyun oynayan birisi hile yapıyor --> 7 günlük ban
-- * Forum'da spam yapan --> Kalıcı ban
-- * Chat'te küfür eden --> 24 saatlik ban
--
-- BU SİSTEMDE:
-- * Admin veya moderatör bir kullanıcıyı banlayabilir
-- * Ban sebebi mutlaka yazılmalı (reason)
-- * Geçici ban süresi dolunca otomatik açılır
-- * Ban geçmişi saklanır (silinmez, is_active=FALSE olur)

-- ====================================================================
-- 1. BANS TABLOSUNU OLUŞTUR
-- ====================================================================
CREATE TABLE IF NOT EXISTS bans (
    -- IF NOT EXISTS: Tablo varsa hata verme, yoksa oluştur
    
    -- ----------------------------------------------------------------
    -- SÜTUN 1: Ban ID (Benzersiz Ban Kaydı Kimliği)
    -- ----------------------------------------------------------------
    id SERIAL PRIMARY KEY,
    -- SERIAL: Otomatik artan sayı (1, 2, 3, 4, ...)
    --         Her yeni ban kaydı için PostgreSQL otomatik numara verir
    -- PRIMARY KEY: Bu tablonun ana anahtarı
    --              * Benzersiz (unique) - her ban'ın kendine özel ID'si
    --              * Boş olamaz (not null)
    --              * Hızlı arama için otomatik indeks oluşur
    --
    -- NEDEN GEREK: Aynı kullanıcı birden fazla kez banlanabilir
    --              Her ban olayını ayrı ayrı kaydetmek için
    --              Ban geçmişini takip edebilmek için
    
    -- ----------------------------------------------------------------
    -- SÜTUN 2: Banlanan Kullanıcının ID'si
    -- ----------------------------------------------------------------
    user_id INT REFERENCES users(id) ON DELETE CASCADE,
    -- INT: Tam sayı - users tablosundaki id ile eşleşir
    -- REFERENCES users(id): Bu sütun users tablosundaki id sütununa referans verir
    --                       FOREIGN KEY (Yabancı Anahtar) ilişkisi
    --                       İLİŞKİ: Her ban bir kullanıcıya aittir
    --
    -- FOREIGN KEY ÇALIŞMA MANTIĞI:
    --   users tablosu        bans tablosu
    --   +-----------+        +----------+
    --   | id | name |        | user_id  | --> users.id'ye işaret eder
    --   +-----------+        +----------+
    --   | 1  | ahmet|   <--- | 1        | (ahmet banlandı)
    --   | 2  | mehmet|  <--- | 2        | (mehmet banlandı)
    --   | 3  | ayşe |   <--- | 3        | (ayşe banlandı)
    --   +-----------+        | 1        | (ahmet tekrar banlandı)
    --                        +----------+
    --
    -- ON DELETE CASCADE: Kullanıcı silinirse, ban kayıtları da SİLİNİR
    --                    Örnek: user_id=5 olan kullanıcı silinirse
    --                           user_id=5 olan tüm ban kayıtları otomatik silinir
    --                    MANTIK: Kullanıcı yoksa ban kaydı anlamsız
    --                    Alternatif: ON DELETE SET NULL --> user_id'yi NULL yap
    --                                (Ban kaydı kalır ama sahibi bilinmez)
    
    -- ----------------------------------------------------------------
    -- SÜTUN 3: Banı Veren Kişinin ID'si
    -- ----------------------------------------------------------------
    banned_by_id INT REFERENCES users(id) ON DELETE SET NULL,
    -- INT: Tam sayı - users tablosundaki admin/moderator id'si
    -- REFERENCES users(id): Yine users tablosuna referans (başka bir ilişki)
    --
    -- İKİNCİ BİR İLİŞKİ:
    --   users tablosu           bans tablosu
    --   +-------------+        +--------------+
    --   | id | name   |        | banned_by_id | --> users.id (admin/mod)
    --   +-------------+        +--------------+
    --   | 10 | admin  |   <--- | 10           | (admin banladı)
    --   | 11 | mod1   |   <--- | 11           | (mod1 banladı)
    --   +-------------+        | 10           | (admin yine banladı)
    --                          +--------------+
    --
    -- ON DELETE SET NULL: Eğer admin hesabı silinirse, banned_by_id = NULL olur
    --                     Ban kaydı kalır ama kimin banladığı bilinmez
    --                     MANTIK: Ban geçmişi önemli, admin silinse bile
    --                            ban kaydı silinmemeli, sadece "bilinmiyor" olsun
    --                     Alternatif: CASCADE kullanırsak admin silinince
    --                                tüm ban kayıtları da silinir (istemiyoruz)
    
    -- ----------------------------------------------------------------
    -- SÜTUN 4: Ban Sebebi
    -- ----------------------------------------------------------------
    reason TEXT NOT NULL,
    -- TEXT: Sınırsız uzunlukta metin (VARCHAR'dan farkı: karakter limiti yok)
    --       VARCHAR(255) yerine TEXT kullanıyoruz çünkü:
    --       * Ban sebebi uzun olabilir
    --       * Detaylı açıklama gerekebilir
    -- NOT NULL: Ban sebebi mutlaka yazılmalı
    --           "Neden banlandım?" sorusunun cevabı olmalı
    --
    -- ÖRNEK DEĞERLER:
    -- * "Spam mesaj gönderme"
    -- * "Diğer kullanıcılara hakaret"
    -- * "Sistem kurallarını ihlal"
    -- * "Hile kullanımı tespit edildi"
    -- * "Tekrarlayan uygunsuz davranış: 3. uyarı sonrası ban"
    --
    -- NEDEN ÖNEMLİ:
    -- * Kullanıcı neden banlandığını bilmeli (şeffaflık)
    -- * İtiraz durumunda kanıt olarak kullanılır
    -- * İstatistik: "En çok hangi sebeple ban veriliyor?" analizi
    
    -- ----------------------------------------------------------------
    -- SÜTUN 5: Ban Süresi (Dakika Cinsinden)
    -- ----------------------------------------------------------------
    duration_minutes INT DEFAULT 0,
    -- INT: Tam sayı - Ban süresi dakika olarak
    -- DEFAULT 0: Varsayılan değer 0
    --            0 = KALICI BAN (süresiz)
    --            0'dan büyük = GEÇİCİ BAN (süre bitince otomatik açılır)
    --
    -- ÖZEL DEĞER: 0 = KALICI BAN
    -- Eğer duration_minutes = 0 ise:
    --   * expires_at = NULL olur (son kullanma tarihi yok)
    --   * Ban süresiz devam eder
    --   * Sadece admin manuel olarak kaldırabilir
    --
    -- GEÇİCİ BAN ÖRNEKLERİ:
    -- * 60 dakika = 1 saat ban
    -- * 1440 dakika = 24 saat (1 gün) ban
    -- * 10080 dakika = 7 gün (1 hafta) ban
    -- * 43200 dakika = 30 gün (1 ay) ban
    --
    -- HESAPLAMA:
    -- 1 saat = 60 dakika
    -- 1 gün = 24 saat = 24 * 60 = 1440 dakika
    -- 1 hafta = 7 gün = 7 * 1440 = 10080 dakika
    -- 1 ay = 30 gün = 30 * 1440 = 43200 dakika
    --
    -- C++ KODUNDA:
    -- duration_minutes = 0;           // Kalıcı ban
    -- duration_minutes = 60;          // 1 saat
    -- duration_minutes = 24 * 60;     // 1 gün
    -- duration_minutes = 7 * 24 * 60; // 1 hafta
    
    -- ----------------------------------------------------------------
    -- SÜTUN 6: Ban Verilme Zamanı
    -- ----------------------------------------------------------------
    banned_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    -- TIMESTAMP: Tarih ve saat bilgisi
    -- DEFAULT CURRENT_TIMESTAMP: Ban verildiğinde otomatik olarak o anki zaman
    -- CURRENT_TIMESTAMP = NOW() = Şu anki tarih ve saat
    --
    -- ÖRNEK: '2024-01-15 18:30:45'
    -- Bu değer hiç değişmez - ban ne zaman verildiğini gösterir
    --
    -- KULLANIM AMACI:
    -- * "Bu kullanıcı 3 gün önce banlandı" bilgisi
    -- * İstatistik: "Bu ay kaç ban verildi?" analizi
    -- * Geçmiş takibi: "Son 1 yılda kaç kez banlandı?" sorgusu
    
    -- ----------------------------------------------------------------
    -- SÜTUN 7: Ban Bitiş Zamanı
    -- ----------------------------------------------------------------
    expires_at TIMESTAMP,
    -- TIMESTAMP: Tarih ve saat
    -- NOT NULL YOK: Bu alan NULL olabilir
    --               NULL = Kalıcı ban (süre yok)
    --               Değer varsa = Geçici ban (bu tarihte otomatik açılır)
    --
    -- NASIL HESAPLANIR:
    -- expires_at = banned_at + (duration_minutes dakika)
    --
    -- ÖRNEK HESAPLAMA:
    -- * banned_at = '2024-01-15 14:00:00'
    -- * duration_minutes = 1440 (1 gün)
    -- * expires_at = '2024-01-16 14:00:00' (1 gün sonra)
    --
    -- SQL'de hesaplama:
    -- expires_at = banned_at + INTERVAL '1440 minutes'
    -- expires_at = NOW() + INTERVAL '24 hours'
    --
    -- NULL DURUMU:
    -- * duration_minutes = 0 ise expires_at = NULL
    -- * NULL kontrolü: IF expires_at IS NULL THEN 'Kalıcı ban'
    
    -- ----------------------------------------------------------------
    -- SÜTUN 8: Ban Aktif mi?
    -- ----------------------------------------------------------------
    is_active BOOLEAN DEFAULT TRUE
    -- BOOLEAN: TRUE (doğru) veya FALSE (yanlış)
    -- DEFAULT TRUE: Ban verildiğinde başlangıçta aktif
    --
    -- DEĞERLER:
    -- * TRUE = Ban aktif, kullanıcı sistemde engellenmiş
    -- * FALSE = Ban pasif, artık etkisiz (süresi dolmuş veya kaldırılmış)
    --
    -- NEDEN BU ALAN VAR?
    -- Ban kayıtlarını silmiyoruz, sadece pasif hale getiriyoruz
    -- Böylece geçmiş takip edilebilir
    --
    -- SENARYO 1: Geçici Ban Süresi Doldu
    -- * Kullanıcı 24 saatlik ban aldı
    -- * 24 saat sonra otomatik olarak is_active = FALSE olur
    -- * Kullanıcı tekrar sisteme girebilir
    -- * Ama ban kaydı veritabanında kalır (geçmiş için)
    --
    -- SENARYO 2: Admin Ban'ı Kaldırdı
    -- * Kullanıcı kalıcı ban almıştı
    -- * Admin "affediyorum" dedi, ban'ı kaldırdı
    -- * is_active = FALSE yapılır
    -- * Ama kayıt silinmez, "bu kullanıcı daha önce banlanmıştı" bilgisi kalır
    --
    -- SORGU ÖRNEĞİ:
    -- Aktif banları göster:
    -- SELECT * FROM bans WHERE is_active = TRUE
    --
    -- Geçmiş banları göster:
    -- SELECT * FROM bans WHERE is_active = FALSE
    --
    -- Kullanıcının ban geçmişi:
    -- SELECT * FROM bans WHERE user_id = 5 ORDER BY banned_at DESC
);

-- ====================================================================
-- 2. PERFORMANS İÇİN İNDEKSLER OLUŞTUR
-- ====================================================================

-- ----------------------------------------------------------------
-- INDEX 1: Kullanıcıya Göre Ban Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_bans_user ON bans(user_id);
-- idx_bans_user: İndeksin ismi
-- ON bans(user_id): bans tablosunda user_id sütununa indeks
--
-- NEDEN LAZIM:
-- * "Bu kullanıcı banlanmış mı?" kontrolü çok sık yapılır
--   SELECT * FROM bans WHERE user_id = 5 AND is_active = TRUE
-- * Kullanıcı giriş yaptığında her seferinde kontrol edilir
-- * Kullanıcının ban geçmişini göstermek için
--   SELECT * FROM bans WHERE user_id = 5 ORDER BY banned_at DESC
--
-- PERFORMANS:
-- * İndeks yok: 100,000 ban varsa hepsini kontrol eder (yavaş)
-- * İndeks var: Direkt user_id=5 olanları bulur (hızlı)

-- ----------------------------------------------------------------
-- INDEX 2: Aktif Ban Durumuna Göre Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_bans_active ON bans(is_active);
-- ON bans(is_active): is_active sütununa indeks
--
-- NEDEN LAZIM:
-- * Tüm aktif banları listelemek için
--   SELECT * FROM bans WHERE is_active = TRUE
-- * İstatistik: "Şu anda kaç kişi banlanmış?" sorgusu
-- * Admin panelinde "Aktif Banlar" sayfası için
--
-- NOT: BOOLEAN indeks genellikle çok verimli olmaz (sadece 2 değer: TRUE/FALSE)
-- Ama bu tabloda çoğunluk FALSE olur (geçmiş kayıtlar)
-- Aktif ban sayısı az olur, indeks yararlı olur

-- ----------------------------------------------------------------
-- INDEX 3: Bitiş Tarihine Göre Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_bans_expires ON bans(expires_at);
-- ON bans(expires_at): expires_at sütununa indeks
--
-- NEDEN LAZIM:
-- * Süresi dolmuş banları otomatik pasif yapmak için
--   SELECT * FROM bans WHERE expires_at < NOW() AND is_active = TRUE
-- * Bu sorgu periyodik olarak (her dakika/saat) çalıştırılır
-- * İndeks olmazsa tüm ban kayıtlarını kontrol eder (yavaş)
--
-- CRON JOB ÖRNEK:
-- Her 5 dakikada bir çalışan otomatik görev:
-- UPDATE bans SET is_active = FALSE 
-- WHERE expires_at < NOW() AND is_active = TRUE;

-- ====================================================================
-- 3. OTOMATİK BAN SÜRESİ KONTROLÜ (TRIGGER)
-- ====================================================================
-- Ban güncellendiğinde otomatik olarak süresinin dolup dolmadığını kontrol et

CREATE OR REPLACE FUNCTION check_ban_expiry()
-- CREATE OR REPLACE FUNCTION: Fonksiyon oluştur, varsa üstüne yaz
-- check_ban_expiry(): Fonksiyonun adı
RETURNS TRIGGER AS $$
-- RETURNS TRIGGER: Bu fonksiyon bir trigger tarafından çağrılacak
-- $$ ... $$: Fonksiyon kodunu içine yazdığımız alan
BEGIN
    -- BEGIN: Fonksiyon kodu başlıyor
    
    -- Ban süresinin dolup dolmadığını kontrol et
    IF NEW.expires_at IS NOT NULL AND NEW.expires_at < CURRENT_TIMESTAMP THEN
        -- IF: Eğer
        -- NEW.expires_at IS NOT NULL: Bitiş tarihi varsa (kalıcı ban değilse)
        -- AND: VE
        -- NEW.expires_at < CURRENT_TIMESTAMP: Bitiş tarihi geçmişse
        -- THEN: O ZAMAN
        
        -- 1. Ban'ı pasif yap
        NEW.is_active = FALSE;
        -- NEW: Güncellenmiş yeni veri
        -- NEW.is_active: Yeni verinin is_active değeri
        -- FALSE'a ayarla = Ban artık aktif değil
        
        -- 2. Kullanıcının yetkisini normale çevir
        UPDATE users SET permission_level = 2 WHERE id = NEW.user_id;
        -- UPDATE users: users tablosunu güncelle
        -- SET permission_level = 2: Yetkiyi USER (normal kullanıcı) yap
        -- WHERE id = NEW.user_id: Banlanan kullanıcıyı bul
        --
        -- YETKİ SEVİYELERİ:
        -- 0 = ADMIN
        -- 1 = MODERATOR  
        -- 2 = USER (normal)
        -- 3 = GUEST
        -- 4 = BANNED
        --
        -- Ban aktif olduğunda permission_level = 4 (BANNED) yapılmıştı
        -- Şimdi ban bittiği için permission_level = 2 (USER) yapıyoruz
        -- Kullanıcı tekrar normal haklarına kavuşuyor
        
    END IF;
    -- IF bloğu bitiyor
    
    RETURN NEW;
    -- RETURN NEW: Güncellenmiş veriyi geri döndür
    -- Bu sayede değişiklikler veritabanına kaydedilir
END;
-- Fonksiyon sonu
$$ LANGUAGE plpgsql;
-- LANGUAGE plpgsql: PostgreSQL programlama dili

-- Şimdi bu fonksiyonu çalıştıracak TRIGGER'ı oluştur:
CREATE TRIGGER check_ban_expiry_trigger
    -- check_ban_expiry_trigger: Trigger'ın ismi
    BEFORE UPDATE ON bans
    -- BEFORE UPDATE: bans tablosunda UPDATE olmadan ÖNCE çalış
    -- ON bans: Bu trigger bans tablosu için geçerli
    FOR EACH ROW
    -- FOR EACH ROW: Her güncellenen satır için ayrı ayrı çalış
    EXECUTE FUNCTION check_ban_expiry();
    -- EXECUTE FUNCTION: Yukarıda tanımladığımız fonksiyonu çalıştır

-- TRIGGER ÇALIŞMA MANTIĞI:
-- 1. Sistem her 5 dakikada bir:
--    UPDATE bans SET expires_at = expires_at WHERE expires_at IS NOT NULL;
--    (Tüm geçici banları güncelle - değişiklik yok ama trigger tetiklenir)
-- 2. Her güncellenen satır için check_ban_expiry() çalışır
-- 3. Fonksiyon expires_at'e bakar
-- 4. Eğer süre dolmuşsa:
--    - is_active = FALSE yapar
--    - users tablosunda permission_level = 2 yapar
-- 5. Kullanıcı artık sisteme girebilir

-- ====================================================================
-- 4. SCHEMA VERSİYON GÜNCELLE
-- ====================================================================
-- Veritabanı şemasının hangi versiyonda olduğunu takip et

INSERT INTO schema_version (version, description) VALUES 
    (3, 'Bans table created');
-- INSERT INTO schema_version: schema_version tablosuna kayıt ekle
--                              (Bu tablo 00_init.sql'de oluşturuldu)
-- VALUES: Eklenecek değerler
--   version = 3: Bu schema dosyasının sürüm numarası
--                (00_init.sql = 0, 01_users.sql = 1, 02_tokens.sql = 2, bu dosya = 3)
--   description = 'Bans table created': Açıklama
--
-- NEDEN LAZIM:
-- * Hangi schema dosyalarının çalıştırıldığını takip etmek için
-- * "SELECT * FROM schema_version" ile veritabanının durumunu görebiliriz
-- * Migration (göç) işlemlerinde hangi adımda olduğumuzu bilmek için

-- Bu satır hata verirse (zaten varsa) devam et, hata verme:
-- ON CONFLICT (version) DO NOTHING;
-- (Daha önceki SQL sürümlerinde kullanılmıştı, burada yok ama eklenebilir)

-- ====================================================================
-- 5. TABLO HAKKINDA AÇIKLAMA EKLE
-- ====================================================================
COMMENT ON TABLE bans IS 'Kullanıcı yasaklama (ban) kayıtlarını saklayan tablo. Her ban olayı, sebebi, süresi ve durumu burada tutulur. Ban geçmişi silinmez, sadece pasif hale getirilir.';

-- Sütunlar için açıklamalar:
COMMENT ON COLUMN bans.id IS 'Otomatik artan benzersiz ban kaydı kimliği';
COMMENT ON COLUMN bans.user_id IS 'Banlanan kullanıcının ID''si (users.id foreign key)';
COMMENT ON COLUMN bans.banned_by_id IS 'Ban''ı veren admin/moderator''ün ID''si (users.id foreign key, silinirse NULL olur)';
COMMENT ON COLUMN bans.reason IS 'Ban sebebi - Neden banlandığının açıklaması';
COMMENT ON COLUMN bans.duration_minutes IS 'Ban süresi dakika olarak (0 = kalıcı ban, >0 = geçici ban)';
COMMENT ON COLUMN bans.banned_at IS 'Ban''ın verildiği tarih ve saat';
COMMENT ON COLUMN bans.expires_at IS 'Ban''ın biteceği tarih ve saat (NULL = kalıcı ban, değer varsa = geçici ban)';
COMMENT ON COLUMN bans.is_active IS 'Ban aktif mi? (TRUE = aktif/engellenmiş, FALSE = pasif/süresi dolmuş)';

-- ====================================================================
-- TABLO YAPISI GÖRSELLEŞTİRME
-- ====================================================================
-- Bans tablosu Excel'de şöyle görünür:
--
-- +----+---------+--------------+----------------------+---------+---------------------+---------------------+-----------+
-- | id | user_id | banned_by_id | reason               | duration| banned_at           | expires_at          | is_active |
-- +----+---------+--------------+----------------------+---------+---------------------+---------------------+-----------+
-- |  1 |    5    |      1       | Spam mesaj gönderme  |  1440   | 2024-01-15 10:00:00 | 2024-01-16 10:00:00 |   TRUE    |
-- |  2 |    7    |      1       | Küfür ve hakaret     |     0   | 2024-01-14 15:30:00 | NULL                |   TRUE    |
-- |  3 |    5    |      2       | Tekrar spam          |  10080  | 2024-01-10 12:00:00 | 2024-01-17 12:00:00 |   FALSE   |
-- |  4 |    9    |      1       | Hile kullanımı       |  43200  | 2024-01-01 09:00:00 | 2024-01-31 09:00:00 |   FALSE   |
-- +----+---------+--------------+----------------------+---------+---------------------+---------------------+-----------+
--        ^            ^                                    ^                                ^              ^
--        |            |                                    |                                |              |
--   Banlanan     Ban veren                          0 = Kalıcı                      NULL = Kalıcı   Aktif mi?
--   kullanıcı    (admin/mod)                        1440 = 1 gün                    Değer var = Geçici
--                                                    10080 = 7 gün
--
-- AÇIKLAMA:
-- * id=1: user_id=5 (ahmet) 1 günlüğüne banlandı, ban aktif
-- * id=2: user_id=7 (mehmet) kalıcı banlandı (duration=0, expires_at=NULL), ban aktif
-- * id=3: user_id=5 (ahmet) daha önce 7 günlük ban almıştı, süresi dolmuş (is_active=FALSE)
-- * id=4: user_id=9 (ayşe) 30 günlük ban almıştı, süresi dolmuş

-- ====================================================================
-- İLİŞKİ DİYAGRAMI
-- ====================================================================
--
--   ┌─────────────────┐         ┌─────────────────┐
--   │     USERS       │         │      BANS       │
--   ├─────────────────┤         ├─────────────────┤
--   │ id (PK)         │◄───────┤ user_id (FK)    │  // Banlanan kullanıcı
--   │ username        │         │ banned_by_id(FK)│◄─┐
--   │ permission_level│         │ reason          │  │
--   │ ...             │         │ duration_minutes│  │
--   └─────────────────┘         │ expires_at      │  │
--        ▲                      │ is_active       │  │
--        │                      └─────────────────┘  │
--        └──────────────────────────────────────────┘
--                Ban veren admin/moderator
--
-- İKİ FOREIGN KEY İLİŞKİSİ:
-- 1. user_id --> users.id: Hangi kullanıcı banlandı?
-- 2. banned_by_id --> users.id: Hangi admin/moderator banladı?
--
-- SELF-REFERENCING (Kendine Referans):
-- bans tablosu, aynı users tablosuna 2 farklı şekilde bağlanıyor
-- Bu tarz ilişkilere "self-referencing foreign key" denir

-- ====================================================================
-- ÖRNEK SORGULAR (Nasıl Kullanılır?)
-- ====================================================================

-- 1. Yeni ban oluştur (1 gün):
-- INSERT INTO bans (user_id, banned_by_id, reason, duration_minutes, expires_at)
-- VALUES (5, 1, 'Spam mesaj gönderme', 1440, NOW() + INTERVAL '1 day');
-- Ayrıca: UPDATE users SET permission_level = 4 WHERE id = 5;

-- 2. Kalıcı ban oluştur:
-- INSERT INTO bans (user_id, banned_by_id, reason, duration_minutes)
-- VALUES (7, 1, 'Ciddi kural ihlali', 0);
-- UPDATE users SET permission_level = 4 WHERE id = 7;

-- 3. Kullanıcı banlanmış mı kontrol et:
-- SELECT * FROM bans 
-- WHERE user_id = 5 AND is_active = TRUE;
-- Eğer sonuç dönerse: Banlanmış
-- Eğer sonuç dönmezse: Banlanmamış

-- 4. Kullanıcının tüm ban geçmişini göster:
-- SELECT b.*, u.username as banned_by 
-- FROM bans b 
-- LEFT JOIN users u ON b.banned_by_id = u.id 
-- WHERE b.user_id = 5 
-- ORDER BY b.banned_at DESC;

-- 5. Aktif banları listele:
-- SELECT b.*, 
--        u1.username as banned_user, 
--        u2.username as banned_by 
-- FROM bans b 
-- JOIN users u1 ON b.user_id = u1.id 
-- LEFT JOIN users u2 ON b.banned_by_id = u2.id 
-- WHERE b.is_active = TRUE;

-- 6. Ban'ı manuel olarak kaldır (affet):
-- UPDATE bans SET is_active = FALSE WHERE id = 1;
-- UPDATE users SET permission_level = 2 WHERE id = (SELECT user_id FROM bans WHERE id = 1);

-- 7. Süresi dolmuş banları pasif yap:
-- UPDATE bans 
-- SET is_active = FALSE 
-- WHERE expires_at < NOW() AND is_active = TRUE;
-- (Bu işlem trigger tarafından otomatik yapılır)

-- 8. En çok hangi sebepten ban veriliyor:
-- SELECT reason, COUNT(*) as count 
-- FROM bans 
-- GROUP BY reason 
-- ORDER BY count DESC 
-- LIMIT 10;

-- ====================================================================
-- C++ KODUNDA KULLANIM
-- ====================================================================
-- DatabaseManager sınıfında bu tablo için metodlar:
--
-- // Kullanıcıyı banla
-- void BanUser(int user_id, int banned_by_id, string reason, int duration_minutes) {
--     pqxx::work txn(conn);
--     
--     // Bitiş tarihini hesapla
--     string expires_at_query;
--     if (duration_minutes == 0) {
--         expires_at_query = "NULL";  // Kalıcı ban
--     } else {
--         expires_at_query = "NOW() + INTERVAL '" + to_string(duration_minutes) + " minutes'";
--     }
--     
--     // Ban kaydı oluştur
--     txn.exec_params(
--         "INSERT INTO bans (user_id, banned_by_id, reason, duration_minutes, expires_at) "
--         "VALUES ($1, $2, $3, $4, " + expires_at_query + ")",
--         user_id, banned_by_id, reason, duration_minutes
--     );
--     
--     // Kullanıcının yetkisini BANNED yap
--     txn.exec_params("UPDATE users SET permission_level = 4 WHERE id = $1", user_id);
--     
--     txn.commit();
-- }
--
-- // Kullanıcı banlanmış mı kontrol et
-- bool IsUserBanned(int user_id) {
--     pqxx::work txn(conn);
--     auto result = txn.exec_params(
--         "SELECT 1 FROM bans WHERE user_id = $1 AND is_active = TRUE LIMIT 1",
--         user_id
--     );
--     return !result.empty();
-- }

-- ====================================================================
-- SON NOTLAR VE İPUÇLARI
-- ====================================================================
-- * FOREIGN KEY: İki tablo arasında ilişki kurar, veri bütünlüğünü sağlar
-- * CASCADE vs SET NULL: Silme durumunda ne olacağını belirler
--   - CASCADE: İlişkili kayıtları da sil
--   - SET NULL: İlişkili alanı NULL yap (kayıt kalır)
--   - RESTRICT: Silmeye izin verme (hata ver)
-- * TRIGGER: Belirli olaylarda otomatik çalışan kod
--   - BEFORE: İşlem yapılmadan önce
--   - AFTER: İşlem yapıldıktan sonra
--   - FOR EACH ROW: Her satır için ayrı çalış
-- * duration_minutes = 0: Özel değer olarak kalıcı ban anlamında kullanılıyor
-- * is_active: Ban kayıtları silinmez, sadece pasif yapılır (geçmiş takibi için)
-- * TIMESTAMP: PostgreSQL'de tarih-saat saklama tipi
-- * INTERVAL: Zaman aralığı belirtmek için ('1 day', '24 hours', '1440 minutes' gibi)
-- * TEXT vs VARCHAR: TEXT sınırsız, VARCHAR(n) maksimum n karakter
