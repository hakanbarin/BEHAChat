-- =====================================================================
-- MIGRATION: Kullanıcı Zaman Damgaları Ekleme
-- Dosya: 001_add_user_timestamps.sql
-- Tarih: 2024
-- Açıklama: users tablosuna last_login ve last_seen kolonlarını ekler
-- =====================================================================

-- last_login: Kullanıcının en son ne zaman giriş yaptığı
-- Bu kolon giriş yapıldığında güncellenir
ALTER TABLE users ADD COLUMN IF NOT EXISTS last_login TIMESTAMP;

-- last_seen: Kullanıcının en son ne zaman aktif olduğu (çıkış zamanı)
-- Bu kolon kullanıcı offline olduğunda güncellenir
ALTER TABLE users ADD COLUMN IF NOT EXISTS last_seen TIMESTAMP;

-- INDEX: Son görülme zamanına göre sıralama için
CREATE INDEX IF NOT EXISTS idx_users_last_seen ON users(last_seen DESC NULLS LAST);

-- INDEX: Online durumuna göre filtreleme için
CREATE INDEX IF NOT EXISTS idx_users_is_online ON users(is_online);

-- Mevcut kullanıcılara varsayılan değerler atama (opsiyonel)
-- UPDATE users SET last_login = created_at WHERE last_login IS NULL;
-- UPDATE users SET last_seen = created_at WHERE last_seen IS NULL;

-- =====================================================================
-- Bu migration'ı çalıştırmak için:
-- psql -U postgres -d secure_chat -f 001_add_user_timestamps.sql
-- =====================================================================
