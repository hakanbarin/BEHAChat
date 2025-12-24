-- ═══════════════════════════════════════════════════════════════════════════
--                         DATABASE İLK KURULUM DOSYASI
--                         Veritabanının temel ayarlarını yapar
-- ═══════════════════════════════════════════════════════════════════════════

-- ─────────────────────────────────────────────────────────────────────────
-- KARAKTER KODLAMASI AYARLARI
-- Türkçe karakterlerin (ğ, ü, ş, ı, ç, ö) düzgün çalışması için gerekli
-- ─────────────────────────────────────────────────────────────────────────

-- SET = Ayar yap
-- client_encoding = İstemci ile sunucu arasındaki karakter kodlaması
-- UTF8 = Evrensel karakter seti (tüm dilleri destekler)
SET client_encoding = 'UTF8';

-- ─────────────────────────────────────────────────────────────────────────
-- SAAT DİLİMİ AYARI
-- Tüm tarih/saat kayıtları bu saat dilimine göre saklanır
-- ─────────────────────────────────────────────────────────────────────────

-- timezone = Saat dilimi ayarı
-- Europe/Istanbul = Türkiye saati (GMT+3)
-- Örnek: 2025-12-19 14:30:00+03 şeklinde kaydeder
SET timezone = 'Europe/Istanbul';

-- ─────────────────────────────────────────────────────────────────────────
-- SCHEMA VERSİYON TABLOSU
-- Hangi SQL dosyalarının çalıştırıldığını takip eder
-- Git gibi ama database için versiyon kontrolü
-- ─────────────────────────────────────────────────────────────────────────

-- DROP TABLE IF EXISTS = Eğer bu tablo varsa önce sil
-- CASCADE = Bu tabloya bağlı diğer nesneleri de sil
DROP TABLE IF EXISTS schema_version CASCADE;

-- CREATE TABLE = Yeni tablo oluştur
CREATE TABLE schema_version (
    -- version = Versiyon numarası (0, 1, 2, 3, ...)
    -- INTEGER = Tam sayı veri tipi (-2147483648 ile 2147483647 arası)
    -- PRIMARY KEY = Ana anahtar (her satır benzersiz olmalı, boş olamaz)
    version INTEGER PRIMARY KEY,
    
    -- applied_at = Bu versiyon ne zaman uygulandı?
    -- TIMESTAMP = Tarih ve saat bilgisi (2025-12-19 14:30:00)
    -- DEFAULT CURRENT_TIMESTAMP = Varsayılan değer şu anki zaman
    -- Yani bu sütun boş bırakılırsa otomatik şimdiki zamanı yazar
    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    -- description = Bu versiyonda ne değişti? (açıklama)
    -- TEXT = Sınırsız uzunlukta metin veri tipi
    -- NULL olabilir (boş bırakılabilir)
    description TEXT
);

-- ─────────────────────────────────────────────────────────────────────────
-- İLK VERSİYON KAYDI
-- Bu dosyanın çalıştırıldığını kaydet
-- ─────────────────────────────────────────────────────────────────────────

-- INSERT INTO = Tabloya yeni satır ekle
-- schema_version = Hangi tabloya?
-- (version, description) = Hangi sütunlara?
-- VALUES = Değerler
-- (0, '...') = 0 numaralı versiyon, açıklama metni
INSERT INTO schema_version (version, description) 
VALUES (0, 'Database ilk kurulum - Saat dilimi ve karakter kodlamasi ayarlandi');

-- ─────────────────────────────────────────────────────────────────────────
-- BİLGİLENDİRME MESAJLARI (PostgreSQL loglarına yazılır)
-- ─────────────────────────────────────────────────────────────────────────

-- COMMENT ON = Açıklama ekle (dokümantasyon)
-- TABLE schema_version = schema_version tablosu için
-- IS = Açıklama metni
COMMENT ON TABLE schema_version IS 
'Database versiyon kontrolu - Hangi SQL dosyalarinin calistirildigini takip eder';

-- RAISE NOTICE = Konsola bilgi mesajı yaz (hata değil, sadece bilgi)
DO $$
BEGIN
    RAISE NOTICE '✓ Database temel ayarlar yuklendi';
    RAISE NOTICE '✓ Karakter kodlamasi: UTF8 (Turkce karakter destegi)';
    RAISE NOTICE '✓ Saat dilimi: Europe/Istanbul (GMT+3)';
END $$;
