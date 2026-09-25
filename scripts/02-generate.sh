#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKUP="${1:-$HOME/rodin-super-backup}"
OUT="${2:-$ROOT/out}"
python3 "$ROOT/tools/make_gpt.py" --profile "$ROOT/profiles/rodin.json" --backup-dir "$BACKUP" --out "$OUT"
echo
echo '===== DEV 20G ====='; cat "$OUT/dev20g/manifest.json"
echo
echo '===== STOCK RESTORE ====='; cat "$OUT/stock/manifest.json"
