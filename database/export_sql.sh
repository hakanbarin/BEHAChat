#!/usr/bin/env bash
set -euo pipefail

# Export PostgreSQL schema and sample data into project folder
# Usage:
#   ./export_sql.sh                 # uses defaults for this project
#   DB_USER=u DB_PASS=p DB_NAME=d ./export_sql.sh

DB_USER="${DB_USER:-server_user}"
DB_PASS="${DB_PASS:-secure_password_123}"
DB_NAME="${DB_NAME:-server_db}"
DB_HOST="${DB_HOST:-localhost}"
DB_PORT="${DB_PORT:-5432}"

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT_DIR="$ROOT_DIR/exports"
SCHEMAS_DIR="$OUT_DIR/schemas"
PREVIEWS_DIR="$OUT_DIR/previews"
DUMPS_DIR="$OUT_DIR/dumps"

mkdir -p "$SCHEMAS_DIR" "$PREVIEWS_DIR" "$DUMPS_DIR"

export PGPASSWORD="$DB_PASS"
PSQL_COMMON=(psql -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -X -v ON_ERROR_STOP=1 -P pager=off)

printf "[export] Connecting to %s@%s:%s/%s\n" "$DB_USER" "$DB_HOST" "$DB_PORT" "$DB_NAME"

# 1) List all tables
"${PSQL_COMMON[@]}" -c "\dt+" > "$OUT_DIR/tables.txt"

# 2) Dump detailed schema of each key table
for tbl in users tokens bans session_logs; do
  echo "[export] Schema for $tbl"
  "${PSQL_COMMON[@]}" -c "\d+ $tbl" > "$SCHEMAS_DIR/${tbl}_schema.txt" || true
  # Sample rows (up to 100)
  echo "[export] Preview data from $tbl"
  "${PSQL_COMMON[@]}" -A -F "," -c "SELECT * FROM $tbl ORDER BY 1 DESC LIMIT 100" > "$PREVIEWS_DIR/${tbl}_preview.csv" || true
done

# 3) Full schema-only dump
echo "[export] Full schema dump"
pg_dump -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -s > "$DUMPS_DIR/schema_dump.sql"

# 4) Data-only dump with INSERTs (small to medium datasets)
echo "[export] Data-only dump"
pg_dump -h "$DB_HOST" -p "$DB_PORT" -U "$DB_USER" -d "$DB_NAME" -a --inserts > "$DUMPS_DIR/data_dump.sql"

# 5) Basic stats (row counts)
echo "[export] Row counts"
"${PSQL_COMMON[@]}" -A -F "," -c "SELECT 'users' AS table, COUNT(*) FROM users UNION ALL SELECT 'tokens', COUNT(*) FROM tokens UNION ALL SELECT 'bans', COUNT(*) FROM bans UNION ALL SELECT 'session_logs', COUNT(*) FROM session_logs" > "$OUT_DIR/table_counts.csv" || true

unset PGPASSWORD

echo "[export] Done. See $OUT_DIR"