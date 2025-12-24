-- ====================================================================
-- KULLANICILAR TABLOSU (USERS TABLE)
-- ====================================================================
-- Bu dosya kullanıcı bilgilerini saklayan ana tabloyu oluşturur
-- Excel'de bir sayfa düşünün, her kullanıcı bir satır olacak
-- Her sütun ise kullanıcı hakkında bir bilgi içerecek

-- YORUM: Her SQL dosyası belirli bir görevi yapar
-- Bu dosyadaki CREATE TABLE komutu bir tablo (Excel sayfası) oluşturur

-- ====================================================================
-- 1. USERS TABLOSUNU OLUŞTUR
-- ====================================================================
CREATE TABLE IF NOT EXISTS users (
    -- IF NOT EXISTS: Eğer bu tablo zaten varsa hata verme, varsa atla
    -- Bu sayede script'i birden fazla kez çalıştırabiliriz
    
    -- ----------------------------------------------------------------
    -- SÜTUN 1: Kullanıcı ID (Benzersiz Kimlik Numarası)
    -- ----------------------------------------------------------------
    id SERIAL PRIMARY KEY,
    -- SERIAL: Otomatik artan sayı (1, 2, 3, 4, ...) 
    --         Her yeni kullanıcı eklendiğinde PostgreSQL otomatik bir numara verir
    --         Örnek: İlk kullanıcı id=1, ikinci id=2, üçüncü id=3 alır
    -- PRIMARY KEY: Bu sütun tablonun ANA ANAHTARI'dır
    --              * Her kullanıcının id'si farklı olmalı (benzersiz/unique)
    --              * Boş olamaz (NULL değer alamaz)
    --              * Bu id ile kullanıcıları hızlıca bulabiliriz
    --              Excel'de "A" sütunu gibi düşünün - her satırın numarası
    
    -- ----------------------------------------------------------------
    -- SÜTUN 2: Kullanıcı Adı
    -- ----------------------------------------------------------------
    username VARCHAR(50) UNIQUE NOT NULL,
    -- VARCHAR(50): Değişken uzunlukta metin, maksimum 50 karakter
    --              VARCHAR = VARiable CHARacter (Değişken Karakter)
    --              Örnek: "ahmet" 5 karakter kullanır, "mehmet123" 9 karakter
    --              50 karakterden uzun kullanıcı adı giremezsiniz
    -- UNIQUE: İki kullanıcı aynı kullanıcı adını kullanamaz
    --         Örnek: Eğer "ahmet" kullanıcı adı alındıysa, 
    --                başka biri "ahmet" kullanıcı adını alamaz
    -- NOT NULL: Bu alan boş bırakılamaz, mutlaka bir değer olmalı
    --           Her kullanıcının mutlaka bir kullanıcı adı olmalı
    
    -- ----------------------------------------------------------------
    -- SÜTUN 3: Şifre Hash'i (Şifrelenmiş Şifre)
    -- ----------------------------------------------------------------
    password_hash VARCHAR(255) NOT NULL,
    -- VARCHAR(255): Maksimum 255 karakterlik metin
    -- Gerçek şifreyi değil, şifrenin şifrelenmiş halini saklarız (güvenlik)
    -- Örnek: Kullanıcı "1234" şifresini girerse
    --        Biz "$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy" gibi
    --        bir hash (şifrelenmiş metin) olarak saklarız
    -- Böylece veritabanına biri girerse gerçek şifreleri göremez
    -- NOT NULL: Şifre mutlaka olmalı
    
    -- ----------------------------------------------------------------
    -- SÜTUN 4: Email Adresi
    -- ----------------------------------------------------------------
    email VARCHAR(100) UNIQUE,
    -- VARCHAR(100): Maksimum 100 karakterlik email adresi
    -- UNIQUE: Her kullanıcının farklı email'i olmalı
    --         Örnek: "ahmet@gmail.com" başka biri tarafından kullanılıyorsa
    --                siz de aynı emaili kullanamazsınız
    -- NOT NULL YOK: Email opsiyonel, girilmeyebilir (NULL olabilir)
    
    -- ----------------------------------------------------------------
    -- SÜTUN 5: Yetki Seviyesi (Permission Level)
    -- ----------------------------------------------------------------
    permission_level INT DEFAULT 2 CHECK (permission_level >= 0 AND permission_level <= 4),
    -- INT: Tam sayı (Integer) - Örnek: 0, 1, 2, 3, 4
    -- DEFAULT 2: Eğer yetki seviyesi belirtilmezse otomatik olarak 2 (USER) atanır
    --            Yeni kayıt olan kullanıcılar varsayılan olarak normal kullanıcı (USER) olur
    -- CHECK (...): Bu alan için bir KURAL koyuyoruz
    --              permission_level değeri 0 ile 4 arasında olmalı
    --              Eğer 5, -1 gibi bir değer girmeye çalışırsanız hata verir
    --
    -- YETKİ SEVİYELERİ:
    -- 0 = ADMIN (Yönetici): Her şeyi yapabilir, tüm yetkilere sahip
    -- 1 = MODERATOR (Moderatör): Kullanıcıları banlayabilir, mesajları silebilir
    -- 2 = USER (Normal Kullanıcı): Mesaj gönderebilir, odalara katılabilir
    -- 3 = GUEST (Misafir): Sadece bakabilir, mesaj gönderemez
    -- 4 = BANNED (Banlanmış): Sisteme girişi engellenmiş
    
    -- ----------------------------------------------------------------
    -- SÜTUN 6: Çevrimiçi Durumu
    -- ----------------------------------------------------------------
    is_online BOOLEAN DEFAULT FALSE,
    -- BOOLEAN: Mantıksal değer - Ya TRUE (doğru) ya FALSE (yanlış)
    --          Açık/Kapalı, Evet/Hayır, 1/0 gibi düşünebilirsiniz
    -- DEFAULT FALSE: Varsayılan olarak çevrimdışı (offline)
    --                Kullanıcı giriş yapınca TRUE yapılacak
    -- Örnek: is_online = TRUE  --> Kullanıcı şu anda çevrimiçi
    --        is_online = FALSE --> Kullanıcı çevrimdışı
    
    -- ----------------------------------------------------------------
    -- SÜTUN 7: Son Aktivite Zamanı
    -- ----------------------------------------------------------------
    last_activity TIMESTAMP,
    -- TIMESTAMP: Tarih ve saat bilgisi
    --            Format: 'YYYY-MM-DD HH:MM:SS' 
    --            Örnek: '2024-01-15 14:30:25' (15 Ocak 2024, saat 14:30:25)
    -- Bu alanda kullanıcının en son ne zaman aktif olduğunu saklıyoruz
    -- Örnek kullanım: "3 dakika önce aktifti" gibi bilgiler göstermek için
    -- NOT NULL YOK: Başlangıçta NULL olabilir
    
    -- ----------------------------------------------------------------
    -- SÜTUN 8: Oluşturulma Zamanı
    -- ----------------------------------------------------------------
    created_at TIMESTAMP DEFAULT NOW(),
    -- TIMESTAMP: Tarih ve saat
    -- DEFAULT NOW(): Varsayılan değer olarak şu anki zamanı kullan
    --                NOW() fonksiyonu PostgreSQL'in anlık tarih-saat bilgisini verir
    -- Kullanıcı kaydı oluşturulduğunda otomatik olarak o anki zaman yazılır
    -- Bu değer hiç değişmez - kullanıcının ne zaman kayıt olduğunu gösterir
    
    -- ----------------------------------------------------------------
    -- SÜTUN 9: Güncellenme Zamanı
    -- ----------------------------------------------------------------
    updated_at TIMESTAMP DEFAULT NOW()
    -- TIMESTAMP: Tarih ve saat
    -- DEFAULT NOW(): İlk başta şu anki zaman
    -- Bu değer kullanıcı bilgileri her güncellendiğinde değişecek
    -- Örnek: Kullanıcı şifresini değiştirirse, updated_at yeni zamana güncellenir
    -- Aşağıda bir TRIGGER (tetikleyici) tanımlayacağız, 
    -- bu trigger her UPDATE'te bu alanı otomatik güncelleyecek
);

