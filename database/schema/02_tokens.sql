-- ====================================================================
-- OTURUM TOKEN'LARI TABLOSU (SESSION TOKENS TABLE)
-- ====================================================================
-- Bu dosya kullanıcı oturum token'larını (session tokens) saklayan tabloyu oluşturur
--
-- TOKEN NEDİR?
-- Token, kullanıcının kimliğini doğrulamak için kullanılan benzersiz bir anahtardır
-- Şöyle düşünün:
-- 1. Kullanıcı giriş yapar (username + şifre)
-- 2. Sistem kullanıcıya bir token verir (örnek: "abc123xyz")
-- 3. Kullanıcı bundan sonra her istekte bu token'ı gönderir
-- 4. Sistem token'a bakarak "Bu kişi ahmet, izin verildi" der
--
-- NEDEN TOKEN KULLANIYORUZ?
-- * Her istekte kullanıcı adı + şifre göndermek güvensiz
-- * Token'lar belirli bir süre sonra otomatik olarak geçersiz olur
-- * Kullanıcı çıkış yapınca token silinir, başkası kullanamaz
--
-- GERÇEK HAYAT ÖRNEK:
-- Alışveriş merkezine girdiğinizde kimlik gösterip giriş kartı alırsınız
-- O kart (token) ile içerde dolaşırsınız
-- Kartın son kullanma saati vardır (expires_at)
-- Çıkışta kartı teslim edersiniz (logout)

