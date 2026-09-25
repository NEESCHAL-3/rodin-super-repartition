#!/usr/bin/env bash
set -euo pipefail
OUT="${1:-$HOME/rodin-super-backup}"
mkdir -p "$OUT"

DISK="$(adb shell 'blockdev --getsize64 /dev/block/sdc' | tr -d '\r')"
SUPER="$(adb shell 'blockdev --getsize64 /dev/block/by-name/super' | tr -d '\r')"
[[ "$DISK" == "264484421632" ]] || { echo "REFUSING: unexpected sdc size $DISK"; exit 1; }
[[ "$SUPER" == "11811160064" ]] || { echo "REFUSING: source super is not stock 11 GiB ($SUPER)"; exit 1; }

echo 'Backing up GPT ends...'
adb exec-out "dd if=/dev/block/sdc bs=4096 count=1024 2>/dev/null" > "$OUT/sdc-first-4MiB.bin"
adb exec-out "dd if=/dev/block/sdc bs=4096 skip=64570368 count=1024 2>/dev/null" > "$OUT/sdc-last-4MiB.bin"

for p in ffu mem countrycode_a countrycode_b oops charger rescue blackbox flashinfo; do
  echo "Backing up $p..."
  adb exec-out "dd if=/dev/block/by-name/$p bs=4096 2>/dev/null" > "$OUT/$p.img"
done

(
  cd "$OUT"
  sha256sum sdc-first-4MiB.bin sdc-last-4MiB.bin ffu.img mem.img countrycode_a.img countrycode_b.img oops.img charger.img rescue.img blackbox.img flashinfo.img > SHA256SUMS.txt
  sha256sum -c SHA256SUMS.txt
)

echo "Backup complete: $OUT"
echo 'Copy it somewhere safe/off-device before continuing.'