-- ====================================================================
-- 2. PERFORMANS İÇİN İNDEKSLER OLUŞTUR
-- ====================================================================
-- INDEX (indeks): Kitabın sonundaki dizin gibi
-- Kitapta bir kelimeyi ararken tüm sayfaları karıştırmak yerine
-- dizine bakıp hangi sayfada olduğunu öğreniriz - çok daha hızlı!
-- Veritabanında da aynı mantık: indeks olmadan tüm satırları tek tek kontrol eder
-- indeks varsa direkt istediğimiz satıra gider

-- ----------------------------------------------------------------
-- INDEX 1: Username'e göre arama indeksi
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_users_username ON users(username);
-- idx_users_username: İndeksin ismi (idx = index kısaltması)
-- ON users(username): users tablosunda username sütununa indeks oluştur
-- NEDEN LAZIM: Kullanıcı giriş yaparken "username" ile arama yaparız
--              "SELECT * FROM users WHERE username = 'ahmet'"
--              İndeks olmazsa tüm kullanıcıları tek tek kontrol eder (yavaş)
--              İndeks varsa direkt "ahmet"i bulur (çok hızlı)

-- ----------------------------------------------------------------
-- INDEX 2: Email'e göre arama indeksi
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
-- Email ile arama yaptığımızda hızlı çalışması için
-- "Şifremi unuttum" özelliğinde email ile kullanıcı ararız