-- ====================================================================
-- 1. TOKENS TABLOSUNU OLUŞTUR
-- ====================================================================
CREATE TABLE IF NOT EXISTS tokens (
    -- IF NOT EXISTS: Tablo varsa hata verme, yoksa oluştur
    
    -- ----------------------------------------------------------------
    -- SÜTUN 1: Token ID (Benzersiz Token Kimliği)
    -- ----------------------------------------------------------------
    id SERIAL PRIMARY KEY,
    -- SERIAL: Otomatik artan sayı (1, 2, 3, 4, ...)
    -- PRIMARY KEY: Bu tablonun ana anahtarı
    --              * Benzersiz (unique)
    --              * Boş olamaz (not null)
    --              * Hızlı arama için otomatik indeks oluşturulur
    -- Her token kaydının kendine özel bir numarası var
    
    -- ----------------------------------------------------------------
    -- SÜTUN 2: Kullanıcı ID'si (Bu token hangi kullanıcıya ait?)
    -- ----------------------------------------------------------------
    user_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    -- INT: Tam sayı, users tablosundaki id ile eşleşmeli
    -- NOT NULL: Boş olamaz, her token'ın mutlaka bir sahibi olmalı
    --
    -- REFERENCES users(id): Bu sütun users tablosundaki id sütununa referans verir
    --                       Buna FOREIGN KEY (Yabancı Anahtar) denir
    --                       İLİŞKİ: Her token bir kullanıcıya aittir
    --                       
    -- FOREIGN KEY NASIL ÇALIŞIR:
    --   users tablosu        tokens tablosu
    --   +-----------+        +----------+
    --   | id | name |        | user_id  | --> users.id'ye işaret eder
    --   +-----------+        +----------+
    --   | 1  | ahmet|   <--- | 1        | (ahmet'in token'ı)
    --   | 2  | mehmet|  <--- | 2        | (mehmet'in token'ı)
    --   | 3  | ayşe |   <--- | 3        | (ayşe'nin token'ı)
    --   +-----------+        | 1        | (ahmet'in başka token'ı)
    --                        +----------+
    --
    -- ON DELETE CASCADE: Eğer kullanıcı silinirse, token'ları da SİL
    --                    Örnek: user_id=5 olan kullanıcı silinirse,
    --                           user_id=5 olan tüm token'lar otomatik silinir
    --                    MANTIK: Kullanıcı yoksa token'ının olması anlamsız
    --                    Alternatifler:
    --                    * ON DELETE SET NULL: Kullanıcı silinince user_id'yi NULL yap
    --                    * ON DELETE RESTRICT: Kullanıcıyı silmeye izin verme (hata ver)
    --                    * CASCADE en mantıklısı bizim için
    
    -- ----------------------------------------------------------------
    -- SÜTUN 3: Token String (Benzersiz Anahtar)
    -- ----------------------------------------------------------------
    token VARCHAR(255) UNIQUE NOT NULL,
    -- VARCHAR(255): Maksimum 255 karakterlik metin
    -- UNIQUE: Her token farklı olmalı, aynı token iki kez kullanılamaz
    --         Örnek: "abc123" token'ı başkası tarafından kullanılıyorsa
    --                yeni bir token "xyz789" gibi farklı olmalı
    -- NOT NULL: Boş token olamaz, mutlaka bir değer olmalı
    --
    -- TOKEN NASIL OLUŞTURULUR:
    -- Genellikle rastgele bir string üretilir (C++ kodunda)
    -- Örnek: "a7f3c9e1-4b2d-4c8a-9f1e-6d3b2a1c8e5f" (UUID formatı)
    -- Veya: "MTIzNDU2Nzg5MGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6" (Base64 encoded)
    -- Önemli: Tahmin edilemez olmalı, şifrelenmiş veriden üretilmeli
    
    -- ----------------------------------------------------------------
    -- SÜTUN 4: Son Kullanma Tarihi
    -- ----------------------------------------------------------------
    expires_at TIMESTAMP NOT NULL,
    -- TIMESTAMP: Tarih ve saat bilgisi
    --            Format: 'YYYY-MM-DD HH:MM:SS'
    --            Örnek: '2024-01-15 18:30:00'
    -- NOT NULL: Boş olamaz, her token'ın son kullanma tarihi olmalı
    --
    -- NEDEN SON KULLANMA TARİHİ VAR?
    -- * Güvenlik: Eski token'lar zamanla geçersiz olmalı
    -- * Örnek: Token çalınırsa sınırsız kullanılamaz
    -- * Genellikle 24 saat, 7 gün gibi süreler verilir
    --
    -- NASIL ÇALIŞIR:
    -- 1. Token oluşturulduğunda: expires_at = NOW() + INTERVAL '24 hours'
    --    (Şu andan 24 saat sonrası)
    -- 2. Her istekte kontrol: IF NOW() > expires_at THEN 'Token geçersiz'
    -- 3. Token geçersiz olunca kullanıcı tekrar giriş yapmalı
    --
    -- ÖRNEK SENARYO:
    -- * Ahmet 15 Ocak 14:00'da giriş yaptı
    -- * Token'ın expires_at'i: 16 Ocak 14:00 (24 saat sonra)
    -- * 16 Ocak 13:00'da token hala geçerli
    -- * 16 Ocak 15:00'da token geçersiz, tekrar giriş yapmalı
    
    -- ----------------------------------------------------------------
    -- SÜTUN 5: Oluşturulma Zamanı
    -- ----------------------------------------------------------------
    created_at TIMESTAMP DEFAULT NOW()
    -- TIMESTAMP: Tarih ve saat
    -- DEFAULT NOW(): Token oluşturulduğunda otomatik olarak o anki zaman yazılır
    -- Bu değer hiç değişmez - token'ın ne zaman oluşturulduğunu gösterir
    --
    -- KULLANIM AMACI:
    -- * Loglama: "Bu token 3 gün önce oluşturuldu" bilgisi
    -- * İstatistik: "Günde kaç token oluşturuluyor?" analizi
    -- * Debugging: Token sorunlarını araştırırken yardımcı olur
);

-- ====================================================================
-- 2. PERFORMANS İÇİN İNDEKSLER OLUŞTUR
-- ====================================================================
-- INDEX: Veritabanında hızlı arama için kullanılır
-- Kitabın sonundaki dizin gibi - kelimeyi bulmak için tüm kitabı okumaya gerek yok

