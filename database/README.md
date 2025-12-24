# 🗄️ Database Schema

Bu klasör PostgreSQL database schema dosyalarını içerir.

## 📁 Dizin Yapısı

```
database/
├── schema/              # Database tabloları ve yapısı
│   ├── 00_init.sql     # İlk kurulum
│   ├── 01_users.sql    # Users tablosu
│   ├── 02_tokens.sql   # Tokens tablosu
│   ├── 03_bans.sql     # Bans tablosu
│   └── 04_session_logs.sql
├── migrations/          # İleride yapılacak değişiklikler
└── init_db.sh          # Otomatik kurulum scripti
```

## 🚀 Hızlı Kurulum

```bash
# 1. PostgreSQL kur (eğer yoksa)
sudo apt install postgresql postgresql-contrib libpq-dev

# 2. Database'i otomatik kur
cd database/
./init_db.sh

# 3. Bağlantı testi
psql -U server_user -d server_db -c "SELECT * FROM users;"
```

## 🔌 Client-Server Mimarisi

```
┌──────────────────────────────────────┐
│  C++ Application (database.cpp)      │
│  ↓ libpqxx (Client library)          │
└──────────────┬───────────────────────┘
               │ TCP/Unix Socket
               │ Port: 5432
               ↓
┌──────────────────────────────────────┐
│  PostgreSQL Server (Daemon)          │
│  /usr/lib/postgresql/15/bin/postgres │
│  ↓                                    │
│  Data: /var/lib/postgresql/15/main/  │
│  - base/                             │
│  - global/                           │
│  - pg_wal/                           │
└──────────────────────────────────────┘
```

## 📊 Tablolar

### 1️⃣ users
- Kullanıcı bilgileri
- Yetki seviyeleri (ADMIN=0, USER=2, BANNED=4)
- Online/offline durumu

### 2️⃣ tokens
- Session token'ları
- Geçerlilik süreleri
- IP ve user agent bilgisi

### 3️⃣ bans
- Ban kayıtları
- Süre ve sebep
- Otomatik sona erme

### 4️⃣ session_logs
- Kullanıcı aktiviteleri
- Login/logout logları
- Yetki değişiklikleri

## 🔧 Manuel Kurulum

```bash
# PostgreSQL servisi başlat
sudo systemctl start postgresql

# Kullanıcı oluştur
sudo -u postgres createuser server_user

# Database oluştur
sudo -u postgres createdb -O server_user server_db

# Schema'ları yükle
for f in schema/*.sql; do
    sudo -u postgres psql -d server_db -f "$f"
done
```

## 📝 Bağlantı Bilgileri

**C++ Connection String:**
```cpp
pqxx::connection conn("postgresql://server_user:secure_password_123@localhost/server_db");
```

**psql ile bağlanma:**
```bash
psql -h localhost -U server_user -d server_db
```

## ⚙️ Yapılandırma

Bağlantı stringini değiştirmek için:
- `src/database.cpp` → Constructor'daki connection string
- `database/init_db.sh` → DB_USER, DB_PASS değişkenleri

## 🔍 Faydalı Komutlar

```bash
# Tabloları listele
psql -U server_user -d server_db -c "\dt"

# Kullanıcıları gör
psql -U server_user -d server_db -c "SELECT * FROM users;"

# Schema versiyonu
psql -U server_user -d server_db -c "SELECT * FROM schema_version;"

# Database boyutu
psql -U server_user -d server_db -c "SELECT pg_size_pretty(pg_database_size('server_db'));"
```

## 🧹 Temizlik

```bash
# Database'i sil
sudo -u postgres dropdb server_db

# Kullanıcıyı sil
sudo -u postgres dropuser server_user
```

## 📚 Notlar

- PostgreSQL **server** ayrı bir process olarak `/usr/lib/postgresql/` dizininde çalışır
- `database.cpp` sadece **client** kodu - network üzerinden sunucuya bağlanır
- Gerçek data `/var/lib/postgresql/` klasöründe saklanır
- Her SQL dosyası bağımsız çalıştırılabilir (idempotent)