-- ----------------------------------------------------------------
-- INDEX 3: Yetki seviyesine göre arama indeksi
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_users_permission ON users(permission_level);
-- NEDEN LAZIM: "Tüm adminleri getir" veya "Tüm banlanmışları getir" 
--              gibi sorguları hızlandırır
-- Örnek sorgu: "SELECT * FROM users WHERE permission_level = 0" (tüm adminler)

-- ----------------------------------------------------------------
-- INDEX 4: Çevrimiçi kullanıcıları bulmak için indeks
-- ----------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_users_online ON users(is_online);
-- NEDEN LAZIM: "Şu anda kimler çevrimiçi?" sorusunu hızlı cevaplayabilmek için
-- Örnek sorgu: "SELECT * FROM users WHERE is_online = TRUE"

-- ====================================================================
-- 3. OTOMATİK GÜNCELLEME İÇİN TRİGGER OLUŞTUR
-- ====================================================================
-- TRIGGER (Tetikleyici): Belirli bir olay olduğunda otomatik çalışan kod
-- Örnek: Bir kullanıcı bilgisi değiştiğinde otomatik olarak 
--        updated_at alanını güncelle

-- Önce bir FONKSİYON oluşturalım:
CREATE OR REPLACE FUNCTION update_updated_at_column()
-- CREATE OR REPLACE FUNCTION: Bir fonksiyon oluştur, varsa üstüne yaz
-- update_updated_at_column(): Fonksiyonun adı
RETURNS TRIGGER AS $$
-- RETURNS TRIGGER: Bu fonksiyon bir trigger tarafından çağrılacak
-- $$ ... $$: Fonksiyon kodunu içine yazacağımız alan (tırnak işareti gibi)
BEGIN
    -- BEGIN: Fonksiyon kodu başlıyor
    
    NEW.updated_at = NOW();
    -- NEW: Güncellenmiş yeni veri (UPDATE işleminden sonraki veri)
    -- NEW.updated_at: Yeni verinin updated_at alanı
    -- NOW(): Şu anki zaman
    -- YANI: Güncellenen kaydın updated_at alanını şu anki zamana ayarla
    
    RETURN NEW;
    -- RETURN NEW: Güncellenmiş veriyi geri döndür
    -- Bu sayede değişiklik kaydedilir