-- ----------------------------------------------------------------
-- INDEX 1: Token String'e Göre Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_tokens_token ON tokens(token);
-- idx_tokens_token: İndeksin ismi
-- ON tokens(token): tokens tablosunda token sütununa indeks
--
-- NEDEN EN ÖNEMLİ İNDEKS:
-- Her API isteğinde token kontrolü yapılır:
-- "SELECT * FROM tokens WHERE token = 'abc123xyz'"
-- Bu sorgu saniyede yüzlerce kez çalışır!
-- İndeks olmazsa: 1 milyon token varsa 1 milyonunu tek tek kontrol eder (YAVAŞ)
-- İndeks varsa: Direkt istenen token'a gider (ÇOK HIZLI)
--
-- PERFORMANS FARKI:
-- * İndeks yok: ~1000ms (1 saniye) - KABUL EDİLEMEZ
-- * İndeks var: ~1ms (1 milisaniye) - MÜKEMMEL

-- ----------------------------------------------------------------
-- INDEX 2: Kullanıcıya Göre Token Bulma
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_tokens_user_id ON tokens(user_id);
-- ON tokens(user_id): user_id sütununa indeks
--
-- NEDEN LAZIM:
-- * Bir kullanıcının tüm token'larını bulmak için
--   "SELECT * FROM tokens WHERE user_id = 5"
-- * Kullanıcı çıkış yaptığında tüm token'larını silmek için
--   "DELETE FROM tokens WHERE user_id = 5"
-- * Aynı anda birden fazla cihazdan giriş yapıldığını tespit etmek için
--
-- ÖRNEK KULLANIM:
-- Admin panelinde "Ahmet'in kaç aktif oturumu var?" sorgusu
-- SELECT COUNT(*) FROM tokens WHERE user_id = 5 AND expires_at > NOW()

-- ----------------------------------------------------------------
-- INDEX 3: Geçerlilik Tarihine Göre Arama
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_tokens_expires_at ON tokens(expires_at);
-- ON tokens(expires_at): expires_at sütununa indeks
--
-- NEDEN LAZIM:
-- * Süresi dolmuş token'ları otomatik temizlemek için
--   "DELETE FROM tokens WHERE expires_at < NOW()"
-- * Bu sorgu günlük bir CRON job ile çalıştırılır
-- * İndeks olmazsa tüm token'ları kontrol eder (yavaş)
--
-- CRON JOB ÖRNEK:
-- Her gece saat 03:00'da çalışan bir script:
-- DELETE FROM tokens WHERE expires_at < NOW();
-- Bu sayede veritabanı temiz kalır, gereksiz kayıtlar silinir

-- ====================================================================
-- 3. OTOMATİK TEMİZLİK FONKSİYONU OLUŞTUR
-- ====================================================================
-- Süresi dolmuş token'ları otomatik olarak silecek bir fonksiyon

