-- ═══════════════════════════════════════════════════════════════════════════
--                         MESAJLAR TABLOSU (MESSAGES TABLE)
-- Gerçek zamanlı chat mesajlarını saklar
-- ═══════════════════════════════════════════════════════════════════════════

-- ====================================================================
-- 1. MESSAGES TABLOSUNU OLUŞTUR
-- ====================================================================
CREATE TABLE IF NOT EXISTS messages (
    -- Mesaj ID (Benzersiz Kimlik)
    id BIGSERIAL PRIMARY KEY,
    -- BIGSERIAL: Çok büyük sayılar için (1, 2, 3, ... 9223372036854775807)
    --            Milyonlarca mesaj saklayabiliriz
    
    -- Gönderen kullanıcı ID'si
    sender_id INT NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    -- REFERENCES users(id): users tablosundaki id'ye referans
    -- ON DELETE CASCADE: Eğer kullanıcı silinirse, mesajları da silinir
    
    -- Gönderen kullanıcı adı (hızlı erişim için, normalize edilmemiş)
    sender_username VARCHAR(50) NOT NULL,
    -- Normalizasyon: sender_id ile users tablosundan username alınabilir
    -- Ama performans için burada da saklıyoruz
    
    -- Mesaj içeriği
    message_text TEXT NOT NULL,
    -- TEXT: Sınırsız uzunlukta metin (VARCHAR'dan farklı)
    
    -- Gönderenin yetki seviyesi (mesaj gönderildiği andaki)
    sender_permission INT NOT NULL CHECK (sender_permission >= 0 AND sender_permission <= 4),
    
    -- Mesaj zamanı
    created_at TIMESTAMP DEFAULT NOW() NOT NULL,
    
    -- Sistem mesajı mı? (Admin/Mod broadcast mesajları için)
    is_system BOOLEAN DEFAULT FALSE,
    
    -- Özel mesaj mı? (Kullanıcıdan kullanıcıya)
    is_private BOOLEAN DEFAULT FALSE,
    
    -- Özel mesaj için hedef kullanıcı ID'si
    recipient_id INT REFERENCES users(id) ON DELETE SET NULL,
    -- ON DELETE SET NULL: Eğer hedef kullanıcı silinirse, recipient_id NULL olur
    --                     ama mesaj silinmez (geçmiş için)
    
    -- Özel mesaj için hedef kullanıcı adı
    recipient_username VARCHAR(50),
    
    -- Mesaj silindi mi? (Soft delete)
    is_deleted BOOLEAN DEFAULT FALSE,
    
    -- Silinme zamanı
    deleted_at TIMESTAMP
);

-- ====================================================================
-- 2. PERFORMANS İÇİN İNDEKSLER
-- ====================================================================

-- Gönderen kullanıcıya göre arama
CREATE INDEX IF NOT EXISTS idx_messages_sender_id ON messages(sender_id);

-- Zaman sırasına göre arama (en yeni mesajlar)
CREATE INDEX IF NOT EXISTS idx_messages_created_at ON messages(created_at DESC);

-- Özel mesajlar için hedef kullanıcıya göre arama
CREATE INDEX IF NOT EXISTS idx_messages_recipient_id ON messages(recipient_id) WHERE is_private = TRUE;

-- Sistem mesajları için arama
CREATE INDEX IF NOT EXISTS idx_messages_is_system ON messages(is_system) WHERE is_system = TRUE;

-- Silinmemiş mesajlar için arama
CREATE INDEX IF NOT EXISTS idx_messages_not_deleted ON messages(created_at DESC) WHERE is_deleted = FALSE;

-- Kullanıcı adına göre arama (hızlı erişim için)
CREATE INDEX IF NOT EXISTS idx_messages_sender_username ON messages(sender_username);

-- ====================================================================
-- 3. YORUMLAR
-- ====================================================================
COMMENT ON TABLE messages IS 'Gerçek zamanlı chat mesajlarını saklayan tablo. Hem genel hem özel mesajlar burada tutulur.';

COMMENT ON COLUMN messages.id IS 'Otomatik artan benzersiz mesaj kimliği';
COMMENT ON COLUMN messages.sender_id IS 'Mesajı gönderen kullanıcının ID''si (users tablosuna referans)';
COMMENT ON COLUMN messages.sender_username IS 'Gönderen kullanıcı adı (performans için denormalize edilmiş)';
COMMENT ON COLUMN messages.message_text IS 'Mesaj içeriği';
COMMENT ON COLUMN messages.sender_permission IS 'Gönderenin yetki seviyesi (mesaj gönderildiği andaki)';
COMMENT ON COLUMN messages.created_at IS 'Mesajın gönderilme zamanı';
COMMENT ON COLUMN messages.is_system IS 'Sistem mesajı mı? (Admin/Mod broadcast)';
COMMENT ON COLUMN messages.is_private IS 'Özel mesaj mı? (Kullanıcıdan kullanıcıya)';
COMMENT ON COLUMN messages.recipient_id IS 'Özel mesaj için hedef kullanıcı ID''si';
COMMENT ON COLUMN messages.recipient_username IS 'Özel mesaj için hedef kullanıcı adı';
COMMENT ON COLUMN messages.is_deleted IS 'Mesaj silindi mi? (Soft delete)';
COMMENT ON COLUMN messages.deleted_at IS 'Mesajın silinme zamanı';

-- ====================================================================
-- 4. ÖRNEK SORGULAR
-- ====================================================================

-- Son 50 mesajı getir (genel chat)
-- SELECT * FROM messages 
-- WHERE is_private = FALSE AND is_deleted = FALSE 
-- ORDER BY created_at DESC 
-- LIMIT 50;

-- Belirli bir kullanıcının mesajlarını getir
-- SELECT * FROM messages 
-- WHERE sender_id = 1 AND is_deleted = FALSE 
-- ORDER BY created_at DESC;

-- İki kullanıcı arasındaki özel mesajları getir
-- SELECT * FROM messages 
-- WHERE is_private = TRUE 
--   AND ((sender_id = 1 AND recipient_id = 2) OR (sender_id = 2 AND recipient_id = 1))
--   AND is_deleted = FALSE
-- ORDER BY created_at ASC;

-- Sistem mesajlarını getir
-- SELECT * FROM messages 
-- WHERE is_system = TRUE 
-- ORDER BY created_at DESC;