END;
$$ LANGUAGE plpgsql;
-- LANGUAGE plpgsql: Bu fonksiyon PL/pgSQL dilinde yazılmış
--                   (PostgreSQL'in programlama dili)

-- Şimdi bu fonksiyonu çalıştıracak TRIGGER'ı oluşturalım:
CREATE TRIGGER trigger_users_updated_at
    -- trigger_users_updated_at: Trigger'ın ismi
    BEFORE UPDATE ON users
    -- BEFORE UPDATE: users tablosunda UPDATE olmadan ÖNCE çalış
    -- ON users: Bu trigger users tablosu için geçerli
    FOR EACH ROW
    -- FOR EACH ROW: Her güncellenen satır için ayrı ayrı çalış
    --               Eğer 10 kullanıcı güncellenirse, 10 kez çalışır
    EXECUTE FUNCTION update_updated_at_column();
    -- EXECUTE FUNCTION: Yukarıda tanımladığımız fonksiyonu çalıştır

-- ÇALIŞMA MANTIĞI:
-- 1. Kullanıcı bir UPDATE komutu çalıştırır
--    Örnek: UPDATE users SET email = 'yeni@email.com' WHERE id = 5
-- 2. PostgreSQL bu UPDATE'i yapmadan ÖNCE trigger'ı çalıştırır
-- 3. Trigger, update_updated_at_column() fonksiyonunu çağırır
-- 4. Fonksiyon updated_at'i şu anki zamana günceller
-- 5. Artık hem email güncellenmiş, hem de updated_at güncellenmiş olur

-- ====================================================================
-- 4. TEST VERİLERİ EKLE
-- ====================================================================
-- Sistemde bazı başlangıç kullanıcıları oluşturalım
-- Bu test için, gerçek üretim ortamında silebilirsiniz

DO $$
-- DO $$ ... $$: Tek seferlik bir kod bloğu çalıştır (fonksiyon değil)
BEGIN
    -- Eğer users tablosu boşsa test kullanıcıları ekle
    IF NOT EXISTS (SELECT 1 FROM users LIMIT 1) THEN
        -- IF NOT EXISTS: Eğer yoksa
        -- SELECT 1 FROM users LIMIT 1: users'dan 1 satır getir
        -- Eğer hiç satır yoksa tablo boş demektir
        
        -- Admin kullanıcısı ekle
        INSERT INTO users (username, password_hash, email, permission_level)
        -- INSERT INTO: Tabloya yeni satır ekle
        -- users: Hangi tabloya ekliyoruz
        -- (username, ...): Hangi sütunlara veri giriyoruz
        VALUES ('admin', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', 'admin@server.com', 0);
        -- VALUES: Girilecek değerler
        -- 'admin': kullanıcı adı
        -- '$2a$10$...': şifrelenmiş şifre (gerçek şifre: "admin123")
        -- 'admin@server.com': email
        -- 0: permission_level = ADMIN
        -- NOT: id, is_online, created_at, updated_at belirtmedik çünkü
        --      bunlar otomatik değerler alıyor (SERIAL, DEFAULT)
        
        -- Moderator kullanıcısı ekle
        INSERT INTO users (username, password_hash, email, permission_level)
        VALUES ('moderator', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', 'mod@server.com', 1);
        -- permission_level = 1 (MODERATOR)
        
        -- Normal kullanıcı ekle
        INSERT INTO users (username, password_hash, email, permission_level)
        VALUES ('user1', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', 'user1@server.com', 2);
        -- permission_level = 2 (USER) - zaten DEFAULT değer 2 ama açıkça yazdık
        
        -- Misafir kullanıcı ekle
        INSERT INTO users (username, password_hash, email, permission_level)
        VALUES ('guest', '$2a$10$N9qo8uLOickgx2ZMRZoMyeIjZAgcfl7p92ldGxad68LJZdL17lhWy', 'guest@server.com', 3);
        -- permission_level = 3 (GUEST)
        
        RAISE NOTICE 'Test kullanıcıları başarıyla oluşturuldu!';
        -- RAISE NOTICE: Ekrana mesaj yazdır (kullanıcıya bilgi ver)
    ELSE
        RAISE NOTICE 'Users tablosu zaten dolu, test kullanıcıları eklenmedi.';
    END IF;
END $$;

-- ====================================================================
-- 5. TABLO HAKKINDA AÇIKLAMA EKLE
-- ====================================================================
-- COMMENT ON: Veritabanına açıklama ekler (dokümantasyon için)
COMMENT ON TABLE users IS 'Kullanıcı bilgilerini saklayan ana tablo. Her kullanıcının kimlik, yetki ve aktivite bilgileri burada tutulur.';

-- Her sütun için de açıklama ekleyelim:
COMMENT ON COLUMN users.id IS 'Otomatik artan benzersiz kullanıcı kimliği';
COMMENT ON COLUMN users.username IS 'Benzersiz kullanıcı adı (maksimum 50 karakter)';
COMMENT ON COLUMN users.password_hash IS 'Bcrypt ile şifrelenmiş şifre hash''i';
COMMENT ON COLUMN users.email IS 'Kullanıcının email adresi (opsiyonel ama benzersiz)';
COMMENT ON COLUMN users.permission_level IS 'Yetki seviyesi: 0=ADMIN, 1=MODERATOR, 2=USER, 3=GUEST, 4=BANNED';
COMMENT ON COLUMN users.is_online IS 'Kullanıcının şu anda çevrimiçi olup olmadığı';
COMMENT ON COLUMN users.last_activity IS 'Kullanıcının son aktivite zamanı';
COMMENT ON COLUMN users.created_at IS 'Kullanıcının kayıt tarihi';
COMMENT ON COLUMN users.updated_at IS 'Kullanıcı bilgilerinin son güncellenme tarihi';

-- ====================================================================
-- TABLO YAPISI GÖRSELLEŞTİRME
-- ====================================================================
-- Users tablosu Excel'de şöyle görünür:
--
-- +----+----------+-----------+-------+-----------------+----------+--------------+-----------+-----------+
-- | id | username | password_ | email | permission_     | is_      | last_        | created_  | updated_  |
-- |    |          | hash      |       | level           | online   | activity     | at        | at        |
-- +----+----------+-----------+-------+-----------------+----------+--------------+-----------+-----------+
-- |  1 | admin    | $2a$10... | ad... | 0 (ADMIN)       | TRUE     | 2024-01-...  | 2024-01...| 2024-01...|
-- |  2 | moderator| $2a$10... | mo... | 1 (MODERATOR)   | FALSE    | 2024-01-...  | 2024-01...| 2024-01...|
-- |  3 | ahmet    | $2a$10... | ah... | 2 (USER)        | TRUE     | 2024-01-...  | 2024-01...| 2024-01...|
-- |  4 | mehmet   | $2a$10... | me... | 2 (USER)        | FALSE    | NULL         | 2024-01...| 2024-01...|
-- |  5 | banned_  | $2a$10... | ba... | 4 (BANNED)      | FALSE    | 2024-01-...  | 2024-01...| 2024-01...|
-- |    | user     |           |       |                 |          |              |           |           |
-- +----+----------+-----------+-------+-----------------+----------+--------------+-----------+-----------+

-- ====================================================================
-- ÖRNEK SORGULAR (Nasıl Kullanılır?)
-- ====================================================================

-- Tüm kullanıcıları listele:
-- SELECT * FROM users;

-- Sadece admınleri listele:
-- SELECT * FROM users WHERE permission_level = 0;

-- Çevrimiçi kullanıcıları listele:
-- SELECT username, email FROM users WHERE is_online = TRUE;

-- Belirli bir kullanıcıyı bul:
-- SELECT * FROM users WHERE username = 'ahmet';

-- Yeni kullanıcı ekle:
-- INSERT INTO users (username, password_hash, email) 
-- VALUES ('yeni_kullanici', 'hash_değeri', 'email@example.com');

-- Kullanıcı şifresini güncelle:
-- UPDATE users SET password_hash = 'yeni_hash' WHERE username = 'ahmet';
-- (Bu UPDATE çalışınca trigger devreye girip updated_at otomatik güncellenir!)

-- Kullanıcıyı sil:
-- DELETE FROM users WHERE id = 5;

-- ====================================================================
-- SON NOTLAR
-- ====================================================================
-- * SERIAL: PostgreSQL'e özgü bir tip, otomatik artan sayı için
-- * VARCHAR: Değişken uzunlukta metin, sabit uzunluk için CHAR kullanılır
-- * TIMESTAMP: Tarih ve saat birlikte, sadece tarih için DATE kullanılır
-- * BOOLEAN: true/false değerleri için
-- * PRIMARY KEY: Tablonun ana anahtarı, benzersiz ve NULL olamaz
-- * UNIQUE: Bu sütunda aynı değer iki kez olamaz
-- * NOT NULL: Bu sütun boş bırakılamaz
-- * DEFAULT: Değer girilmezse kullanılacak varsayılan değer
-- * CHECK: Sütun için bir kural/kısıtlama
-- * INDEX: Arama performansını artırır (kitabın dizini gibi)
-- * TRIGGER: Belirli olaylarda otomatik çalışan kod
-- * COMMENT ON: Dokümantasyon için açıklamalar