CREATE OR REPLACE FUNCTION cleanup_expired_tokens()
-- CREATE OR REPLACE FUNCTION: Fonksiyon oluştur, varsa üstüne yaz
-- cleanup_expired_tokens(): Fonksiyonun adı
RETURNS void AS $$
-- RETURNS void: Bu fonksiyon bir değer döndürmez (sadece iş yapar)
-- $$ ... $$: Fonksiyon kodunu sarmalayan işaretler
BEGIN
    -- BEGIN: Fonksiyon kodu başlıyor
    
    DELETE FROM tokens WHERE expires_at < NOW();
    -- DELETE FROM tokens: tokens tablosundan sil
    -- WHERE expires_at < NOW(): Son kullanma tarihi geçmiş olanları
    -- NOW(): Şu anki zaman
    --
    -- MANTIK:
    -- Eğer expires_at = '2024-01-15 14:00' ve
    --    NOW() = '2024-01-16 10:00' ise
    --    expires_at < NOW() --> DOĞRU --> SİL
    --
    -- Örnek: Bugün 16 Ocak saat 10:00
    --        15 Ocak'ta sona eren token'lar silinecek
    
    RAISE NOTICE 'Süresi dolmuş token''lar temizlendi.';
    -- RAISE NOTICE: Ekrana bilgi mesajı yazdır
    -- İki tırnak ('') bir tırnak (') karakteri yazmak için
END;
$$ LANGUAGE plpgsql;
-- LANGUAGE plpgsql: PostgreSQL programlama dili

-- Bu fonksiyonu manuel olarak şöyle çağırabilirsiniz:
-- SELECT cleanup_expired_tokens();
--
-- VEYA bir CRON job kurun:
-- crontab -e
-- 0 3 * * * psql -U server_user -d server_db -c "SELECT cleanup_expired_tokens();"
-- (Her gün saat 03:00'da çalışır)

-- ====================================================================
-- 4. TABLO HAKKINDA AÇIKLAMA EKLE
-- ====================================================================
COMMENT ON TABLE tokens IS 'Kullanıcı oturum token''larını saklayan tablo. Her token bir kullanıcının aktif oturumunu temsil eder ve belirli bir süre sonra otomatik olarak geçersiz olur.';

-- Sütunlar için açıklamalar:
COMMENT ON COLUMN tokens.id IS 'Otomatik artan benzersiz token kimliği';
COMMENT ON COLUMN tokens.user_id IS 'Token''ın ait olduğu kullanıcının ID''si (users.id foreign key)';
COMMENT ON COLUMN tokens.token IS 'Benzersiz token string''i - kimlik doğrulama için kullanılır';
COMMENT ON COLUMN tokens.expires_at IS 'Token''ın son kullanma tarihi - bu tarihten sonra geçersiz olur';
COMMENT ON COLUMN tokens.created_at IS 'Token''ın oluşturulma tarihi';

-- ====================================================================
-- TABLO YAPISI GÖRSELLEŞTİRME
-- ====================================================================
-- Tokens tablosu Excel'de şöyle görünür:
--
-- +----+---------+------------------+---------------------+---------------------+
-- | id | user_id | token            | expires_at          | created_at          |
-- +----+---------+------------------+---------------------+---------------------+
-- |  1 |    1    | abc123xyz...     | 2024-01-16 14:00:00 | 2024-01-15 14:00:00 |
-- |  2 |    2    | def456uvw...     | 2024-01-17 09:30:00 | 2024-01-16 09:30:00 |
-- |  3 |    1    | ghi789rst...     | 2024-01-18 20:15:00 | 2024-01-17 20:15:00 |
-- |  4 |    3    | jkl012opq...     | 2024-01-16 11:00:00 | 2024-01-15 11:00:00 |
-- +----+---------+------------------+---------------------+---------------------+
--        ^                                    ^
--        |                                    |
--   users.id'ye referans            Geçerlilik süresi (24 saat sonra)
--
-- ÖRNEK SENARYO:
-- * user_id=1 olan kullanıcının (örn. ahmet) 2 token'ı var (id=1 ve id=3)
-- * Bu, ahmet'in 2 farklı cihazdan giriş yaptığı anlamına gelir
--   (Örn: telefon ve bilgisayar)
-- * id=4 olan token'ın expires_at'i geçmiş (bugün 17 Ocak ise)
--   Bu token artık geçersiz, cleanup fonksiyonu silebilir

-- ====================================================================
-- İLİŞKİ DİYAGRAMI
-- ====================================================================
--
--   ┌─────────────────┐         ┌─────────────────┐
--   │     USERS       │         │     TOKENS      │
--   ├─────────────────┤         ├─────────────────┤
--   │ id (PK)         │◄───────┤ user_id (FK)    │
--   │ username        │         │ token           │
--   │ password_hash   │         │ expires_at      │
--   │ ...             │         │ ...             │
--   └─────────────────┘         └─────────────────┘
--        (1)                           (*)
--    Bir kullanıcı              Birden fazla token
--                               (Her cihaz için 1)
--
-- PK = Primary Key (Birincil Anahtar)
-- FK = Foreign Key (Yabancı Anahtar)
-- (*) = Çoklu ilişki (bir kullanıcının birden fazla token'ı olabilir)
-- (1) = Tekli ilişki (her token sadece bir kullanıcıya ait)

-- ====================================================================
-- ÖRNEK SORGULAR (Nasıl Kullanılır?)
-- ====================================================================

-- 1. Yeni token oluştur:
-- INSERT INTO tokens (user_id, token, expires_at)
-- VALUES (1, 'yeni_token_abc123', NOW() + INTERVAL '24 hours');
--        ^    ^                   ^
--        |    |                   Şu andan 24 saat sonra
--        |    Rastgele üretilmiş token string
--        Kullanıcının ID'si (users tablosundan)

-- 2. Token geçerli mi kontrol et:
-- SELECT * FROM tokens 
-- WHERE token = 'abc123xyz' AND expires_at > NOW();
-- Eğer sonuç dönerse: Token geçerli
-- Eğer sonuç dönmezse: Token geçersiz veya yok

-- 3. Kullanıcının tüm aktif token'larını listele:
-- SELECT token, expires_at, created_at 
-- FROM tokens 
-- WHERE user_id = 1 AND expires_at > NOW();

-- 4. Kullanıcıyı çıkış yaptır (tüm token'larını sil):
-- DELETE FROM tokens WHERE user_id = 1;

-- 5. Belirli bir token'ı sil:
-- DELETE FROM tokens WHERE token = 'abc123xyz';

-- 6. Süresi dolmuş tüm token'ları temizle:
-- DELETE FROM tokens WHERE expires_at < NOW();

-- 7. Token'ın sahibini öğren:
-- SELECT u.username, u.email 
-- FROM tokens t 
-- JOIN users u ON t.user_id = u.id 
-- WHERE t.token = 'abc123xyz';

-- 8. Kullanıcının kaç aktif oturumu var:
-- SELECT COUNT(*) AS active_sessions 
-- FROM tokens 
-- WHERE user_id = 1 AND expires_at > NOW();

-- ====================================================================
-- GÜVENLİK ÖNERİLERİ
-- ====================================================================
-- 1. Token'lar kriptografik olarak güvenli rastgele sayı üreteci ile oluşturulmalı
-- 2. Token'lar HTTPS üzerinden iletilmeli (HTTP değil)
-- 3. Token'lar veritabanında şifrelenmiş saklanabilir (ekstra güvenlik)
-- 4. Şüpheli aktivite tespit edilirse token'lar iptal edilmeli
-- 5. Hassas işlemler için (şifre değiştirme) ek doğrulama istenmeli
-- 6. Token'ların son kullanma süresi çok uzun olmamalı (max 30 gün)
-- 7. "Beni hatırla" özelliği için ayrı long-lived token'lar kullanılabilir

-- ====================================================================
-- SON NOTLAR
-- ====================================================================
-- * Bu tablo JWT (JSON Web Token) yerine database session token kullanır
-- * JWT'den fark: Token'lar veritabanında saklanır, hemen iptal edilebilir
-- * JWT avantajı: Veritabanı sorgusu gerektirmez (stateless)
-- * Database token avantajı: Anında iptal edilebilir, daha fazla kontrol
-- * Hangisi daha iyi? Projeye göre değişir, bu proje için database tercih edilmiş
--
-- * ON DELETE CASCADE: Kullanıcı silinince token'ları da otomatik silinir
-- * FOREIGN KEY: İki tablo arasında ilişki kurar, veri bütünlüğünü sağlar
-- * INDEX: Performans için kritik, özellikle token sütunu için
-- * INTERVAL: PostgreSQL'de zaman aralığı belirtmek için
--   Örnekler: '1 hour', '24 hours', '7 days', '30 days'