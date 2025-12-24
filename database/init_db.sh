#!/bin/bash

# ═══════════════════════════════════════════════════════════════════════════
#                         DATABASE SETUP SCRIPT
# ═══════════════════════════════════════════════════════════════════════════

set -e  # Hata olursa dur

# Renkler
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Değişkenler
DB_NAME="server_db"
DB_USER="server_user"
DB_PASS="secure_password_123"
SCHEMA_DIR="./schema"

echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}        PostgreSQL Database Setup Script               ${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"

# PostgreSQL yüklü mü kontrol et
if ! command -v psql &> /dev/null; then
    echo -e "${RED}✗ PostgreSQL bulunamadı!${NC}"
    echo -e "${YELLOW}Kurulum için:${NC}"
    echo "  sudo apt install postgresql postgresql-contrib libpq-dev"
    exit 1
fi

echo -e "${GREEN}✓ PostgreSQL bulundu${NC}"

# PostgreSQL versiyonunu tespit et
PG_VERSION=$(psql --version | awk '{print $3}' | cut -d. -f1)
echo -e "${GREEN}✓ PostgreSQL ${PG_VERSION} tespit edildi${NC}"

# Servisi başlat (farklı dağıtımlar için)
echo -e "${YELLOW}→ PostgreSQL servisi başlatılıyor...${NC}"

if sudo systemctl is-active --quiet postgresql; then
    echo -e "${GREEN}✓ PostgreSQL zaten çalışıyor${NC}"
elif sudo systemctl start postgresql 2>/dev/null; then
    echo -e "${GREEN}✓ PostgreSQL başlatıldı (systemctl)${NC}"
elif sudo service postgresql start 2>/dev/null; then
    echo -e "${GREEN}✓ PostgreSQL başlatıldı (service)${NC}"
elif sudo pg_ctlcluster ${PG_VERSION} main start 2>/dev/null; then
    echo -e "${GREEN}✓ PostgreSQL başlatıldı (pg_ctlcluster)${NC}"
else
    echo -e "${RED}✗ PostgreSQL başlatılamadı!${NC}"
    echo -e "${YELLOW}Manuel başlatma:${NC}"
    echo "  sudo systemctl start postgresql"
    echo "  veya"
    echo "  sudo pg_ctlcluster ${PG_VERSION} main start"
    exit 1
fi

sleep 2

# Kullanıcı var mı kontrol et
echo -e "${YELLOW}→ Kullanıcı kontrol ediliyor...${NC}"
if sudo -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname='${DB_USER}'" | grep -q 1; then
    echo -e "${YELLOW}⚠ Kullanıcı zaten mevcut, atlanıyor${NC}"
else
    echo -e "${YELLOW}→ Kullanıcı oluşturuluyor: ${DB_USER}${NC}"
    sudo -u postgres psql -c "CREATE USER ${DB_USER} WITH PASSWORD '${DB_PASS}';"
    echo -e "${GREEN}✓ Kullanıcı oluşturuldu${NC}"
fi

# Database var mı kontrol et
echo -e "${YELLOW}→ Database kontrol ediliyor...${NC}"
if sudo -u postgres psql -lqt | cut -d \| -f 1 | grep -qw ${DB_NAME}; then
    echo -e "${YELLOW}⚠ Database zaten mevcut!${NC}"
    read -p "Silip yeniden oluşturulsun mu? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo -e "${YELLOW}→ Database siliniyor...${NC}"
        sudo -u postgres psql -c "DROP DATABASE IF EXISTS ${DB_NAME};"
        sudo -u postgres psql -c "CREATE DATABASE ${DB_NAME} OWNER ${DB_USER};"
        echo -e "${GREEN}✓ Database yeniden oluşturuldu${NC}"
    else
        echo -e "${YELLOW}Mevcut database korunuyor${NC}"
    fi
else
    echo -e "${YELLOW}→ Database oluşturuluyor: ${DB_NAME}${NC}"
    sudo -u postgres psql -c "CREATE DATABASE ${DB_NAME} OWNER ${DB_USER};"
    echo -e "${GREEN}✓ Database oluşturuldu${NC}"
fi

# Schema dosyalarını yükle
echo -e "${YELLOW}→ Schema dosyaları yükleniyor...${NC}"

if [ ! -d "${SCHEMA_DIR}" ]; then
    echo -e "${RED}✗ Schema klasörü bulunamadı: ${SCHEMA_DIR}${NC}"
    exit 1
fi

for sql_file in ${SCHEMA_DIR}/*.sql; do
    if [ -f "${sql_file}" ]; then
        filename=$(basename "${sql_file}")
        echo -e "${YELLOW}  → ${filename}${NC}"
        sudo -u postgres psql -d ${DB_NAME} -f "${sql_file}" > /dev/null
        echo -e "${GREEN}  ✓ ${filename} yüklendi${NC}"
    fi
done

# İzinleri ver
echo -e "${YELLOW}→ İzinler ayarlanıyor...${NC}"
sudo -u postgres psql -d ${DB_NAME} -c "GRANT ALL PRIVILEGES ON DATABASE ${DB_NAME} TO ${DB_USER};"
sudo -u postgres psql -d ${DB_NAME} -c "GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO ${DB_USER};"
sudo -u postgres psql -d ${DB_NAME} -c "GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO ${DB_USER};"
echo -e "${GREEN}✓ İzinler verildi${NC}"

# Test et
echo -e "${YELLOW}→ Bağlantı testi yapılıyor...${NC}"
if PGPASSWORD=${DB_PASS} psql -h localhost -U ${DB_USER} -d ${DB_NAME} -c "SELECT 1;" > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Bağlantı başarılı!${NC}"
else
    echo -e "${RED}✗ Bağlantı hatası!${NC}"
    exit 1
fi

# Bilgileri göster
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}              KURULUM TAMAMLANDI!                      ${NC}"
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"
echo ""
echo -e "${YELLOW}Database Bilgileri:${NC}"
echo "  Database: ${DB_NAME}"
echo "  User    : ${DB_USER}"
echo "  Password: ${DB_PASS}"
echo "  Host    : localhost"
echo "  Port    : 5432"
echo ""
echo -e "${YELLOW}C++ Connection String:${NC}"
echo "  postgresql://${DB_USER}:${DB_PASS}@localhost/${DB_NAME}"
echo ""
echo -e "${YELLOW}psql ile bağlanma:${NC}"
echo "  PGPASSWORD=${DB_PASS} psql -h localhost -U ${DB_USER} -d ${DB_NAME}"
echo ""
echo -e "${YELLOW}Tabloları listele:${NC}"
echo "  PGPASSWORD=${DB_PASS} psql -h localhost -U ${DB_USER} -d ${DB_NAME} -c '\\dt'"
echo ""
echo -e "${GREEN}═══════════════════════════════════════════════════════${NC}"